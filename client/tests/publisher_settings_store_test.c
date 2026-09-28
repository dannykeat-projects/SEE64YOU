#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/reu_layout.h"
#include "publisher/settings_store.h"

static unsigned char reu_settings[S64_REU_SETTINGS_CAPACITY];
static unsigned char stored[512];
static unsigned char temporary[512];
static uint16_t stored_length;
static uint16_t temporary_length;
static uint16_t read_offset;
static uint8_t read_open;
static uint8_t write_open;
static uint8_t fail_write;

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

uint8_t s64_reu_store(uint8_t bank, uint16_t address, const void* source, uint16_t length)
{
    if (bank != S64_REU_SETTINGS_BANK
        || address < S64_REU_SETTINGS_BASE
        || (uint32_t)(address - S64_REU_SETTINGS_BASE) + length > sizeof(reu_settings)) {
        return 0u;
    }
    memcpy(reu_settings + (address - S64_REU_SETTINGS_BASE), source, length);
    return 1u;
}

uint8_t s64_reu_load(uint8_t bank, uint16_t address, void* destination, uint16_t length)
{
    if (bank != S64_REU_SETTINGS_BANK
        || address < S64_REU_SETTINGS_BASE
        || (uint32_t)(address - S64_REU_SETTINGS_BASE) + length > sizeof(reu_settings)) {
        return 0u;
    }
    memcpy(destination, reu_settings + (address - S64_REU_SETTINGS_BASE), length);
    return 1u;
}

S64DosResult s64_dos_file_exists(S64Dos* dos, const char* filename, uint8_t* exists)
{
    (void)dos;
    if (exists == 0) return S64_DOS_INVALID_ARGUMENT;
    if (strcmp(filename, S64_SETTINGS_FILENAME) == 0) {
        *exists = stored_length != 0u;
    } else if (strcmp(filename, "SEE64YOU.CFG.TMP") == 0) {
        *exists = temporary_length != 0u;
    } else {
        *exists = 0u;
    }
    return S64_DOS_OK;
}

S64DosResult s64_dos_open_read(S64Dos* dos, const char* filename)
{
    (void)dos;
    if (strcmp(filename, S64_SETTINGS_FILENAME) != 0 || stored_length == 0u) {
        return S64_DOS_NOT_FOUND;
    }
    read_offset = 0u;
    read_open = 1u;
    return S64_DOS_OK;
}

S64DosResult s64_dos_open_write(S64Dos* dos, const char* filename)
{
    (void)dos;
    if (strcmp(filename, "SEE64YOU.CFG.TMP") != 0) return S64_DOS_INVALID_ARGUMENT;
    temporary_length = 0u;
    write_open = 1u;
    return S64_DOS_OK;
}

S64DosResult s64_dos_read(
    S64Dos* dos,
    uint16_t requested,
    S64UciDataSink sink,
    void* context,
    uint16_t* bytes_read
)
{
    uint16_t remaining;
    uint16_t count;

    (void)dos;
    if (bytes_read != 0) *bytes_read = 0u;
    if (!read_open || sink == 0) return S64_DOS_NO_FILE_OPEN;

    remaining = (uint16_t)(stored_length - read_offset);
    count = remaining < requested ? remaining : requested;
    if (count != 0u && !sink(stored + read_offset, count, context)) {
        return S64_DOS_CONSUMER_ABORTED;
    }
    read_offset = (uint16_t)(read_offset + count);
    if (bytes_read != 0) *bytes_read = count;
    return S64_DOS_OK;
}

S64DosResult s64_dos_write(S64Dos* dos, const uint8_t* data, uint16_t length)
{
    (void)dos;
    if (!write_open) return S64_DOS_NO_FILE_OPEN;
    if (fail_write) return S64_DOS_TRANSPORT_ERROR;
    if ((uint32_t)temporary_length + length > sizeof(temporary)) return S64_DOS_TARGET_ERROR;
    memcpy(temporary + temporary_length, data, length);
    temporary_length = (uint16_t)(temporary_length + length);
    return S64_DOS_OK;
}

S64DosResult s64_dos_close(S64Dos* dos)
{
    (void)dos;
    read_open = 0u;
    write_open = 0u;
    return S64_DOS_OK;
}

S64DosResult s64_dos_delete_file(S64Dos* dos, const char* filename)
{
    (void)dos;
    if (strcmp(filename, S64_SETTINGS_FILENAME) == 0) {
        stored_length = 0u;
        return S64_DOS_OK;
    }
    if (strcmp(filename, "SEE64YOU.CFG.TMP") == 0) {
        temporary_length = 0u;
        return S64_DOS_OK;
    }
    return S64_DOS_NOT_FOUND;
}

S64DosResult s64_dos_rename_file(S64Dos* dos, const char* filename, const char* new_name)
{
    (void)dos;
    if (strcmp(filename, "SEE64YOU.CFG.TMP") != 0
        || strcmp(new_name, S64_SETTINGS_FILENAME) != 0
        || temporary_length == 0u) {
        return S64_DOS_INVALID_ARGUMENT;
    }
    memcpy(stored, temporary, temporary_length);
    stored_length = temporary_length;
    temporary_length = 0u;
    return S64_DOS_OK;
}

int main(void)
{
    S64Dos dos;
    S64SettingsExchange settings;
    S64SettingsExchange loaded;
    S64SettingsStoreResult result;
    const char* key =
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

    memset(&dos, 0, sizeof(dos));
    memset(reu_settings, 0, sizeof(reu_settings));
    stored_length = 0u;
    temporary_length = 0u;
    fail_write = 0u;

    s64_settings_exchange_init(&loaded);
    require(
        s64_settings_store_load(&dos, &loaded) == S64_SETTINGS_STORE_NOT_FOUND,
        "missing settings reported"
    );

    s64_settings_exchange_init(&settings);
    settings.destination_type = (uint8_t)S64_DESTINATION_STANDALONE;
    settings.allow_insecure_http = 1u;
    strcpy(settings.base_url, "http://relay.local:6464");
    strcpy(settings.relay_key, key);
    result = s64_settings_store_save(&dos, &settings);
    require(result == S64_SETTINGS_STORE_OK, "settings save succeeds");
    require(stored_length == sizeof(settings), "settings file size");

    memset(&loaded, 0, sizeof(loaded));
    result = s64_settings_store_load(&dos, &loaded);
    require(result == S64_SETTINGS_STORE_OK, "settings load succeeds");
    require(loaded.destination_type == (uint8_t)S64_DESTINATION_STANDALONE, "type round trip");
    require(loaded.allow_insecure_http == 1u, "http flag round trip");
    require(strcmp(loaded.base_url, "http://relay.local:6464") == 0, "url round trip");
    require(strcmp(loaded.relay_key, key) == 0, "relay key round trip");

    stored[12] ^= 0x20u;
    require(
        s64_settings_store_load(&dos, &loaded) == S64_SETTINGS_STORE_INVALID,
        "corrupt settings rejected"
    );
    stored[12] ^= 0x20u;

    fail_write = 1u;
    result = s64_settings_store_save(&dos, &settings);
    require(result == S64_SETTINGS_STORE_IO_ERROR, "write failure reported");
    require(stored_length == sizeof(settings), "old settings preserved");
    require(temporary_length == 0u, "failed temp cleaned up");

    puts("publisher settings store tests passed");
    return 0;
}
