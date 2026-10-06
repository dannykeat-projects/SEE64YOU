#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#define main probe_main
#include "../diagnostics/settings_exchange_probe.c"
#undef main

static S64SettingsExchange reu_record, backup;
static S64SettingsExchange disk_record;
static uint16_t disk_length;
static uint8_t disk_exists, key_open;
static uint8_t stores, loads, saves, keys, corrupt_read, fail_write, fail_store;
static char selected = 'r', import_key;
static char output[8192];
static const char direct_secret[] =
    "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
static const char relay_secret[] =
    "abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789";

void clrscr(void) {}
void cputs(const char* text) { assert(strlen(output) + strlen(text) < sizeof(output)); strcat(output, text); }
int cprintf(const char* format, ...)
{
    va_list args;
    int count;
    size_t offset = strlen(output);
    va_start(args, format);
    count = vsnprintf(output + offset, sizeof(output) - offset, format, args);
    va_end(args);
    assert(count >= 0 && (size_t)count < sizeof(output) - offset);
    return count;
}
char cgetc(void)
{
    if (keys++ == 0) return selected;
    if (selected == 't' && keys == 2 && import_key) return 'k';
    return ' ';
}

uint8_t s64_reu_probe(void) { return 1; }
uint8_t s64_reu_store(uint8_t bank, uint16_t address, const void* source, uint16_t length)
{
    assert(bank == S64_REU_SETTINGS_BANK && address == S64_REU_SETTINGS_BASE);
    assert(length == sizeof(reu_record));
    ++stores;
    if (fail_store && stores == 1) return 0;
    memcpy(&reu_record, source, length);
    return 1;
}
uint8_t s64_reu_load(uint8_t bank, uint16_t address, void* destination, uint16_t length)
{
    assert(bank == S64_REU_SETTINGS_BANK && address == S64_REU_SETTINGS_BASE);
    assert(length == sizeof(reu_record));
    ++loads;
    memcpy(destination, &reu_record, length);
    if (corrupt_read && loads == 2) ((uint8_t*)destination)[0] ^= 1;
    return 1;
}

S64UciResult s64_uci_execute(const S64UciCommandPart* parts, uint8_t count, S64UciReply* reply)
{
    uint8_t opcode = parts[0].data[1];
    uint8_t i;
    const char* status_text = "00,OK";
    for (i = 1; i < count; ++i) {
        if (parts[i].flags & S64_UCI_PART_NATIVE_TEXT) {
            const char* name = (const char*)parts[i].data;
            uint16_t length = parts[i].length;
            if (length && name[length - 1] == 0) --length;
            assert((length == strlen(PROBE_TEMP) && memcmp(name, PROBE_TEMP, length) == 0)
                || (length == strlen(PROBE_FINAL) && memcmp(name, PROBE_FINAL, length) == 0)
                || (length == strlen(S64_DIRECT_KEY_FILENAME) && memcmp(name, S64_DIRECT_KEY_FILENAME, length) == 0)
                || (length == 4 && memcmp(name, "/sd/", 4) == 0));
        }
    }
    if (opcode == 2 && parts[0].data[2] != 1) {
        assert(parts[0].data[2] == 6); /* Actual production CREATE_NEW. */
        assert(stores == 1); /* Only the diagnostic's explicit REU round-trip. */
        ++saves;
        disk_length = 0;
    }
    if (opcode == 2 && parts[0].data[2] == 1)
        key_open = parts[1].length == strlen(S64_DIRECT_KEY_FILENAME)
            && memcmp(parts[1].data, S64_DIRECT_KEY_FILENAME, parts[1].length) == 0;
    if (opcode == 4) {
        uint16_t requested = parts[0].data[2] | ((uint16_t)parts[0].data[3] << 8);
        if (key_open) {
            assert(requested == 64);
            assert(reply->data_sink((const uint8_t*)direct_secret, 64, reply->data_context));
        } else {
            assert(requested == sizeof(disk_record) && disk_exists);
            assert(reply->data_sink((const uint8_t*)&disk_record, disk_length, reply->data_context));
        }
    }
    if (opcode == 5 && !fail_write) {
        assert(disk_length + parts[1].length <= sizeof(disk_record));
        memcpy((uint8_t*)&disk_record + disk_length, parts[1].data, parts[1].length);
        disk_length += parts[1].length;
    }
    if (opcode == 10) disk_exists = 1;
    if (opcode == 8 && !disk_exists) status_text = "82,FILE NOT FOUND";
    if (opcode == 5 && fail_write) status_text = "85,WRITE FAILED";
    strcpy(reply->status, status_text);
    reply->status_length = strlen(status_text);
    return S64_UCI_OK;
}

static void prepare(void)
{
    stores = loads = saves = keys = corrupt_read = fail_write = fail_store = 0;
    disk_length = disk_exists = key_open = 0;
    selected = 'r';
    import_key = 0;
    output[0] = 0;
    s64_settings_exchange_init(&reu_record);
    reu_record.destination_type = S64_DESTINATION_STANDALONE;
    strcpy(reu_record.base_url, "http://diagnostic.invalid");
    reu_record.allow_insecure_http = 1;
    strcpy(reu_record.direct_key, direct_secret);
    strcpy(reu_record.relay_key, relay_secret);
    reu_record.checksum = s64_settings_exchange_checksum(&reu_record);
    backup = reu_record;
}

static void verify_restore_and_secrecy(void)
{
    assert(memcmp(&backup, &reu_record, sizeof(backup)) == 0);
    assert(strstr(output, direct_secret) == 0 && strstr(output, relay_secret) == 0);
}

int main(void)
{
    prepare();
    assert(probe_main() == 0 && saves == 1);
    assert(strstr(output, "DIRECT KEY: VALID 64-HEX") != 0);
    assert(strstr(output, "RELAY KEY: VALID 64-HEX") != 0);
    verify_restore_and_secrecy();

    prepare();
    memset(reu_record.base_url, 'x', sizeof(reu_record.base_url));
    backup = reu_record;
    assert(probe_main() == 1 && saves == 0);
    assert(strstr(output, "TERMINATED: 0") != 0);
    verify_restore_and_secrecy();

    prepare();
    strcpy(reu_record.direct_key, "invalid");
    backup = reu_record;
    assert(probe_main() == 1 && saves == 0);
    assert(strstr(output, "DIRECT KEY: INVALID") != 0);
    verify_restore_and_secrecy();

    prepare(); corrupt_read = 1;
    assert(probe_main() == 1 && saves == 0);
    assert(strstr(output, "READBACK CHECKSUM/VALIDATION: FAIL") != 0);
    verify_restore_and_secrecy();

    prepare(); fail_store = 1;
    assert(probe_main() == 1 && saves == 0);
    verify_restore_and_secrecy();

    prepare(); fail_write = 1;
    assert(probe_main() == 1 && saves == 1);
    assert(strstr(output, "NORMAL STORE SAVE: FAIL") != 0);
    verify_restore_and_secrecy();

    prepare(); selected = 't';
    assert(probe_main() == 0 && saves == 1);
    assert(strstr(output, "DIRECT KEY: EMPTY") != 0);
    verify_restore_and_secrecy();

    prepare(); selected = 't'; import_key = 1;
    assert(probe_main() == 0 && saves == 1);
    assert(strstr(output, "DIRECT KEY: VALID 64-HEX") != 0);
    verify_restore_and_secrecy();
    puts("settings exchange probe tests passed");
    return 0;
}
