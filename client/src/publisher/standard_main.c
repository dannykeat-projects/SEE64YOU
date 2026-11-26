#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include "buffer.h"
#include "document.h"
#include "storage/local.h"
#include "platform/uci.h"
#include "platform/reu.h"
#include "platform/dos_path.h"
#include "platform/package_paths.h"
#include "publisher/settings_store.h"
#include "publisher/standard_transport.h"
#include "publisher/standard_display.h"
#include "publisher/local_picker.h"
#include "publisher/standard_publish_names.h"

static S64Buffer body;
static S64Document document;
static S64LocalDocument local;
static S64Dos dos;
static S64UciCapabilities capabilities;
static S64SettingsExchange settings;
static S64StandardClient client;
static S64StandardReply reply;
/* Server replies store up to 80 characters: publisher/standard_display.c
 * keeps that text and draws it across three 39-column rows, so every stored
 * character reaches the 40x25 grid and stale text is cleared each time. */
static char remote_name[65], prompt[65];
static uint8_t loaded, network_available, path_captured;
static char original_path[440];
/* Browsed server rows are no longer used when the exit-only DOS readback
 * starts. Sharing this scratch space saves 390 bytes of C64 program memory. */
static union {
    char remote_rows[6][65];
    char verify_path[440];
} scratch;
static unsigned char old_background, old_border, old_text;

/* Every action fits on one 40-column row; digits work without a mouse. */
static void draw_action(uint8_t row, const char* text)
{
    gotoxy(2u, row);
    revers(1u);
    cputs(text);
    revers(0u);
}

static void render(void)
{
    clrscr();
    s64_publish_put(0u, "SEE64YOU SOURCE PUBLISHER", 39u);
    s64_publish_put(1u, "---------------------------------------", 39u);
    s64_publish_put(2u, "LOCAL SOURCE", 39u);
    s64_publish_put(3u, loaded ? local.filename : "(8tml / 8ss / txt)", 39u);
    s64_publish_put(5u, "REMOTE TARGET", 39u);
    s64_publish_put(6u, remote_name[0] ? remote_name : "(choose name)", 39u);
    s64_publish_put(8u, "SERVER", 39u);
    s64_publish_put(9u, s64_settings_publish_id(&settings)[0] ? settings.base_url : "Import key in S64CFG.PRG", 39u);

    draw_action(11u, "[1] BROWSE LOCAL FILES");
    draw_action(13u, "[2] BROWSE REMOTE");
    draw_action(15u, "[3] PUBLISH / REPLACE");
    draw_action(17u, "[4] TEST CONNECTION");
    draw_action(19u, "[0] EXIT TO BASIC");

    s64_publish_put(21u, "HTTP source is not encrypted.", 39u);
    s64_publish_draw(22u);
}

/* conio writes screen memory synchronously: show feedback before a blocking
 * DOS/NET operation rather than after its response arrives. */
static void working(const char* operation)
{
    clrscr();
    cputs(operation);
    cputs("\r\nPLEASE WAIT...");
}

static uint8_t input(const char* label, char* output, uint8_t capacity)
{
    uint8_t length = 0u;
    unsigned char key;
    clrscr(); cputs(label); cputs("\r\nSTOP CANCEL\r\n");
    cursor(1);
    for (;;) {
        key = cgetc();
        if (key == CH_STOP) { cursor(0); return 0u; }
        if (key == CH_ENTER) {
            output[length] = 0; cursor(0); return length != 0u;
        }
        if (key == CH_DEL) {
            if (length) { --length; cputc(CH_DEL); }
        } else if (key >= 32u && key != 127u && length + 1u < capacity) {
            output[length++] = (char)key; cputc(key);
        }
    }
}

static void load_settings(void)
{
    memset(&settings, 0, sizeof(settings));
    if (!capabilities.dos_target) { s64_publish_message("Settings storage unavailable"); return; }
    if (s64_settings_publish_store_load(&dos, &settings) != S64_SETTINGS_STORE_OK
        || !s64_settings_publish_id(&settings)[0]) {
        memset(&settings, 0, sizeof(settings));
        s64_publish_message("Import standard key in S64CFG.PRG");
    } else s64_publish_message("Standard destination loaded");
    client.url = settings.base_url;
    client.id = s64_settings_publish_id(&settings);
    client.key = settings.direct_key;
    client.allow_http = settings.allow_insecure_http;
}

static uint8_t ready(void)
{
    if (!network_available) { s64_publish_message("C64U network target unavailable"); return 0u; }
    if (!s64_settings_publish_id(&settings)[0]) {
        s64_publish_message("Import standard key in S64CFG.PRG"); return 0u;
    }
    return 1u;
}

