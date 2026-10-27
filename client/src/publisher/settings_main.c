#include <conio.h>
#include <string.h>
#ifdef S64_INDEPENDENT_SETTINGS
#include <stdlib.h>
#include "platform/journal_migration.h"
#include "platform/turbo.h"
#include "platform/dos_path.h"
#endif

#include "charset.h"
#ifndef S64_MANAGED_OVERLAY
#include "platform/chain.h"
#endif
#include "platform/dos.h"
#include "platform/reu.h"
#include "platform/uci.h"
#include "platform/package_paths.h"
#include "platform/package_version.h"
#include "publisher/destination.h"
#include "publisher/settings_exchange.h"
#include "publisher/settings_store.h"
#ifdef S64_PERSISTENT_RUNTIME_FIT
#include "publisher/workspace.h"
#include "publisher/persistent_settings_control.h"
#include "publisher/persistent_settings_render.h"
#include "publisher/persistent_settings_ui.h"
#endif
#ifdef S64_MANAGED_OVERLAY
#include "platform/module_arrival.h"
#include "platform/module_runtime.h"
#include "platform/module_transition.h"
#include "platform/module_transfer.h"
#include "platform/package_version.h"
#include "publisher/settings_session.h"
static S64ModuleRuntime managed_runtime;
static void managed_entry_failed(uint8_t stage)
{
    clrscr();
    cputs("Managed settings failed. Restart launcher.");
    cputc((char)('0' + stage));
    for (;;) { }
}
#endif

#ifdef S64_SETTINGS_SAVE_TRACE
#include "../../diagnostics/settings_save_trace.h"
static char save_trace_path[80], save_trace_path_status[32];
static uint8_t save_trace_path_length, save_trace_path_truncated;
static uint8_t save_trace_input_values, save_trace_input_record;
static S64UciResult save_trace_path_result;

static uint8_t save_path_sink(const uint8_t* data, uint16_t length, void* context)
{
    uint16_t i;
    uint8_t native;
    (void)context;
    for (i = 0; i < length && data[i] != 0; ++i) {
        if (save_trace_path_length + 1 < sizeof(save_trace_path)) {
            save_trace_path[save_trace_path_length++] =
                s64_charset_ascii_to_native(data[i], &native) ? native : '?';
        } else save_trace_path_truncated = 1;
    }
    save_trace_path[save_trace_path_length] = 0;
    return 1;
}

static void begin_save_trace(S64Dos* traced_dos, const S64SettingsExchange* input)
{
    uint8_t command[2];
    S64UciCommandPart part;
    S64UciReply reply;
    save_trace_reset();
    save_trace_input_values = s64_settings_exchange_values_valid(input);
    save_trace_input_record = s64_settings_exchange_validate(input);
    save_trace_path_length = save_trace_path_truncated = 0;
    save_trace_path[0] = save_trace_path_status[0] = 0;
    command[0] = traced_dos->target;
    command[1] = 0x12; /* Read-only GET_PATH; deliberately no CHANGE_DIR. */
    part.data = command;
    part.length = 2;
    part.flags = 0;
    reply.data_sink = save_path_sink;
    reply.data_context = 0;
    reply.status = save_trace_path_status;
    reply.status_capacity = sizeof(save_trace_path_status);
    reply.status_length = 0;
    save_trace_path_result = s64_uci_execute(&part, 1, &reply);
    traced_dos->execute = save_trace_execute;
}

static void end_save_trace(const char* stage, uint8_t result);
#endif

#define SCREEN_COLUMNS 40u
#define STATUS_ROW 24u
#define S64_KEY_DIAG_STATUS_CAPACITY 32u
#define S64_KEY_DIAG_TARGET 0x01u
#define S64_KEY_DIAG_OPEN 0x02u
#define S64_KEY_DIAG_CLOSE 0x03u
#define S64_KEY_DIAG_READ 0x04u

#ifdef S64_PERSISTENT_RUNTIME_FIT
typedef struct {
    char open[S64_KEY_DIAG_STATUS_CAPACITY];
    char read[S64_KEY_DIAG_STATUS_CAPACITY];
    char close[S64_KEY_DIAG_STATUS_CAPACITY];
    uint16_t bytes;
} S64PersistentKeyDiagScratch;
#endif

#ifdef S64_PERSISTENT_RUNTIME_FIT
#define settings (*(S64SettingsExchange*)&s64_publisher_workspace.bytes[S64_SETTINGS_WORK_RECORD_OFFSET])
#define profile_settings (*(S64SettingsExchange*)&s64_publisher_workspace.bytes[S64_SETTINGS_WORK_SCRATCH_OFFSET])
#define settings_screen (*(S64PersistentSettingsScreen*)&s64_publisher_workspace.bytes[S64_SETTINGS_WORK_SCRATCH_OFFSET])
#else
static S64SettingsExchange settings;
static S64SettingsExchange original_settings;
#ifndef S64_INDEPENDENT_SETTINGS
static S64SettingsExchange profile_settings;
#endif
#endif
#ifndef S64_PERSISTENT_RUNTIME_FIT
#ifndef S64_INDEPENDENT_SETTINGS
static S64PublishDestination checked_destination;
#endif
#endif
static S64Dos dos;
static S64UciCapabilities capabilities;
static char status_text[SCREEN_COLUMNS + 1u];
#ifdef S64_PERSISTENT_RUNTIME_FIT
#define relay_replacement (profile_settings.direct_key)
#else
#ifndef S64_INDEPENDENT_SETTINGS
static char relay_replacement[S64_RELAY_KEY_CAPACITY];
#endif
#endif
#ifdef S64_PERSISTENT_RUNTIME_FIT
#define key_diag_scratch (*(S64PersistentKeyDiagScratch*)&s64_publisher_workspace.bytes[S64_SETTINGS_WORK_SCRATCH_OFFSET])
#define key_diag_open (key_diag_scratch.open)
#define key_diag_read (key_diag_scratch.read)
#define key_diag_close (key_diag_scratch.close)
#define key_diag_bytes (key_diag_scratch.bytes)
#else
#ifndef S64_INDEPENDENT_SETTINGS
static char key_diag_open[S64_KEY_DIAG_STATUS_CAPACITY];
static char key_diag_read[S64_KEY_DIAG_STATUS_CAPACITY];
static char key_diag_close[S64_KEY_DIAG_STATUS_CAPACITY];
static uint16_t key_diag_bytes;
#endif
#endif
static uint8_t advanced_view;
#ifdef S64_INDEPENDENT_SETTINGS
static uint8_t setup_step;
static uint8_t original_background, original_border, original_text;
static uint8_t original_path_valid;
static char original_path[256], restored_path[256];
#endif

static uint8_t validate_settings(void);

