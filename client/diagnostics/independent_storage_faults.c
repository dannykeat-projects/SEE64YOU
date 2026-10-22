#include <conio.h>
#include <stdint.h>
#include <string.h>
#include "buffer.h"
#include "document.h"
#include "platform/dos.h"
#include "platform/reu.h"
#include "platform/uci.h"
#include "publisher/settings_store.h"
#include "storage/local.h"

/* Diagnostic only: real SD writes/readback, selected transport failures.
 * Run only with absent S6RTEST files and profile 4, and backed-up config/temp. */
static S64Dos dos;
static S64Buffer body;
static S64Document document;
static S64LocalDocument local;
static S64SettingsExchange settings;
static uint8_t fail_rename, fail_read, rename_hits, read_hits;
static S64LocalResult save_result;

static S64UciResult fault_execute(const S64UciCommandPart* parts,
    uint8_t count, S64UciReply* reply)
{
    if (count && parts[0].length >= 2u && parts[0].data[0] == 1u) {
        if (fail_rename && parts[0].data[1] == 0x0au) {
            ++rename_hits;
            return S64_UCI_UNAVAILABLE;
        }
        if (fail_read && parts[0].data[1] == 0x04u) {
            ++read_hits;
            return S64_UCI_UNAVAILABLE;
        }
    }
    return s64_uci_execute(parts, count, reply);
}

#define CHECK(condition, stage) do { if (!(condition)) { \
    cprintf("FAIL %u\r\n", (unsigned)(stage)); return (stage); } } while (0)

int main(void)
{
    static const uint8_t prefix[] = {'R','e','c','o','v','e','r','y','.',' '};
    unsigned i;
    uint8_t exists;
    uint16_t original_length;
    clrscr();
    CHECK(s64_reu_probe_banks() == 256u, 1);
    s64_dos_init(&dos, fault_execute, 1u);
    CHECK(s64_dos_change_dir(&dos, "/sd/") == S64_DOS_OK, 2);
    s64_buffer_init(&body);
    s64_document_init(&document, &body);
    s64_local_init(&local, &dos, &document);
    CHECK(s64_local_open(&local, "S6T04.S64", 1u) == S64_LOCAL_OK, 3);
    original_length = s64_buffer_length(&body);
    CHECK(s64_dos_file_exists(&dos,"S6RTEST.S64.tmp",&exists)==S64_DOS_OK && !exists,22);
    CHECK(s64_local_save_as(&local, "S6RTEST.S64", 0u) == S64_LOCAL_OK, 4);
    CHECK(s64_buffer_move_cursor(&body, 0u), 5);
    for (i=0; i<sizeof(prefix); ++i) CHECK(s64_buffer_insert(&body, prefix[i]), 6);
    fail_rename = 1u;
    save_result = s64_local_save(&local);
    cprintf("SAVE %u DOS %u HITS %u\r\n", (unsigned)save_result,
        (unsigned)local.dos_result, (unsigned)rename_hits);
    CHECK(save_result == S64_LOCAL_IO_ERROR, 7);
    fail_rename = 0u;
    CHECK(rename_hits == 1u, 8);
    CHECK(s64_local_open(&local, "S6RTEST.S64.tmp", 1u) == S64_LOCAL_OK, 9);
    CHECK(s64_buffer_length(&body) == original_length + sizeof(prefix), 10);
    for (i=0; i<sizeof(prefix); ++i) CHECK(s64_buffer_char_at(&body,i)==prefix[i],11);
    cputs("DOCUMENT TEMP REOPEN PASSED\r\n");

    CHECK(s64_settings_store_load(&dos, &settings) == S64_SETTINGS_STORE_OK, 12);
    CHECK(s64_dos_change_dir(&dos,"/sd/SEE64YOU/")==S64_DOS_OK,19);
    CHECK(s64_dos_file_exists(&dos,"S64DEST4.CFG",&exists)==S64_DOS_OK && !exists,20);
    CHECK(s64_dos_file_exists(&dos,"SEE64YOU.CFG.TMP",&exists)==S64_DOS_OK && !exists,21);
    CHECK(s64_settings_profile_save(&dos, 3u, &settings) == S64_SETTINGS_STORE_OK,13);
    fail_read = 1u;
    CHECK(s64_settings_profile_save(&dos, 3u, &settings) == S64_SETTINGS_STORE_VERIFY_ERROR,14);
    fail_read = 0u;
    CHECK(read_hits == 1u,15);
    CHECK(s64_settings_profile_load(&dos,3u,&settings)==S64_SETTINGS_STORE_OK,16);
    cputs("CONFIG PRIOR READBACK PASSED\r\n");
    fail_rename = 1u;
    CHECK(s64_settings_profile_save(&dos,3u,&settings)==S64_SETTINGS_STORE_IO_ERROR,17);
    fail_rename = 0u;
    CHECK(rename_hits == 2u,18);
    cputs("STORAGE FAULT CHECKS PASSED\r\n");
    return 0;
}