static void show_result(S64StandardResult result)
{
    if (result == S64_STANDARD_OK) s64_publish_message("Endpoint ready");
    else if (result == S64_STANDARD_UNCERTAIN) s64_publish_message("Outcome unknown. Inspect remote first.");
    else if (result == S64_STANDARD_TLS) s64_publish_message("HTTPS unavailable; no downgrade");
    else if (result == S64_STANDARD_CONSENT) s64_publish_message("Allow HTTP explicitly in Settings");
    else if (result == S64_STANDARD_TOO_LARGE) s64_publish_message("Source exceeds server limit");
    else if (reply.message[0]) s64_publish_message(reply.message);
    else s64_publish_message("Publishing failed; check endpoint/key");
    if (result == S64_STANDARD_SERVER || result == S64_STANDARD_PROTOCOL
        || result == S64_STANDARD_UNCERTAIN || result == S64_STANDARD_NETWORK) {
        static const char* const stages[] = {"STATUS", "CHALLENGE", "LIST", "PUT"};
        clrscr();
        cputs(stages[reply.operation]);
        if (reply.http_code[0]) { cputs(" HTTP "); cputs(reply.http_code); }
        s64_publish_put(2u, reply.error_code, 39u);
        s64_publish_draw(4u);
        s64_publish_put(10u, "Press a key to return", 39u);
        cgetc();
    }
}

static uint16_t remote_start, remote_seen;
static uint8_t remote_count, remote_more;

static void remote_file(const char* name, uint32_t size)
{
    (void)size;
    if (remote_seen++ < remote_start) return;
    if (remote_count < 6u) strcpy(scratch.remote_rows[remote_count++], name);
    else remote_more = 1u;
}

/* Selection only changes the target. Publish performs its own fresh check. */
static uint8_t browse_remote(void)
{
    S64StandardResult result;
    unsigned char key;
    uint8_t row, col;
    if (!ready()) return input("REMOTE NAME (OFFLINE):", prompt, sizeof(prompt));
    remote_start = 0u;
    for (;;) {
        remote_seen = 0u; remote_count = 0u; remote_more = 0u;
        working("READING SERVER FILES");
        result = s64_standard_browse(&client, &reply, remote_file);
        if (result != S64_STANDARD_OK) {
            show_result(result);
            clrscr();
            s64_publish_draw(20u);
            s64_publish_put(23u, "7 SET NAME  0 BACK", 39u);
            for (;;) {
                key = cgetc();
                if (key == '0' || key == CH_STOP) return 0u;
                if (key == '7') return input("REMOTE NAME:", prompt, sizeof(prompt));
            }
        }
        clrscr(); cputs("BROWSE REMOTE\r\n");
        for (row = 0u; row < remote_count; ++row) {
            gotoxy(0u, 3u + row * 2u); cputc('1' + row); cputs(" ");
            for (col = 0u; scratch.remote_rows[row][col]; ++col) {
                if (col == 37u) gotoxy(2u, 4u + row * 2u);
                cputc(scratch.remote_rows[row][col]);
            }
        }
        if (!remote_count) s64_publish_put(3u, "(no remote files)", 39u);
        s64_publish_put(17u, "7 SET NAME  8 PREVIOUS  9 NEXT", 39u);
        s64_publish_put(19u, "0 BACK / STOP", 39u);
        s64_publish_put(21u, "Select sets target; does not upload.", 39u);
        for (;;) {
            key = cgetc();
            if (key == '0' || key == CH_STOP) return 0u;
            if (key == '7') return input("REMOTE NAME:", prompt, sizeof(prompt));
            if (key >= '1' && key < '1' + remote_count) {
                strcpy(prompt, scratch.remote_rows[key - '1']); return 1u;
            }
            if (key == '8' && remote_start) { remote_start -= 6u; break; }
            if (key == '9' && remote_more) { remote_start += 6u; break; }
        }
    }
}

static int write_source(S64UciDataSink sink, void* context, void* source)
{
    return s64_document_write_wire((S64Document*)source, sink, context) == S64_DOCUMENT_OK;
}

static void publish(void)
{
    S64StandardResult result;
    unsigned char key;
    if (!loaded) { s64_publish_message("Choose local source first"); return; }
    if (!remote_name[0]) { s64_publish_message("Choose remote name first"); return; }
    if (!ready()) return;
    working("CHECKING SERVER");
    result = s64_standard_status(&client, &reply);
    if (result != S64_STANDARD_OK) { show_result(result); return; }
    if ((uint32_t)s64_buffer_length(&body) > reply.max_size) {
        s64_publish_message("Source exceeds server limit"); return;
    }
    working("CHECKING REMOTE FILE");
    result = s64_standard_list(&client, remote_name, &reply);
    if (result != S64_STANDARD_OK) { show_result(result); return; }
    /* Confirm both identities; local source may differ from remote target. */
    clrscr(); cputs("LOCAL SOURCE:\r\n"); cputs(local.filename);
    cputs("\r\nSERVER:\r\n"); cputs(settings.base_url);
    cputs("\r\nREMOTE TARGET:\r\n"); cputs(remote_name);
    cputs(reply.exists ? "\r\nREPLACE EXISTING FILE?\r\n" : "\r\nPUBLISH PUBLIC FILE?\r\n");
    cputs("List cannot prevent concurrent changes.\r\nUploaded source is public.\r\n[1] CONFIRM  [0] CANCEL");
    key = cgetc();
    if (key != '1' && key != 'y' && key != 'Y') { s64_publish_message("Publish cancelled"); return; }
    working("UPLOADING FILE");
    result = s64_standard_put(&client, remote_name, s64_buffer_length(&body), write_source, &document, &reply);
    if (result != S64_STANDARD_OK) { show_result(result); return; }
    clrscr(); cputs("PUBLISHED\r\n"); cputs(reply.url);
    if (reply.warning) { cputs("\r\nWARNING: "); cputs(reply.message); }
    cputs("\r\nPRESS KEY"); cgetc();
    s64_publish_message(reply.warning ? "Published with server warning" : "Published; URL shown");
}