#ifdef S64_SETTINGS_SAVE_TRACE
static void end_save_trace(const char* stage, uint8_t result)
{
    uint8_t i;
    const S64SaveTraceEntry* entry;
    const char* label;
    dos.execute = s64_uci_execute;
    clrscr();
    cputs("ACTUAL SETTINGS SAVE TRACE\r\n");
    cprintf("%s RESULT: %u\r\n", stage, result);
    cprintf("BEFORE UI: VALUES %u RECORD %u\r\n", save_trace_input_values, save_trace_input_record);
    cprintf("AFTER: VALUES %u RECORD %u DOS %u\r\n",
        s64_settings_exchange_values_valid(&settings),
        s64_settings_exchange_validate(&settings), capabilities.dos_target);
    cprintf("TYPE: %u HTTP: %u STATE: %u\r\n", settings.destination_type,
        settings.allow_insecure_http, settings.state);
    cprintf("GET_PATH UCI: %u\r\n%s\r\nPATH: %s\r\n",
        save_trace_path_result, save_trace_path_status, save_trace_path);
    if (save_trace_path_truncated) cputs("PATH DISPLAY TRUNCATED\r\n");
    if (save_trace_count == 0)
        cputs("NO SAVE DOS COMMANDS ISSUED\r\nINVALID=SETTINGS VALUES FAILURE\r\n");
    cputs("PRESS KEY FOR COMMAND RESULTS");
    cgetc();
    for (i = 0; i < save_trace_count; ++i) {
        entry = &save_trace[i];
        label = "OTHER";
        switch (entry->opcode) {
            case 2: label = "OPEN FILE (GATE)"; break;
            case 3: label = "CLOSE (READBACK CHECKED)"; break;
            case 4: label = "READ FINAL FILE"; break;
            case 5: label = "WRITE (GATE)"; break;
            case 8: label = "STAT FINAL FILE"; break;
            case 17: label = "SELECT STORAGE"; break;
            case 9: label = entry->final_file ? "DELETE CFG (BEST EFFORT)"
                : "DELETE TEMP (BEST EFFORT)"; break;
            case 10: label = "RENAME TEMP TO CFG (GATE)"; break;
        }
        clrscr();
        cprintf("COMMAND %u/%u\r\n%s\r\nOPCODE $%02X UCI %u\r\n",
            i + 1, save_trace_count, label, entry->opcode, entry->transport);
        if (entry->opcode == 2) cprintf("ATTRIBUTES $%02X\r\n", entry->attributes);
        if (entry->opcode == 5) cprintf("BYTES ATTEMPTED %u\r\n", entry->bytes);
        cprintf("RAW STATUS: %s\r\n", entry->status[0] ? entry->status : "(EMPTY)");
        cputs("PRESS KEY FOR NEXT");
        cgetc();
    }
    clrscr();
    cprintf("%s RESULT: %u\r\n", stage, result);
    i = save_trace_failed_gate();
    if (i) cprintf("FIRST FAILED DOS GATE: COMMAND %u\r\n", i);
    else cputs("NO FAILED DOS GATE CAPTURED\r\n");
    if (save_trace_overflow) cputs("TRACE FULL; LATER COMMANDS OMITTED\r\n");
    cputs("STORE: 0=OK 2=INVALID 3=IO 4=VERIFY\r\nUI/DOS/REU BOOLEAN: 0=FAIL\r\nPRESS KEY TO RETURN TO SETTINGS");
    cgetc();
}
#endif

static void clear_row(unsigned char row)
{
    unsigned char i;
    gotoxy(0u, row);
    for (i = 0u; i < SCREEN_COLUMNS; ++i) cputc(' ');
}

static void set_status(const char* text)
{
    unsigned char i = 0u;
    while (i < SCREEN_COLUMNS && text[i] != '\0') {
        status_text[i] = text[i];
        ++i;
    }
    status_text[i] = '\0';
}

#ifndef S64_PERSISTENT_RUNTIME_FIT
#ifndef S64_INDEPENDENT_SETTINGS
static const char* destination_name(void)
{
    if (settings.destination_type == (uint8_t)S64_DESTINATION_STANDALONE) return "Standalone";
    if (settings.destination_type == (uint8_t)S64_DESTINATION_WORDPRESS) return "WordPress";
    return "None";
}
#endif

static void print_value(unsigned char row, const char* label, const char* value)
{
    unsigned char col = 0u;
    clear_row(row);
    gotoxy(0u, row);
    while (col < SCREEN_COLUMNS && *label != '\0') {
        cputc((unsigned char)*label++);
        ++col;
    }
    while (col < SCREEN_COLUMNS && *value != '\0') {
        cputc((unsigned char)*value++);
        ++col;
    }
}

static void render_basic(void)
{
#ifdef S64_INDEPENDENT_SETTINGS
    clrscr();
    revers(1); cputs(" SEE64YOU PUBLISHING SETTINGS          "); revers(0);
    print_value(3u, "Endpoint: ", settings.destination_type == S64_DESTINATION_STANDARD
        ? settings.base_url : "(import a standard key file)");
    print_value(6u, "Key ID: ", s64_settings_publish_id(&settings));
    print_value(8u, "Secret: ", settings.destination_type == S64_DESTINATION_STANDARD
        ? "configured (hidden)" : "not configured");
    print_value(10u, "Unencrypted HTTP: ", settings.allow_insecure_http ? "allowed" : "blocked");
    gotoxy(0u, 13u); cputs("I IMPORT 8TMLKEY.TXT");
    gotoxy(0u, 14u); cputs("H CHANGE HTTP CONSENT");
    gotoxy(0u, 15u); cputs("D DISABLE PUBLISHING");
    gotoxy(0u, 17u); cputs("T CHECK CONFIGURATION");
    gotoxy(0u, 18u); cputs("S SAVE + RETURN");
    gotoxy(0u, 19u); cputs("RUN/STOP CANCEL + RETURN");
    gotoxy(0u, 21u); cputs("HTTP exposes document text.\r\nProvision/revoke keys over HTTPS.");
    print_value(STATUS_ROW, "", status_text);
#else

    const char* publishing;

    clrscr();
    revers(1);
    gotoxy(0u, 0u);
    cputs(" SEE64YOU SETTINGS                     ");
    revers(0);
    print_value(3u, "Website: ", settings.base_url[0] != '\0'
        ? settings.base_url : "(not set)");
    print_value(5u, "Publishing key: ", settings.direct_key[0] != '\0'
        ? "configured" : "not set");
    publishing = settings.destination_type == (uint8_t)S64_DESTINATION_NONE
        ? (const char*)"not configured"
        : destination_name();
    print_value(7u, "Publishing: ", publishing);

    gotoxy(0u, 11u); cputs("1 WEBSITE");
    gotoxy(0u, 12u); cputs("2 IMPORT SEE64KEY.TXT");
    gotoxy(0u, 13u); cputs("3 TYPE / REPLACE KEY");
    gotoxy(0u, 14u); cputs("4 DISABLE PUBLISHING KEY");
    gotoxy(0u, 17u); cputs("T CHECK READINESS");
    gotoxy(0u, 18u); cputs("A ADVANCED SETTINGS");
#ifdef S64_INDEPENDENT_SETTINGS
    gotoxy(0u, 19u); cputs("W FIRST PAGE SETUP");
    gotoxy(0u, 20u); cputs("M MOVE SESSION FILES INTO FOLDER");
#endif
    gotoxy(0u, 21u); cputs("S SAVE + RETURN");
    gotoxy(0u, 22u); cputs("RUN/STOP CANCEL + RETURN");
    clear_row(STATUS_ROW);
    gotoxy(0u, STATUS_ROW);
    revers(1); cputs(status_text); revers(0);
#endif
}

