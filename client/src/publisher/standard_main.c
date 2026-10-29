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
#include "publisher/standard_publish_names.h"

static S64Buffer body;
static S64Document document;
static S64LocalDocument local;
static S64Dos dos;
static S64UciCapabilities capabilities;
static S64SettingsExchange settings;
static S64StandardClient client;
static S64StandardReply reply;
static char remote_name[65], prompt[65], status[81];
static uint8_t loaded, network_available, path_captured;
static char original_path[440], verify_path[440];
static unsigned char old_background, old_border, old_text;

static void message(const char* text)
{
    strncpy(status, text, sizeof(status) - 1u);
    status[sizeof(status) - 1u] = 0;
}

static void put_text(uint8_t row, const char* text, uint8_t maximum)
{
    gotoxy(0u, row);
    while (*text && maximum--) cputc(*text++);
}

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
    put_text(0u, "SEE64YOU 8TML PUBLISHER", 39u);
    put_text(1u, "---------------------------------------", 39u);
    put_text(2u, "LOCAL SOURCE", 39u);
    put_text(3u, loaded ? local.filename : "(choose .8tml file)", 39u);
    put_text(5u, "REMOTE TARGET", 39u);
    put_text(6u, remote_name[0] ? remote_name : "(choose name)", 39u);
    put_text(8u, "SERVER", 39u);
    put_text(9u, s64_settings_publish_id(&settings)[0] ? settings.base_url : "Import key in S64CFG.PRG", 39u);

    draw_action(11u, "[1] CHOOSE LOCAL FILE");
    draw_action(13u, "[2] SET REMOTE NAME");
    draw_action(15u, "[3] PUBLISH / REPLACE");
    draw_action(17u, "[4] TEST CONNECTION");
    draw_action(19u, "[0] EXIT TO BASIC");

    put_text(21u, "HTTP source is not encrypted.", 39u);
    put_text(23u, status, 39u);
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
    if (!capabilities.dos_target) { message("Settings storage unavailable"); return; }
    if (s64_settings_publish_store_load(&dos, &settings) != S64_SETTINGS_STORE_OK
        || !s64_settings_publish_id(&settings)[0]) {
        memset(&settings, 0, sizeof(settings));
        message("Import standard key in S64CFG.PRG");
    } else message("Standard destination loaded");
    client.url = settings.base_url;
    client.id = s64_settings_publish_id(&settings);
    client.key = settings.direct_key;
    client.allow_http = settings.allow_insecure_http;
}

static uint8_t ready(void)
{
    if (!network_available) { message("C64U network target unavailable"); return 0u; }
    if (!s64_settings_publish_id(&settings)[0]) {
        message("Import standard key in S64CFG.PRG"); return 0u;
    }
    return 1u;
}

static void show_result(S64StandardResult result)
{
    if (result == S64_STANDARD_OK) message("Endpoint ready");
    else if (result == S64_STANDARD_UNCERTAIN) message("Outcome unknown. Inspect remote first.");
    else if (result == S64_STANDARD_TLS) message("HTTPS unavailable; no downgrade");
    else if (result == S64_STANDARD_CONSENT) message("Allow HTTP explicitly in Settings");
    else if (result == S64_STANDARD_TOO_LARGE) message("Source exceeds server limit");
    else if (reply.message[0]) message(reply.message);
    else message("Publishing failed; check endpoint/key");
}

static int write_source(S64UciDataSink sink, void* context, void* source)
{
    return s64_document_write_wire((S64Document*)source, sink, context) == S64_DOCUMENT_OK;
}

static void publish(void)
{
    S64StandardResult result;
    unsigned char key;
    if (!loaded) { message("Choose local .8TML first"); return; }
    if (!remote_name[0]) { message("Choose remote name first"); return; }
    if (!ready()) return;
    result = s64_standard_status(&client, &reply);
    if (result != S64_STANDARD_OK) { show_result(result); return; }
    if ((uint32_t)s64_buffer_length(&body) > reply.max_size) {
        message("Source exceeds server limit"); return;
    }
    result = s64_standard_list(&client, remote_name, &reply);
    if (result != S64_STANDARD_OK) { show_result(result); return; }
    clrscr(); cputs(settings.base_url); cputs("\r\n"); cputs(remote_name);
    cputs(reply.exists ? "\r\nREPLACE EXISTING FILE?\r\n" : "\r\nPUBLISH PUBLIC FILE?\r\n");
    cputs("List cannot prevent concurrent changes.\r\nUploaded source is public.\r\n[1] CONFIRM  [0] CANCEL");
    key = cgetc();
    if (key != '1' && key != 'y' && key != 'Y') { message("Publish cancelled"); return; }
    result = s64_standard_put(&client, remote_name, s64_buffer_length(&body), write_source, &document, &reply);
    if (result != S64_STANDARD_OK) { show_result(result); return; }
    clrscr(); cputs("PUBLISHED\r\n"); cputs(reply.url);
    if (reply.warning) { cputs("\r\nWARNING: "); cputs(reply.message); }
    cputs("\r\nPRESS KEY"); cgetc();
    message(reply.warning ? "Published with server warning" : "Published; URL shown");
}

int main(void)
{
    unsigned char key;
    S64LocalResult result;
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
                if (path_captured && (s64_dos_change_dir(&dos, original_path) != S64_DOS_OK
                    || s64_dos_get_path(&dos, verify_path, sizeof(verify_path)) != S64_DOS_OK
                    || strcmp(original_path, verify_path))) {
                    message("DOS restore failed; STOP retries exit");
                    continue;
                }
                break;
            }
            if (key == 'r' || key == 'R') load_settings();
            else if (key == '1' || key == 'l' || key == 'L') {
                if (!capabilities.dos_target) { message("C64U DOS unavailable"); continue; }
                if (!input("OPEN .8TML:", prompt, S64_LOCAL_FILENAME_CAPACITY)) continue;
                if (s64_dos_change_dir(&dos, S64_PACKAGE_DIRECTORY) != S64_DOS_OK) {
                    message("Storage not ready"); continue;
                }
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
                    message("Local .8TML loaded");
                } else {
                    s64_buffer_init(&body); s64_document_init(&document, &body);
                    message(s64_local_result_message(&local, result));
                }
            } else if (key == '2' || key == 'n' || key == 'N') {
                if (input("REMOTE NAME:", prompt, sizeof(prompt))) {
                    if (s64_publish_name_valid(prompt)) {
                        strcpy(remote_name, prompt);
                        /* The standard server lowercases names. Confirm that same path. */
                        for (key = 0u; remote_name[key]; ++key)
                            if (remote_name[key] >= 'A' && remote_name[key] <= 'Z')
                                remote_name[key] += 'a' - 'A';
                        message("Remote name selected");
                    } else message("Invalid remote name");
                }
            } else if (key == '4' || key == 't' || key == 'T') {
                if (ready()) show_result(s64_standard_status(&client, &reply));
            } else if (key == '3' || key == 'p' || key == 'P') publish();
        }
    }
    memset(&settings, 0, sizeof(settings));
    bgcolor(old_background); bordercolor(old_border); textcolor(old_text);
    cursor(1); return 0;
}
