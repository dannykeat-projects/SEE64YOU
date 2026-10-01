#ifdef __CC65__
#include <conio.h>
#else
void clrscr(void);
void cputs(const char* text);
int cprintf(const char* format, ...);
char cgetc(void);
#endif
#include <string.h>
#include "platform/dos.h"
#include "platform/reu.h"
#include "platform/reu_layout.h"
#include "publisher/settings_store.h"
/* Reuse the exact private key/termination classifiers without exporting a
 * production API. This diagnostic is the only translation unit including it. */
#include "../src/publisher/settings_exchange.c"

#define PROBE_TEMP "S64EXCH.TMP"
#define PROBE_FINAL "S64EXCH.CFG"

static S64SettingsExchange settings, original, readback;
static S64Dos dos;
static uint8_t failed;

/* Run the normal save algorithm unchanged, redirecting only its two filenames.
 * Reply data and status are never printed: records can contain publishing keys. */
static S64UciResult diagnostic_execute(const S64UciCommandPart* parts,
    uint8_t count, S64UciReply* reply)
{
    S64UciCommandPart redirected[4];
    uint8_t i;
    uint8_t terminated;
    uint16_t name_length;
    const char* replacement;
    if (count > 4) return S64_UCI_PROTOCOL_ERROR;
    for (i = 0; i < count; ++i) {
        redirected[i] = parts[i];
        replacement = 0;
        name_length = parts[i].length;
        terminated = name_length != 0 && parts[i].data[name_length - 1] == 0;
        if (terminated) --name_length;
        if (parts[i].flags & S64_UCI_PART_NATIVE_TEXT) {
            if (name_length == sizeof("SEE64YOU.CFG.TMP") - 1
                && memcmp(parts[i].data, "SEE64YOU.CFG.TMP", name_length) == 0)
                replacement = PROBE_TEMP;
            else if (name_length == sizeof(S64_SETTINGS_FILENAME) - 1
                && memcmp(parts[i].data, S64_SETTINGS_FILENAME, name_length) == 0)
                replacement = PROBE_FINAL;
        }
        if (replacement != 0) {
            redirected[i].data = (const uint8_t*)replacement;
            redirected[i].length = strlen(replacement) + terminated;
        }
    }
    return s64_uci_execute(redirected, count, reply);
}

static const char* key_state(const char* key)
{
    if (key[0] == 0) return "EMPTY";
    return text_terminated(key, 65) && key_valid(key) ? "VALID 64-HEX" : "INVALID";
}

static void show_fields(void)
{
    cprintf("VALUES VALID: %u\r\nRECORD VALID: %u\r\n",
        s64_settings_exchange_values_valid(&settings),
        s64_settings_exchange_validate(&settings));
    cprintf("DESTINATION TYPE: %u\r\nURL PRESENT: %u TERMINATED: %u\r\n",
        settings.destination_type, settings.base_url[0] != 0,
        text_terminated(settings.base_url, sizeof(settings.base_url)));
    cprintf("PLAIN HTTP: %u (VALID 0/1: %u)\r\n",
        settings.allow_insecure_http, settings.allow_insecure_http <= 1);
    cprintf("DIRECT KEY: %s\r\nRELAY KEY: %s\r\n",
        key_state(settings.direct_key), key_state(settings.relay_key));
    cprintf("VERSION: %u STATE: %u RESULT: %u\r\n",
        settings.version, settings.state, settings.result);
}

static uint8_t check(const char* label, uint8_t okay)
{
    cprintf("%s: %s\r\n", label, okay ? "PASS" : "FAIL");
    if (!okay) failed = 1;
    return okay;
}

static void pause_page(void)
{
    cputs("PRESS KEY TO CONTINUE");
    cgetc();
    clrscr();
}