#ifndef S64_INDEPENDENT_SETTINGS
static void render_advanced(void)
{
    clrscr();
    revers(1);
    gotoxy(0u, 0u);
    cputs(" SEE64YOU ADVANCED SETTINGS            ");
    revers(0);
    gotoxy(0u, 2u); cputs("B BASIC SETTINGS");
    print_value(3u, "Destination: ", destination_name());
    print_value(5u, "Site URL: ", settings.base_url[0] != '\0' ? settings.base_url : "(not set)");
    print_value(7u, "Plain HTTP: ", settings.allow_insecure_http ? "allowed" : "blocked");
    print_value(9u, "Direct key: ", settings.direct_key[0] != '\0' ? "configured" : "off");
    print_value(10u, "Relay key: ", settings.relay_key[0] != '\0' ? "configured" : "off");
    gotoxy(0u, 12u); cputs("1 DESTINATION TYPE");
    gotoxy(0u, 13u); cputs("2 SITE URL");
    gotoxy(0u, 14u); cputs("3 PLAIN HTTP POLICY");
    gotoxy(0u, 15u); cputs("4 IMPORT DIRECT KEY FILE");
    gotoxy(0u, 16u); cputs("A TYPE DIRECT KEY");
    gotoxy(0u, 17u); cputs("5 DISABLE DIRECT KEY");
    gotoxy(0u, 18u); cputs("6 REPLACE RELAY KEY");
    gotoxy(0u, 19u); cputs("7 DISABLE RELAY KEY");
    gotoxy(0u, 20u); cputs("8 LOAD PROFILE");
    gotoxy(0u, 21u); cputs("9 SAVE PROFILE");
    gotoxy(0u, 22u); cputs("D DIAGNOSTICS");
#ifdef S64_INDEPENDENT_SETTINGS
    cputs("  M MOVE SESSION FILES");
#endif
    gotoxy(0u, 23u); cputs("S SAVE  B BASIC  STOP CANCEL");
    clear_row(STATUS_ROW);
    gotoxy(0u, STATUS_ROW);
    revers(1); cputs(status_text); revers(0);
}
#endif

#endif

#ifdef S64_INDEPENDENT_SETTINGS
#ifndef S64_INDEPENDENT_SETTINGS
static void render_setup(void)
{
    clrscr();
    cprintf("FIRST PAGE SETUP - STEP %u/3\r\n\r\n", setup_step);
    if (setup_step == 1u) {
        cputs("Create your site in a web browser.\r\nOpen start.php for the full guide.\r\n\r\nUse the site folder URL, not api.php.\r\n1 ENTER WEBSITE\r\n");
        print_value(9u, "Website: ", settings.base_url);
    } else if (setup_step == 2u) {
        cputs("In a browser use HTTPS pair.php.\r\nCopy key to:\r\n" S64_PACKAGE_DIRECTORY "SEE64KEY.TXT\r\nor type the 64 hex characters here.\r\nKeep the file/key private.\r\nNever enter your site password here.\r\n\r\n1 IMPORT KEY FILE\r\n2 TYPE / REPLACE KEY\r\n");
        print_value(12u, "Key: ", settings.direct_key[0] ? "configured" : "not set");
    } else {
        cputs("Direct publishing uses HTTP.\r\nDocument text is not encrypted.\r\n1 CHANGE HTTP CONSENT\r\n\r\nS SAVE VERIFIED SETTINGS\r\nThen launch SEE64YOU.PRG, write text,\r\nsave FIRST.S64 and exit to BASIC.\r\nLaunch S64PUB.PRG: L FIRST.S64,\r\n2 connect, T page/post, 4 published,\r\n3 publish. Wait for success.\r\nCheck the page/post in your browser.\r\n");
        print_value(16u, "Plain HTTP: ", settings.allow_insecure_http ? "allowed" : "blocked");
        cputs("\r\nHTTPS is never downgraded.\r\nUse Advanced for other transports.");
    }
    gotoxy(0u, 20u); cputs("M MOVE SESSION FILES INTO FOLDER");
    gotoxy(0u, 21u); cputs("N NEXT  B BACK  W BASIC SETTINGS");
    gotoxy(0u, 22u); cputs("STOP CANCEL / SET UP LATER");
    print_value(STATUS_ROW, "", status_text);
}
#endif
#endif

#ifdef S64_INDEPENDENT_SETTINGS
typedef struct {
    S64TurboBackend turbo;
    S64SystemState system;
} S64SettingsMigrationRuntime;

#ifndef S64_INDEPENDENT_SETTINGS
static uint8_t migration_restore(void* context,
    const S64SystemSnapshot* snapshot)
{
    S64SettingsMigrationRuntime* runtime = (S64SettingsMigrationRuntime*)context;
    return s64_system_state_adopt(&runtime->system, snapshot) == S64_SYSTEM_OK
        && s64_system_state_restore(&runtime->system) == S64_SYSTEM_OK;
}
#endif

#ifndef S64_INDEPENDENT_SETTINGS
static uint8_t migration_verify(void* context,
    const S64SystemSnapshot* snapshot)
{
    S64SettingsMigrationRuntime* runtime = (S64SettingsMigrationRuntime*)context;
    return runtime->system.backend.verify(&runtime->turbo, snapshot);
}
#endif

#ifndef S64_INDEPENDENT_SETTINGS
static void migrate_session_files(void)
{
    S64SettingsMigrationRuntime runtime;
    S64SystemBackend system_backend;
    S64SessionRecoveryBackend recovery_backend;
    S64SessionJournalResult result;
    unsigned char key;

    clrscr();
    cputs("MOVE SESSION FILES INTO SEE64YOU\r\n\r\nClose other SEE64YOU programs first.\r\nInterrupted machine settings will be\r\nrestored and verified before moving.\r\nOriginal root journals are backed up\r\nin the folder before removal.\r\nDocuments/config are not changed.\r\n\r\nCONTINUE? Y/N");
    key = (unsigned char)cgetc();
    if (key != 'y' && key != 'Y') {
        set_status("Session migration cancelled");
        return;
    }
    if (!capabilities.dos_target) {
        set_status("Migration needs Ultimate DOS");
        return;
    }
    s64_turbo_backend_init_hardware(&runtime.turbo);
    s64_turbo_backend_bind_adopt(&system_backend);
    s64_system_state_init(&runtime.system, &system_backend, &runtime.turbo);
    recovery_backend.restore = migration_restore;
    recovery_backend.verify = migration_verify;
    recovery_backend.context = &runtime;
    clrscr();
    cputs("Migrating...\r\nChecking/restoring session files.\r\nPlease wait.");
    result = s64_journal_migration_stage(&dos, &recovery_backend);
    if (result == S64_SESSION_JOURNAL_OK) {
        clrscr();
        cputs("Migrating...\r\nVerifying backups/removing root slots.\r\nPlease wait.");
        result = s64_journal_migration_retire(&dos);
    }
    clrscr();
    if (result == S64_SESSION_JOURNAL_OK) {
        cputs("SESSION FILES MOVED AND VERIFIED\r\n\r\nNew sessions use " S64_PACKAGE_DIRECTORY ".\r\nOriginal root backups are retained:\r\nS64OLD0.DAT / S64OLD1.DAT\r\nwhen those original slots existed.");
        set_status("Session migration complete");
    } else {
        cprintf("SESSION MIGRATION STOPPED: %u\r\n\r\n", result);
        cputs("Keep root/folder journals and backups.\r\nDo not delete or overwrite them.\r\nCheck storage or conflicting sessions,\r\nthen retry M from Settings.\r\nIf it repeats, collect diagnostics.");
        set_status("Migration stopped; keep files, retry M");
    }
    cputs("\r\n\r\nRETURN: BACK TO SETTINGS");
    while ((unsigned char)cgetc() != CH_ENTER) { }
}
#endif
#endif