int main(void)
{
    unsigned char key;
    S64LocalResult result;
    uint8_t selection;
    old_background = bgcolor(COLOR_BLUE);
    old_border = bordercolor(COLOR_BLUE);
    old_text = textcolor(COLOR_LIGHTBLUE);
    cursor(0);
    if (!s64_reu_probe() || s64_reu_probe_banks() != 256u) {
        clrscr(); cputs("SEE64YOU PUBLISHER\r\n16 MB REU REQUIRED\r\nPRESS KEY"); cgetc();
    } else {
        s64_buffer_init(&body); s64_document_init(&document, &body);
        s64_dos_init(&dos, s64_uci_execute, S64_DOS_TARGET_PRIMARY);
        s64_local_init(&local, &dos, &document);
        s64_uci_probe_capabilities(&capabilities);
        network_available = s64_uci_probe_target(0x03u);
        s64_net_init(&client.network, s64_uci_execute);
        if (capabilities.dos_target) {
            path_captured = s64_dos_get_path(&dos, original_path, sizeof(original_path)) == S64_DOS_OK;
            if (!path_captured) capabilities.dos_target = 0u;
        }
        load_settings();
        for (;;) {
            render(); key = cgetc();
            if (key == CH_STOP || key == '0') {
                if (path_captured) working("RESTORING DOS PATH");
                if (path_captured && (s64_dos_change_dir(&dos, original_path) != S64_DOS_OK
                    || s64_dos_get_path(&dos, scratch.verify_path, sizeof(scratch.verify_path)) != S64_DOS_OK
                    || strcmp(original_path, scratch.verify_path))) {
                    s64_publish_message("DOS restore failed; STOP retries exit");
                    continue;
                }
                break;
            }
            if (key == 'r' || key == 'R') { working("LOADING SETTINGS"); load_settings(); }
            else if (key == '1' || key == 'l' || key == 'L') {
                if (!capabilities.dos_target) { s64_publish_message("C64U DOS unavailable"); continue; }
                working("READING LOCAL FILES");
                if (s64_dos_change_dir(&dos, S64_PACKAGE_DOCUMENTS_DIRECTORY) != S64_DOS_OK) {
                    s64_publish_message("docs/ missing; save in Editor first"); continue;
                }
                selection = s64_local_pick(&dos, prompt, sizeof(prompt));
                if (!selection) continue;
                if (selection == 2u || selection == 3u) {
                    if (!input(selection == 3u ? "LIST ERROR. TYPE SOURCE:"
                                               : "OPEN SOURCE:",
                               prompt, S64_LOCAL_FILENAME_CAPACITY)) continue;
                }
                working("OPENING LOCAL FILE");
                result = s64_local_open(&local, prompt, 1u);
                loaded = result == S64_LOCAL_OK;
                remote_name[0] = 0;
                if (loaded) {
                    if (s64_publish_name_valid(local.filename)) {
                        strcpy(remote_name, local.filename);
                        for (key = 0u; remote_name[key]; ++key)
                            if (remote_name[key] >= 'A' && remote_name[key] <= 'Z')
                                remote_name[key] += 'a' - 'A';
                    }
                    s64_publish_message("Local source loaded");
                } else {
                    s64_buffer_init(&body); s64_document_init(&document, &body);
                    s64_publish_message(s64_local_result_message(&local, result));
                }
            } else if (key == '2' || key == 'n' || key == 'N') {
                if (browse_remote()) {
                    if (s64_publish_name_valid(prompt)) {
                        strcpy(remote_name, prompt);
                        /* The standard server lowercases names. Confirm that same path. */
                        for (key = 0u; remote_name[key]; ++key)
                            if (remote_name[key] >= 'A' && remote_name[key] <= 'Z')
                                remote_name[key] += 'a' - 'A';
                        s64_publish_message("Remote name selected");
                    } else s64_publish_message("Invalid remote name");
                }
            } else if (key == '4' || key == 't' || key == 'T') {
                if (ready()) { working("TESTING CONNECTION"); show_result(s64_standard_status(&client, &reply)); }
            } else if (key == '3' || key == 'p' || key == 'P') publish();
        }
    }
    memset(&settings, 0, sizeof(settings));
    bgcolor(old_background); bordercolor(old_border); textcolor(old_text);
    cursor(1); return 0;
}
