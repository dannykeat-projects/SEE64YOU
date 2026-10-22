/* Reuse the actual helper's path/status capture and UI validation. This probe
 * never saves, writes REU settings, or displays URLs/key payloads. */
#define S64_SETTINGS_SAVE_TRACE 1
#define main settings_helper_main
#include "../src/publisher/settings_main.c"
#undef main

static char original_path[80];
static uint8_t terminate_stat;

/* Controlled diagnostic comparison: firmware FILE_STAT consumes a C string,
 * unlike OPEN_FILE it does not append a terminator itself. */
static S64UciResult load_execute(const S64UciCommandPart* parts,
    uint8_t count, S64UciReply* reply)
{
    S64UciCommandPart terminated[3];
    static const uint8_t zero = 0;
    if (terminate_stat && count == 2 && parts[0].length >= 2
        && parts[0].data[1] == 8) {
        terminated[0] = parts[0];
        terminated[1] = parts[1];
        terminated[2].data = &zero;
        terminated[2].length = 1;
        terminated[2].flags = 0;
        return save_trace_execute(terminated, 3, reply);
    }
    return save_trace_execute(parts, count, reply);
}

static void inspect_load(const char* label)
{
    S64SettingsStoreResult result;
    uint8_t i;
    begin_save_trace(&dos, &settings);
    dos.execute = load_execute;
    result = s64_settings_store_load(&dos, &settings);
    dos.execute = s64_uci_execute;
    clrscr();
    cprintf("SETTINGS LOAD PROBE: %s\r\n", label);
    cprintf("PATH: %s\r\nPATH UCI: %u %s\r\n", save_trace_path,
        save_trace_path_result, save_trace_path_status);
    cprintf("DOS AVAILABLE: %u STORE RESULT: %u\r\n", capabilities.dos_target, result);
    cprintf("VALUES: %u RECORD: %u UI: %u\r\n",
        s64_settings_exchange_values_valid(&settings),
        s64_settings_exchange_validate(&settings), validate_settings());
    cprintf("TYPE: %u HTTP: %u STATE: %u\r\n", settings.destination_type,
        settings.allow_insecure_http, settings.state);
    for (i = 0; i < save_trace_count; ++i) {
        cprintf("$%02X UCI %u %s\r\n", save_trace[i].opcode,
            save_trace[i].transport, save_trace[i].status);
    }
    cputs("STORE 0=OK 1=MISSING 2=INVALID 3=IO\r\n");
}

int main(void)
{
    uint8_t retained;
    S64DosResult changed;
    bgcolor(COLOR_BLUE);
    bordercolor(COLOR_BLUE);
    textcolor(COLOR_LIGHTBLUE);
    clrscr();
    cursor(0);
    s64_uci_probe_capabilities(&capabilities);
    s64_dos_init(&dos, s64_uci_execute, S64_DOS_TARGET_PRIMARY);
    retained = s64_settings_exchange_load(&settings);
    begin_save_trace(&dos, &settings);
    dos.execute = s64_uci_execute;
    if (save_trace_path_result != S64_UCI_OK || save_trace_path_truncated
        || save_trace_path[0] == 0) {
        cputs("CANNOT CAPTURE PATH; NO CHANGE MADE");
        for (;;) cgetc();
    }
    strcpy(original_path, save_trace_path);
    inspect_load("ORIGINAL PATH");
    cprintf("RETAINED REU VALID: %u\r\n", retained);
    cputs("PRESS KEY FOR /SD/ CONTROL");
    cgetc();
    changed = s64_dos_change_dir(&dos, "/sd/");
    inspect_load("EXPLICIT /SD/");
    cprintf("CHANGE DIR RESULT: %u\r\n", changed);
    cputs("PRESS KEY FOR NUL-STAT CONTROL");
    cgetc();
    terminate_stat = 1;
    inspect_load("NUL-STAT CONTROL");
    cprintf("RESTORE PATH RESULT: %u\r\n", s64_dos_change_dir(&dos, original_path));
    cputs("READ ONLY; CFG AND REU UNCHANGED");
    for (;;) cgetc();
    return 0;
}