static void render(void)
{
#ifdef S64_INDEPENDENT_SETTINGS
    render_basic();
#elif defined(S64_PERSISTENT_RUNTIME_FIT)
    if (!s64_persistent_settings_screen(
            &settings,
            advanced_view,
            status_text,
            &settings_screen
        )) {
        set_status("Settings screen unavailable");
        return;
    }
    s64_persistent_settings_render_screen(&settings_screen);
#else
    if (advanced_view) render_advanced();
    else render_basic();
#endif
}

#ifndef S64_INDEPENDENT_SETTINGS
static uint8_t printable(unsigned char key)
{
    uint8_t ascii;
    return s64_charset_native_to_ascii(key, &ascii)
        && ascii >= 0x20u && ascii <= 0x7eu;
}
#endif

#ifndef S64_INDEPENDENT_SETTINGS
static unsigned char edit_text(
    const char* label,
    char* value,
    unsigned char capacity,
    unsigned char masked
)
{
    unsigned char key;
    unsigned char length = (unsigned char)strlen(value);
    unsigned char label_length = (unsigned char)strlen(label);
    unsigned char visible;
    unsigned char start;
    unsigned char shown;

    if (capacity == 0u || label_length >= SCREEN_COLUMNS) return 0u;
    visible = (unsigned char)(SCREEN_COLUMNS - label_length - 1u);

    for (;;) {
        start = length > visible ? (unsigned char)(length - visible) : 0u;
        shown = 0u;
        clear_row(STATUS_ROW);
        revers(1);
        gotoxy(0u, STATUS_ROW);
        cputs(label);
        while (shown < visible && (unsigned char)(start + shown) < length) {
            cputc(masked ? '*' : (unsigned char)value[start + shown]);
            ++shown;
        }
        while (shown < visible) {
            cputc(' ');
            ++shown;
        }
        gotoxy((unsigned char)(label_length + (length - start)), STATUS_ROW);

        key = (unsigned char)cgetc();
        if (key == CH_STOP) {
            revers(0);
            return 0u;
        }
        if (key == CH_ENTER) {
            value[length] = '\0';
            revers(0);
            return 1u;
        }
        if (key == CH_DEL) {
            if (length != 0u) {
                --length;
                value[length] = '\0';
            }
        } else if (printable(key) && length + 1u < capacity) {
            value[length++] = (char)key;
            value[length] = '\0';
        }
    }
}
#endif

#ifndef S64_INDEPENDENT_SETTINGS
static void edit_site_url(void)
{
    if (edit_text(
            "SITE URL: ",
            settings.base_url,
            (unsigned char)sizeof(settings.base_url),
            0u
        )) {
        if (settings.base_url[0] != '\0'
            && settings.destination_type == (uint8_t)S64_DESTINATION_NONE) {
            settings.destination_type = (uint8_t)S64_DESTINATION_STANDALONE;
        }
        set_status("Website updated; S to save");
    }
}
#endif

#ifndef S64_INDEPENDENT_SETTINGS
static void replace_direct_key(void)
{
    relay_replacement[0] = '\0';
    if (edit_text(
            "DIRECT KEY: ",
            relay_replacement,
            (unsigned char)sizeof(relay_replacement),
            1u
        )) {
        if (relay_replacement[0] == '\0') {
            set_status("Direct key unchanged");
        } else {
            memcpy(
                settings.direct_key,
                relay_replacement,
                sizeof(relay_replacement)
            );
            if (!s64_settings_exchange_values_valid(&settings)) {
                settings.direct_key[0] = '\0';
                set_status("Direct key must be 64 hex chars");
            } else {
                set_status("Direct key replaced; S to save");
            }
        }
    }
    memset(relay_replacement, 0, sizeof(relay_replacement));
}
#endif

static void toggle_plain_http(void)
{
    unsigned char key;

    if (settings.allow_insecure_http) {
        settings.allow_insecure_http = 0u;
        set_status("Plain HTTP blocked");
        return;
    }

    clear_row(STATUS_ROW);
    gotoxy(0u, STATUS_ROW);
    revers(1);
    cputs("ALLOW UNENCRYPTED HTTP? Y/N");
    for (;;) {
        key = (unsigned char)cgetc();
        if (key == 'y' || key == 'Y') {
            settings.allow_insecure_http = 1u;
            revers(0);
            set_status("Plain HTTP allowed; S to save");
            return;
        }
        if (key == 'n' || key == 'N' || key == CH_STOP) {
            revers(0);
            set_status("Plain HTTP remains blocked");
            return;
        }
    }
}

static void check_settings(void)
{
    if (settings.destination_type == (uint8_t)S64_DESTINATION_NONE
        && settings.base_url[0] != '\0') {
        settings.destination_type = (uint8_t)S64_DESTINATION_STANDALONE;
    }
    if (validate_settings()) set_status("Settings are valid");
}

#ifndef S64_INDEPENDENT_SETTINGS
static void choose_destination(void)
{
    unsigned char key;
    clear_row(STATUS_ROW);
    gotoxy(0u, STATUS_ROW);
    revers(1);
    cputs("0=NONE 1=STANDALONE");
    for (;;) {
        key = (unsigned char)cgetc();
        if (key == '0') {
            settings.destination_type = (uint8_t)S64_DESTINATION_NONE;
            settings.base_url[0] = '\0';
            settings.allow_insecure_http = 0u;
            break;
        }
        if (key == '1') {
            settings.destination_type = (uint8_t)S64_DESTINATION_STANDALONE;
            break;
        }
        if (key == CH_STOP) {
            revers(0);
            set_status("Destination unchanged");
            return;
        }
    }
    revers(0);
    set_status("Destination type changed");
}
#endif