int main(void)
{
    char mode;
    uint8_t saved_original, stored, loaded, valid;
    S64SettingsStoreResult save_result;
    S64DosResult path_result;

    clrscr();
    cputs("SETTINGS EXCHANGE PROBE\r\n\r\nR = RETAINED REU SETTINGS\r\nT = KNOWN-GOOD STANDALONE TEST\r\nOTHER KEY EXITS\r\n\r\nWRITES S64EXCH.TMP/CFG ONLY.\r\nKEYS ARE NEVER DISPLAYED.\r\nREU SETTINGS RESTORED AT EXIT.\r\nTEST CFG MAY CONTAIN KEYS.\r\n");
    mode = cgetc();
    if (mode != 'r' && mode != 't') return 0;
    failed = 0;
    s64_dos_init(&dos, diagnostic_execute, S64_DOS_TARGET_PRIMARY);
    saved_original = s64_reu_load(S64_REU_SETTINGS_BANK, S64_REU_SETTINGS_BASE,
        &original, sizeof(original));
    if (!check("INITIAL REU READ", saved_original)) goto done;
    if (!check("REU AVAILABLE", s64_reu_probe())) goto done;
    if (mode == 'r') {
        settings = original;
        cputs("SOURCE: RETAINED REU RECORD\r\n");
        cputs("UNSAVED RAM EDITS NOT INCLUDED\r\n");
    } else {
        s64_settings_exchange_init(&settings);
        settings.destination_type = S64_DESTINATION_STANDALONE;
        strcpy(settings.base_url, "http://diagnostic.invalid");
        settings.allow_insecure_http = 1;
        /* UI edits invalidate the old checksum; production store rebuilds it. */
        settings.state = S64_SETTINGS_STATE_EDIT_REQUEST;
        settings.checksum = 0;
        cputs("SOURCE: KNOWN-GOOD TEST\r\nK = IMPORT SEE64KEY.TXT\r\nOTHER KEY USES EMPTY KEYS\r\n");
        if (cgetc() == 'k') {
            path_result = s64_dos_change_dir(&dos, "/sd/");
            if (!check("SD ROOT FOR IMPORT", path_result == S64_DOS_OK)) goto done;
            save_result = s64_settings_import_direct_key(&dos, settings.direct_key);
            cprintf("KEY IMPORT RESULT: %u\r\n", save_result);
            if (!check("KEY IMPORT", save_result == S64_SETTINGS_STORE_OK)) goto done;
        }
    }
    show_fields();
    pause_page();
    /* Match save_named's READY/OK transition, not a stale UI checksum. */
    settings.state = S64_SETTINGS_STATE_READY;
    settings.result = S64_SETTINGS_RESULT_OK;
    stored = s64_settings_exchange_store(&settings);
    check("EXCHANGE STORE", stored);
    memset(&readback, 0, sizeof(readback));
    loaded = s64_reu_load(S64_REU_SETTINGS_BANK, S64_REU_SETTINGS_BASE,
        &readback, sizeof(readback));
    check("IMMEDIATE REU READ", loaded);
    valid = loaded && s64_settings_exchange_validate(&readback);
    check("READBACK CHECKSUM/VALIDATION", valid);
    valid = valid && memcmp(&settings, &readback, sizeof(settings)) == 0;
    check("READBACK EXACT BYTES", valid);
    cprintf("READBACK STATE: %u RESULT: %u\r\n", readback.state, readback.result);
    pause_page();
    if (stored && valid) {
        path_result = s64_dos_change_dir(&dos, "/sd/");
        if (check("SD ROOT FOR SAVE", path_result == S64_DOS_OK)) {
            save_result = s64_settings_store_save(&dos, &settings);
            cprintf("NORMAL STORE SAVE RESULT: %u\r\n", save_result);
            check("NORMAL STORE SAVE", save_result == S64_SETTINGS_STORE_OK);
        }
    } else cputs("NORMAL STORE SAVE: SKIPPED\r\n");
    /* Restore only the settings exchange region; no REU sizing/destructive
     * bank probe and no changes to document/history/clipboard ownership. */
    stored = s64_reu_store(S64_REU_SETTINGS_BANK, S64_REU_SETTINGS_BASE,
        &original, sizeof(original));
    loaded = s64_reu_load(S64_REU_SETTINGS_BANK, S64_REU_SETTINGS_BASE,
        &readback, sizeof(readback));
    check("RESTORE ORIGINAL REU", stored && loaded
        && memcmp(&original, &readback, sizeof(original)) == 0);
done:
    cputs(failed ? "EXCHANGE PROBE FAIL\r\n" : "EXCHANGE PROBE PASS\r\n");
    cputs("RESTART NORMAL APP BEFORE USE.\r\nPRESS KEY TO EXIT");
    cgetc();
    return failed != 0;
}