static uint8_t validate_values(S64SettingsExchange* target)
{
#ifdef S64_PERSISTENT_RUNTIME_FIT
    S64PersistentSettingsControlResult result;

    result = s64_persistent_settings_validate_values(target);
    if (result == S64_SETTINGS_CONTROL_OK) return 1u;
    if (result == S64_SETTINGS_CONTROL_INVALID_DESTINATION) {
        set_status("Publishing destination invalid");
    } else if (result == S64_SETTINGS_CONTROL_INVALID_VALUES) {
        set_status("Publishing key invalid");
    } else {
        set_status("Settings validation failed");
    }
    return 0u;
#else
#ifndef S64_INDEPENDENT_SETTINGS
    S64DestinationResult result;
#endif

#ifdef S64_INDEPENDENT_SETTINGS
    if (target != 0 && (target->destination_type == S64_DESTINATION_STANDARD
        || target->destination_type == S64_DESTINATION_NONE)
        && s64_settings_exchange_values_valid(target)) return 1u;
    set_status("Import a standard key file first");
    return 0u;
#else
    if (target == 0) return 0u;
    if (target->destination_type == (uint8_t)S64_DESTINATION_NONE) {
        target->base_url[0] = '\0';
        target->allow_insecure_http = 0u;
        return s64_settings_exchange_values_valid(target) ? 1u : 0u;
    }

    result = s64_destination_configure(
        &checked_destination,
        (S64DestinationType)target->destination_type,
        target->base_url,
        target->allow_insecure_http
    );
    if (result != S64_DESTINATION_OK) {
        set_status(s64_destination_result_message(result));
        return 0u;
    }
    memcpy(
        target->base_url,
        checked_destination.base_url,
        sizeof(target->base_url)
    );
    target->allow_insecure_http = checked_destination.allow_insecure_http;
    if (!s64_settings_exchange_values_valid(target)) {
        set_status("Publishing key invalid");
        return 0u;
    }
    return 1u;
#endif
#endif
}

static uint8_t validate_settings(void)
{
    return validate_values(&settings);
}

#ifndef S64_INDEPENDENT_SETTINGS
static uint8_t choose_profile_slot(void)
{
    unsigned char key;

    clear_row(STATUS_ROW);
    gotoxy(0u, STATUS_ROW);
    revers(1);
    cputs("PROFILE 1-4  STOP=CANCEL");

    for (;;) {
        key = (unsigned char)cgetc();
        if (key >= '1' && key <= '4') {
            revers(0);
            return (uint8_t)(key - '1');
        }
        if (key == CH_STOP) {
            revers(0);
            return 0xffu;
        }
    }
}
#endif

#ifndef S64_INDEPENDENT_SETTINGS
static void load_profile(void)
{
    uint8_t slot;

    if (!capabilities.dos_target) {
        set_status("Settings storage unavailable");
        return;
    }

    slot = choose_profile_slot();
    if (slot == 0xffu) {
        set_status("Profile load cancelled");
        return;
    }

#ifdef S64_PERSISTENT_RUNTIME_FIT
    {
        S64PersistentSettingsProfileResult result =
            s64_persistent_settings_profile_load(
                &dos, slot, &settings, &profile_settings);

        if (result == S64_SETTINGS_PROFILE_OK) {
            set_status("Profile loaded; S to use");
        } else if (result == S64_SETTINGS_PROFILE_NOT_FOUND) {
            set_status("Profile slot is empty");
        } else if (result == S64_SETTINGS_PROFILE_INVALID) {
            set_status("Profile data invalid");
        } else {
            set_status("Profile load failed");
        }
    }
#else
    {
        S64SettingsStoreResult result;

        result = s64_settings_profile_load(&dos, slot, &profile_settings);
        if (result == S64_SETTINGS_STORE_NOT_FOUND) {
            set_status("Profile slot is empty");
            return;
        }
        if (result == S64_SETTINGS_STORE_INVALID) {
            set_status("Profile data invalid");
            return;
        }
        if (result != S64_SETTINGS_STORE_OK) {
            set_status("Profile load failed");
            return;
        }
        if (!validate_values(&profile_settings)) {
            return;
        }

        settings = profile_settings;
        settings.state = S64_SETTINGS_STATE_EDIT_REQUEST;
        set_status("Profile loaded; S to use");
    }
#endif
}
#endif

#ifndef S64_INDEPENDENT_SETTINGS
static void save_profile(void)
{
    uint8_t slot;

    if (!capabilities.dos_target) {
        set_status("Settings storage unavailable");
        return;
    }

    slot = choose_profile_slot();
    if (slot == 0xffu) {
        set_status("Profile save cancelled");
        return;
    }

#ifdef S64_PERSISTENT_RUNTIME_FIT
    {
        S64PersistentSettingsProfileResult result =
            s64_persistent_settings_profile_save(
                &dos, slot, &settings, &profile_settings);

        if (result == S64_SETTINGS_PROFILE_OK) {
            set_status("Profile saved");
        } else if (result == S64_SETTINGS_PROFILE_INVALID) {
            set_status("Settings are invalid");
        } else {
            set_status("Profile save failed");
        }
    }
#else
    {
        S64SettingsStoreResult result;

        if (!validate_settings()) return;
        profile_settings = settings;
        result = s64_settings_profile_save(&dos, slot, &profile_settings);
        if (result == S64_SETTINGS_STORE_OK) {
            set_status("Profile saved");
        } else {
            set_status("Profile save failed");
        }
    }
#endif
}
#endif


#ifndef S64_INDEPENDENT_SETTINGS
static uint8_t diag_count_bytes(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    uint16_t* count = (uint16_t*)context;
    (void)data;
    *count = (uint16_t)(*count + length);
    return 1u;
}
#endif

#ifndef S64_INDEPENDENT_SETTINGS
static void diag_copy_status(
    char* destination,
    const char* source,
    uint16_t length
)
{
    uint16_t copy = length;

    if (copy >= S64_KEY_DIAG_STATUS_CAPACITY) {
        copy = S64_KEY_DIAG_STATUS_CAPACITY - 1u;
    }
    if (copy != 0u && source != 0) {
        memcpy(destination, source, copy);
    }
    destination[copy] = '\0';
    if (copy == 0u) strcpy(destination, "(empty)");
}
#endif

#ifndef S64_INDEPENDENT_SETTINGS
static void diagnose_direct_key_io(void)
{
    static const char filename[] = S64_DIRECT_KEY_FILENAME;
    uint8_t open_prefix[3];
    uint8_t read_command[4];
    uint8_t close_command[2];
    S64UciCommandPart open_parts[2];
    S64UciCommandPart part;
    S64UciReply reply;
    char status[S64_KEY_DIAG_STATUS_CAPACITY];
    uint16_t filename_length = (uint16_t)(sizeof(filename) - 1u);

    strcpy(key_diag_open, "(not run)");
    strcpy(key_diag_read, "(not run)");
    strcpy(key_diag_close, "(not run)");
    key_diag_bytes = 0u;

    open_prefix[0] = S64_KEY_DIAG_TARGET;
    open_prefix[1] = S64_KEY_DIAG_OPEN;
    open_prefix[2] = 0x01u;
    open_parts[0].data = open_prefix;
    open_parts[0].length = 3u;
    open_parts[0].flags = 0u;
    open_parts[1].data = (const uint8_t*)filename;
    open_parts[1].length = filename_length;
    open_parts[1].flags = S64_UCI_PART_NATIVE_TEXT;

    reply.data_sink = 0;
    reply.data_context = 0;
    reply.status = status;
    reply.status_capacity = (uint16_t)sizeof(status);
    reply.status_length = 0u;
    status[0] = '\0';
    (void)s64_uci_execute(open_parts, 2u, &reply);
    diag_copy_status(key_diag_open, status, reply.status_length);

    read_command[0] = S64_KEY_DIAG_TARGET;
    read_command[1] = S64_KEY_DIAG_READ;
    read_command[2] = 64u;
    read_command[3] = 0u;
    part.data = read_command;
    part.length = 4u;
    part.flags = 0u;

    reply.data_sink = diag_count_bytes;
    reply.data_context = &key_diag_bytes;
    reply.status = status;
    reply.status_capacity = (uint16_t)sizeof(status);
    reply.status_length = 0u;
    status[0] = '\0';
    (void)s64_uci_execute(&part, 1u, &reply);
    diag_copy_status(key_diag_read, status, reply.status_length);

    close_command[0] = S64_KEY_DIAG_TARGET;
    close_command[1] = S64_KEY_DIAG_CLOSE;
    part.data = close_command;
    part.length = 2u;
    part.flags = 0u;

    reply.data_sink = 0;
    reply.data_context = 0;
    reply.status = status;
    reply.status_capacity = (uint16_t)sizeof(status);
    reply.status_length = 0u;
    status[0] = '\0';
    (void)s64_uci_execute(&part, 1u, &reply);
    diag_copy_status(key_diag_close, status, reply.status_length);
}
#endif

#ifndef S64_INDEPENDENT_SETTINGS
static void show_direct_key_diagnostics(void)
{
    char number[6];
    uint16_t value = key_diag_bytes;
    uint8_t used = 0u;

    clrscr();
    cputs("DIRECT KEY IMPORT DIAGNOSTICS\r\n\r\n");
    cputs("OPEN:  "); cputs(key_diag_open); cputs("\r\n");
    cputs("READ:  "); cputs(key_diag_read); cputs("\r\n");
    cputs("BYTES: ");
    if (value == 0u) {
        cputc('0');
    } else {
        while (value != 0u && used < sizeof(number)) {
            number[used++] = (char)('0' + (value % 10u));
            value /= 10u;
        }
        while (used != 0u) cputc(number[--used]);
    }
    cputs("\r\n");
    cputs("CLOSE: "); cputs(key_diag_close); cputs("\r\n\r\n");
    cputs("PRESS A KEY TO RETURN");
    cgetc();
}
#endif


#ifndef S64_INDEPENDENT_SETTINGS
static void show_system_diagnostics(void)
{
    uint16_t banks;
    uint8_t network;
    unsigned char key;

    banks = s64_reu_probe_banks();
    network = s64_uci_probe_target(0x03u);

    clrscr();
    revers(1);
    cputs(" SEE64YOU DIAGNOSTICS                  ");
    revers(0);
    cputs("\r\n\r\n");
    cputs("UCI:       ");
    cputs(capabilities.interface_present ? "READY" : "UNAVAILABLE");
    cputs("\r\nDOS:       ");
    cputs(capabilities.dos_target ? "READY" : "UNAVAILABLE");
    cputs("\r\nNETWORK:   ");
    cputs(network ? "READY" : "UNAVAILABLE");
    cputs("\r\nHTTP $06:  ");
    cputs(capabilities.http_target ? "AVAILABLE" : "UNAVAILABLE");
    cputs("\r\nREU BANKS: ");
    cprintf("%u", banks);
    cputs(banks == 256u ? " (16 MB READY)" : " (CHECK REQUIRED)");
    cputs("\r\nPACKAGE:   ");
    cprintf("%04X", S64_PACKAGE_VERSION);
    cputs("\r\nCONFIG:    " S64_PACKAGE_DIRECTORY S64_SETTINGS_FILENAME);
    cputs("\r\n\r\nK KEY IMPORT DETAILS");
    cputs("\r\nOTHER KEY RETURN");

    key = (unsigned char)cgetc();
    if (key == 'k' || key == 'K') {
        diagnose_direct_key_io();
        show_direct_key_diagnostics();
    }
}
#endif

static void import_direct_key(void)
{
#ifdef S64_INDEPENDENT_SETTINGS
    S64SettingsStoreResult result;
    if (!capabilities.dos_target) {
        set_status("Settings storage unavailable");
        return;
    }
    result = s64_settings_import_publish_key(&dos, &settings);
    if (result == S64_SETTINGS_STORE_OK) set_status("Imported; review HTTP consent then save");
    else if (result == S64_SETTINGS_STORE_NOT_FOUND) set_status("8TMLKEY.TXT not found in app folder");
    else if (result == S64_SETTINGS_STORE_INVALID) set_status("Standard key file invalid; unchanged");
    else set_status("Key import failed; settings unchanged");
    return;
#else

    if (!capabilities.dos_target) {
        set_status("Settings storage unavailable");
        return;
    }

#ifdef S64_PERSISTENT_RUNTIME_FIT
    {
        S64PersistentSettingsKeyImportResult result =
            s64_persistent_settings_import_direct_key(
                &dos, &settings, relay_replacement);

        if (result == S64_SETTINGS_KEY_IMPORT_OK) {
            set_status("Direct key imported; S to save");
        } else if (result == S64_SETTINGS_KEY_IMPORT_NOT_FOUND) {
            set_status("SEE64KEY.TXT not found");
        } else if (result == S64_SETTINGS_KEY_IMPORT_INVALID) {
            set_status("SEE64KEY.TXT key invalid");
        } else {
            diagnose_direct_key_io();
            show_direct_key_diagnostics();
            set_status("Direct key import failed");
        }
    }
#else
    {
        S64SettingsStoreResult result;

        memset(relay_replacement, 0, sizeof(relay_replacement));
        result = s64_settings_import_direct_key(&dos, relay_replacement);
        if (result == S64_SETTINGS_STORE_NOT_FOUND) {
            set_status("SEE64KEY.TXT not found");
        } else if (result == S64_SETTINGS_STORE_INVALID) {
            set_status("SEE64KEY.TXT key invalid");
        } else if (result != S64_SETTINGS_STORE_OK) {
            diagnose_direct_key_io();
            show_direct_key_diagnostics();
            set_status("Direct key import failed");
        } else {
            memcpy(
                settings.direct_key,
                relay_replacement,
                sizeof(settings.direct_key)
            );
            set_status("Direct key imported; S to save");
        }
        memset(relay_replacement, 0, sizeof(relay_replacement));
    }
#endif
#endif
}

#ifndef S64_PERSISTENT_RUNTIME_FIT
static void return_to_publisher(void)
{
#ifdef S64_INDEPENDENT_SETTINGS
    if (original_path_valid
        && (s64_dos_change_dir(&dos, original_path) != S64_DOS_OK
            || s64_dos_get_path(&dos, restored_path, sizeof(restored_path)) != S64_DOS_OK
            || strcmp(original_path, restored_path) != 0)) {
        set_status("DOS restore failed; STOP retries exit");
        return;
    }
    bgcolor(original_background);
    bordercolor(original_border);
    textcolor(original_text);
    cursor(1);
    exit(0);
#elif defined(S64_MANAGED_OVERLAY)
    S64ModuleTransferPlan plan;
    S64ModuleTransitionResult transition;

    clrscr();
    cputs("RETURNING TO PUBLISHER...\r\n");
    transition = s64_module_transition_prepare(
        &managed_runtime,
        S64_MODULE_PUBLISHER,
        &plan
    );
    if (transition == S64_MODULE_TRANSITION_RECOVERY_REQUIRED) {
        managed_entry_failed(6u);
    }
    if (transition != S64_MODULE_TRANSITION_OK) {
        set_status("Publisher handoff failed");
        return;
    }
    if (!s64_module_transfer_execute(&plan)) {
        transition = s64_module_transition_cancel(&managed_runtime, &plan);
        if (transition != S64_MODULE_TRANSITION_OK) managed_entry_failed(6u);
        set_status("Publisher handoff failed");
    }
#else
    clrscr();
    cputs("RETURNING TO PUBLISHER...\r\n");
    s64_chain_publisher();
#endif
}

#endif

static void save_and_return(void)
{
#ifdef S64_PERSISTENT_RUNTIME_FIT
    S64PersistentSettingsCommitResult result =
        s64_persistent_settings_commit(&dos, &settings);

    if (result == S64_SETTINGS_COMMIT_INVALID) {
        set_status("Settings are invalid");
        return;
    }
    if (result == S64_SETTINGS_COMMIT_VERIFY_ERROR) {
        set_status("SAVE FAILED - READBACK INVALID");
        return;
    }
    if (result == S64_SETTINGS_COMMIT_STORE_ERROR) {
        set_status("SAVE FAILED - SETTINGS NOT WRITTEN");
        return;
    }
    if (result == S64_SETTINGS_COMMIT_EXCHANGE_ERROR) {
        set_status("Cannot preserve settings");
        return;
    }

    set_status("Saved SEE64YOU/SEE64YOU.CFG");
#else
    S64SettingsStoreResult result;
#ifdef S64_SETTINGS_SAVE_TRACE
    begin_save_trace(&dos, &settings);
#endif
    if (!validate_settings()) {
#ifdef S64_SETTINGS_SAVE_TRACE
        end_save_trace("UI VALIDATION", 0);
#endif
        return;
    }

    settings.state = S64_SETTINGS_STATE_READY;
    settings.result = S64_SETTINGS_RESULT_OK;
    if (capabilities.dos_target) {
        result = s64_settings_store_save(&dos, &settings);
        if (result != S64_SETTINGS_STORE_OK) {
            settings.result = S64_SETTINGS_RESULT_IO_ERROR;
            set_status(result == S64_SETTINGS_STORE_VERIFY_ERROR
                ? "SAVE FAILED - READBACK INVALID"
                : "SAVE FAILED - SETTINGS NOT WRITTEN");
#ifdef S64_SETTINGS_SAVE_TRACE
            end_save_trace("STORE SAVE", (uint8_t)result);
#endif
            return;
        }
    } else {
        settings.result = S64_SETTINGS_RESULT_IO_ERROR;
        set_status("SAVE FAILED - DOS UNAVAILABLE");
#ifdef S64_SETTINGS_SAVE_TRACE
        end_save_trace("DOS AVAILABLE", 0);
#endif
        return;
    }
#if defined(S64_INDEPENDENT_SETTINGS)
    /* Verified disk settings are authoritative; no volatile REU exchange. */
#elif defined(S64_MANAGED_OVERLAY)
    if (!s64_settings_session_publish(
            &managed_runtime.reu_control,
            &managed_runtime.lifecycle.journal,
            &settings
        )) {
        set_status("Cannot publish settings session");
        return;
    }
#else
    if (!s64_settings_exchange_store(&settings)) {
        set_status("Cannot preserve settings");
#ifdef S64_SETTINGS_SAVE_TRACE
        end_save_trace("POST-SAVE REU STORE", 0);
#endif
        return;
    }
#endif

    set_status("Saved SEE64YOU/SEE64YOU.CFG");
#ifdef S64_INDEPENDENT_SETTINGS
    if (setup_step) {
        clrscr();
        cputs("SETTINGS SAVED AND VERIFIED\r\n\r\nLaunch SEE64YOU.PRG to write and save.\r\nThen S64PUB.PRG to connect/publish.\r\nConnection has not been tested yet.\r\n\r\nRETURN: EXIT TO BASIC");
        while ((unsigned char)cgetc() != CH_ENTER) { }
    }
#endif
#ifdef S64_SETTINGS_SAVE_TRACE
    end_save_trace("SAVE COMPLETE", 0);
#else
    return_to_publisher();
#endif
#endif
}

static void cancel_and_return(void)
{
#ifdef S64_PERSISTENT_RUNTIME_FIT
    if (s64_persistent_settings_restore_original(&settings)
            != S64_SETTINGS_COMMIT_OK) {
        set_status("Cannot restore original settings");
    } else {
        set_status("Settings changes cancelled");
    }
#else
#ifdef S64_PERSISTENT_RUNTIME_FIT
    /* unreachable: kept out of the persistent root above */
#else
    settings = original_settings;
#endif
    settings.state = S64_SETTINGS_STATE_READY;
#ifdef S64_INDEPENDENT_SETTINGS
    /* Cancel does not write the configuration file. */
#elif defined(S64_MANAGED_OVERLAY)
    if (!s64_settings_session_publish(
            &managed_runtime.reu_control,
            &managed_runtime.lifecycle.journal,
            &settings
        )) {
        set_status("Cannot restore settings session");
        return;
    }
#else
    if (!s64_settings_exchange_store(&settings)) {
        set_status("Cannot preserve settings");
        return;
    }
#endif
    return_to_publisher();
#endif
}

#if !defined(S64_MANAGED_OVERLAY) && !defined(S64_PERSISTENT_RUNTIME_FIT) && !defined(S64_INDEPENDENT_SETTINGS)
static void service_load_request(void)
{
    S64SettingsStoreResult result;

    s64_settings_exchange_init(&settings);
    if (!capabilities.dos_target) {
        settings.result = S64_SETTINGS_RESULT_IO_ERROR;
    } else {
        result = s64_settings_store_load(&dos, &settings);
        if (result == S64_SETTINGS_STORE_NOT_FOUND) {
            s64_settings_exchange_init(&settings);
            settings.result = S64_SETTINGS_RESULT_NOT_FOUND;
        } else if (result == S64_SETTINGS_STORE_INVALID) {
            s64_settings_exchange_init(&settings);
            settings.result = S64_SETTINGS_RESULT_INVALID_FILE;
        } else if (result != S64_SETTINGS_STORE_OK) {
            s64_settings_exchange_init(&settings);
            settings.result = S64_SETTINGS_RESULT_IO_ERROR;
        } else if (!validate_settings()) {
            s64_settings_exchange_init(&settings);
            settings.result = S64_SETTINGS_RESULT_INVALID_FILE;
        } else {
            settings.result = S64_SETTINGS_RESULT_OK;
        }
    }
    settings.state = S64_SETTINGS_STATE_READY;
    if (!s64_settings_exchange_store(&settings)) {
        clrscr();
        cputs("SETTINGS HANDOFF FAILED\r\n");
        return;
    }
    return_to_publisher();
}

#endif

int main(void)
{
    unsigned char key;
#if !defined(S64_MANAGED_OVERLAY) && !defined(S64_INDEPENDENT_SETTINGS)
    unsigned char have_exchange;
#endif

#ifdef S64_INDEPENDENT_SETTINGS
    original_background = bgcolor(COLOR_BLUE);
    original_border = bordercolor(COLOR_BLUE);
    original_text = textcolor(COLOR_LIGHTBLUE);
#else
    bgcolor(COLOR_BLUE);
    bordercolor(COLOR_BLUE);
    textcolor(COLOR_LIGHTBLUE);
#endif
    clrscr();
    cursor(0);

#ifdef S64_MANAGED_OVERLAY
    if (!s64_module_transfer_consume_arrival(S64_MODULE_SETTINGS)) {
        managed_entry_failed(1u);
    }
    s64_dos_init(&dos, s64_uci_execute, S64_DOS_TARGET_PRIMARY);
    s64_module_runtime_init_hardware(&managed_runtime, &dos);
    if (!s64_module_runtime_adopt(
            &managed_runtime,
            S64_MODULE_SETTINGS,
            S64_PACKAGE_VERSION,
            1u
        )) {
        managed_entry_failed(2u);
    }
    if (!s64_settings_session_load(
            &managed_runtime.reu_control,
            &managed_runtime.lifecycle.journal,
            &settings
        )) {
        managed_entry_failed(3u);
    }
    if (s64_module_runtime_resume_interactive(&managed_runtime)
            != S64_SYSTEM_OK) {
        managed_entry_failed(4u);
    }
#else
    if (!s64_reu_probe() || s64_reu_probe_banks() < 2u) {
        cputs("SEE64YOU SETTINGS\r\n128K REU REQUIRED\r\n");
#ifdef S64_INDEPENDENT_SETTINGS
        bgcolor(original_background);
        bordercolor(original_border);
        textcolor(original_text);
        cursor(1);
#endif
        return 1;
    }

    s64_dos_init(&dos, s64_uci_execute, S64_DOS_TARGET_PRIMARY);
#ifdef S64_INDEPENDENT_SETTINGS
    s64_uci_probe_capabilities(&capabilities);
    if (capabilities.dos_target) {
        original_path_valid = s64_dos_get_path(&dos, original_path,
            sizeof(original_path)) == S64_DOS_OK;
        if (!original_path_valid) capabilities.dos_target = 0u;
    }
    s64_settings_exchange_init(&settings);
    if (capabilities.dos_target) {
        S64SettingsStoreResult result = s64_settings_publish_store_load(&dos, &settings);
        if (result != S64_SETTINGS_STORE_OK || !validate_settings()) {
            s64_settings_exchange_init(&settings);
        }
    }
    settings.state = S64_SETTINGS_STATE_EDIT_REQUEST;
#else
    have_exchange = s64_settings_exchange_load(&settings) ? 1u : 0u;
#ifndef S64_PERSISTENT_RUNTIME_FIT
    if (have_exchange && settings.state == S64_SETTINGS_STATE_LOAD_REQUEST) {
        service_load_request();
    }
#endif

    if (!have_exchange) {
        s64_settings_exchange_init(&settings);
        if (capabilities.dos_target) {
            S64SettingsStoreResult result = s64_settings_store_load(&dos, &settings);
            if (result != S64_SETTINGS_STORE_OK || !validate_settings()) {
                s64_settings_exchange_init(&settings);
            }
        }
        settings.state = S64_SETTINGS_STATE_EDIT_REQUEST;
    }

    if (settings.state != S64_SETTINGS_STATE_EDIT_REQUEST
        && settings.state != S64_SETTINGS_STATE_READY) {
        s64_settings_exchange_init(&settings);
        settings.state = S64_SETTINGS_STATE_EDIT_REQUEST;
    }
#endif
#endif

#ifndef S64_INDEPENDENT_SETTINGS
    s64_uci_probe_capabilities(&capabilities);
#endif
#ifndef S64_PERSISTENT_RUNTIME_FIT
    original_settings = settings;
#endif
    advanced_view = 0u;
    status_text[0] = '\0';
#ifdef S64_INDEPENDENT_SETTINGS
    if (!original_path_valid) set_status("DOS path not captured; storage blocked");
    setup_step = 0u;
#endif
    for (;;) {
        render();
        key = (unsigned char)cgetc();

#ifdef S64_INDEPENDENT_SETTINGS
        if (key == 'i' || key == 'I') import_direct_key();
        else if (key == 'h' || key == 'H') toggle_plain_http();
        else if (key == 't' || key == 'T') check_settings();
        else if (key == 'd' || key == 'D') {
            s64_settings_exchange_init(&settings);
            set_status("Publishing disabled; S to save");
        } else if (key == 's' || key == 'S') save_and_return();
        else if (key == CH_STOP) cancel_and_return();
        continue;
#else
        if (!advanced_view) {
            if (key == '1') {
                edit_site_url();
            } else if (key == '2') {
                import_direct_key();
            } else if (key == '3') {
                replace_direct_key();
            } else if (key == '4') {
                settings.direct_key[0] = '\0';
                set_status("Publishing key disabled");
            } else if (key == 't' || key == 'T') {
                check_settings();
            } else if (key == 'a' || key == 'A') {
                advanced_view = 1u;
                set_status("Advanced settings");
#ifdef S64_INDEPENDENT_SETTINGS
            } else if (key == 'w' || key == 'W') {
                setup_step = 1u;
                status_text[0] = '\0';
#endif
            } else if (key == 's' || key == 'S') {
                save_and_return();
            } else if (key == CH_STOP) {
                cancel_and_return();
            }
            continue;
        }

        if (key == '1') {
            choose_destination();
        } else if (key == '2') {
            edit_site_url();
        } else if (key == '3') {
            toggle_plain_http();
        } else if (key == '4') {
            import_direct_key();
        } else if (key == 'a' || key == 'A') {
            replace_direct_key();
        } else if (key == '5') {
            settings.direct_key[0] = '\0';
            set_status("Direct key disabled");
        } else if (key == '6') {
            relay_replacement[0] = '\0';
            if (edit_text(
                    "RELAY KEY: ",
                    relay_replacement,
                    (unsigned char)sizeof(relay_replacement),
                    1u
                )) {
                if (relay_replacement[0] == '\0') {
                    set_status("Relay key unchanged");
                } else {
                    memcpy(settings.relay_key, relay_replacement, sizeof(relay_replacement));
                    if (!s64_settings_exchange_values_valid(&settings)) {
                        settings.relay_key[0] = '\0';
                        set_status("Relay key must be 64 hex chars");
                    } else {
                        set_status("Relay key replaced");
                    }
                }
            }
            memset(relay_replacement, 0, sizeof(relay_replacement));
        } else if (key == '7') {
            settings.relay_key[0] = '\0';
            set_status("Relay key disabled");
        } else if (key == '8') {
            load_profile();
        } else if (key == '9') {
            save_profile();
        } else if (key == 'd' || key == 'D') {
            show_system_diagnostics();
            set_status("Diagnostics complete");
        } else if (key == 'b' || key == 'B') {
            advanced_view = 0u;
            set_status("Basic settings");
        } else if (key == 's' || key == 'S') {
            save_and_return();
        } else if (key == CH_STOP) {
            cancel_and_return();
        }
#endif
    }
    return 0;
}
