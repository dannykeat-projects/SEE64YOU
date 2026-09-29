#include "publisher/settings_store.h"
#include "charset.h"

#include <stddef.h>
#include <string.h>

#define S64_SETTINGS_TEMP_FILENAME "SEE64YOU.CFG.TMP"

typedef struct {
    S64SettingsExchange* settings;
    uint16_t offset;
    uint8_t overflow;
} S64SettingsRead;

#define S64_SETTINGS_V1_VERSION 1u

typedef struct {
    char key[65];
    uint8_t count;
    uint8_t invalid;
} S64DirectKeyRead;

static uint8_t direct_key_read(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64DirectKeyRead* read = (S64DirectKeyRead*)context;
    uint16_t i;

    for (i = 0u; i < length; ++i) {
        uint8_t ascii = data[i];
        uint8_t native;

        if (read->count < 64u) {
            if (!((ascii >= 0x30u && ascii <= 0x39u)
                || (ascii >= 0x41u && ascii <= 0x46u)
                || (ascii >= 0x61u && ascii <= 0x66u))
                || !s64_charset_ascii_to_native(ascii, &native)) {
                read->invalid = 1u;
                return 0u;
            }
            read->key[read->count++] = (char)native;
            continue;
        }

        if (ascii != 0x09u && ascii != 0x0au
            && ascii != 0x0du && ascii != 0x20u) {
            read->invalid = 1u;
            return 0u;
        }
    }
    return 1u;
}
#define S64_SETTINGS_V1_CHECKSUM_OFFSET ((uint16_t)offsetof(S64SettingsExchange, direct_key))
#define S64_SETTINGS_V1_SIZE (S64_SETTINGS_V1_CHECKSUM_OFFSET + 2u)

static uint16_t legacy_checksum(const S64SettingsExchange* settings)
{
    const uint8_t* bytes = (const uint8_t*)settings;
    uint8_t sum1 = 0x53u;
    uint8_t sum2 = 0x36u;
    uint16_t i;

    for (i = 0u; i < S64_SETTINGS_V1_CHECKSUM_OFFSET; ++i) {
        sum1 = (uint8_t)(sum1 + bytes[i]);
        sum2 = (uint8_t)(sum2 + sum1);
    }
    return (uint16_t)(((uint16_t)sum2 << 8) | sum1);
}

static uint8_t migrate_legacy_settings(
    S64SettingsExchange* settings,
    uint16_t bytes_read
)
{
    uint8_t* bytes = (uint8_t*)settings;
    uint16_t stored_checksum;

    if (settings == 0
        || bytes_read != S64_SETTINGS_V1_SIZE
        || settings->version != S64_SETTINGS_V1_VERSION) {
        return 0u;
    }

    stored_checksum = (uint16_t)(
        bytes[S64_SETTINGS_V1_CHECKSUM_OFFSET]
        | ((uint16_t)bytes[S64_SETTINGS_V1_CHECKSUM_OFFSET + 1u] << 8)
    );
    if (stored_checksum != legacy_checksum(settings)) {
        return 0u;
    }

    memset(settings->direct_key, 0, sizeof(settings->direct_key));
    settings->version = S64_SETTINGS_VERSION;
    settings->checksum = 0u;
    return s64_settings_exchange_values_valid(settings) ? 1u : 0u;
}

static uint8_t read_settings(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64SettingsRead* read = (S64SettingsRead*)context;

    if ((uint32_t)read->offset + length > sizeof(*read->settings)) {
        read->overflow = 1u;
        return 0u;
    }
    memcpy((uint8_t*)read->settings + read->offset, data, length);
    read->offset = (uint16_t)(read->offset + length);
    return 1u;
}

static uint8_t profile_filename(uint8_t slot, char* filename)
{
    static const char template_name[] = "S64DEST1.CFG";

    if (filename == 0 || slot >= S64_SETTINGS_PROFILE_COUNT) return 0u;
    memcpy(filename, template_name, sizeof(template_name));
    filename[7] = (char)('1' + slot);
    return 1u;
}

static S64SettingsStoreResult load_named(
    S64Dos* dos,
    const char* filename,
    S64SettingsExchange* settings
)
{
    S64DosResult result;
    S64DosResult close_result;
    S64SettingsRead read;
    uint8_t exists;
    uint16_t bytes_read;

    if (dos == 0 || filename == 0 || settings == 0) {
        return S64_SETTINGS_STORE_INVALID;
    }

    result = s64_dos_file_exists(dos, filename, &exists);
    if (result != S64_DOS_OK) return S64_SETTINGS_STORE_IO_ERROR;
    if (!exists) return S64_SETTINGS_STORE_NOT_FOUND;

    result = s64_dos_open_read(dos, filename);
    if (result != S64_DOS_OK) return S64_SETTINGS_STORE_IO_ERROR;

    memset(settings, 0, sizeof(*settings));
    read.settings = settings;
    read.offset = 0u;
    read.overflow = 0u;
    bytes_read = 0u;
    result = s64_dos_read(
        dos,
        (uint16_t)sizeof(*settings),
        read_settings,
        &read,
        &bytes_read
    );
    close_result = s64_dos_close(dos);

    if (result != S64_DOS_OK || close_result != S64_DOS_OK) {
        return S64_SETTINGS_STORE_IO_ERROR;
    }
    if (read.overflow || read.offset != bytes_read) {
        return S64_SETTINGS_STORE_INVALID;
    }
    if (bytes_read == (uint16_t)sizeof(*settings)) {
        if (!s64_settings_exchange_validate(settings)
            || settings->state != S64_SETTINGS_STATE_READY) {
            return S64_SETTINGS_STORE_INVALID;
        }
        return S64_SETTINGS_STORE_OK;
    }
    if (migrate_legacy_settings(settings, bytes_read)
        && settings->state == S64_SETTINGS_STATE_READY) {
        return S64_SETTINGS_STORE_OK;
    }
    return S64_SETTINGS_STORE_INVALID;
}

static S64SettingsStoreResult save_named(
    S64Dos* dos,
    const char* filename,
    S64SettingsExchange* settings
)
{
    S64DosResult result;
    S64DosResult close_result;
    uint8_t exists;

    if (dos == 0 || filename == 0 || settings == 0) {
        return S64_SETTINGS_STORE_INVALID;
    }

    settings->state = S64_SETTINGS_STATE_READY;
    settings->result = S64_SETTINGS_RESULT_OK;
    if (!s64_settings_exchange_store(settings)) {
        return S64_SETTINGS_STORE_INVALID;
    }

    result = s64_dos_open_write(dos, S64_SETTINGS_TEMP_FILENAME);
    if (result != S64_DOS_OK) return S64_SETTINGS_STORE_IO_ERROR;

    result = s64_dos_write(
        dos,
        (const uint8_t*)settings,
        (uint16_t)sizeof(*settings)
    );
    close_result = s64_dos_close(dos);
    if (result != S64_DOS_OK || close_result != S64_DOS_OK) {
        (void)s64_dos_delete_file(dos, S64_SETTINGS_TEMP_FILENAME);
        return S64_SETTINGS_STORE_IO_ERROR;
    }

    result = s64_dos_file_exists(dos, filename, &exists);
    if (result != S64_DOS_OK) {
        (void)s64_dos_delete_file(dos, S64_SETTINGS_TEMP_FILENAME);
        return S64_SETTINGS_STORE_IO_ERROR;
    }
    if (exists) {
        result = s64_dos_delete_file(dos, filename);
        if (result != S64_DOS_OK) {
            (void)s64_dos_delete_file(dos, S64_SETTINGS_TEMP_FILENAME);
            return S64_SETTINGS_STORE_IO_ERROR;
        }
    }

    result = s64_dos_rename_file(
        dos,
        S64_SETTINGS_TEMP_FILENAME,
        filename
    );
    if (result != S64_DOS_OK) {
        (void)s64_dos_delete_file(dos, S64_SETTINGS_TEMP_FILENAME);
        return S64_SETTINGS_STORE_IO_ERROR;
    }
    return S64_SETTINGS_STORE_OK;
}

S64SettingsStoreResult s64_settings_store_load(
    S64Dos* dos,
    S64SettingsExchange* settings
)
{
    return load_named(dos, S64_SETTINGS_FILENAME, settings);
}

S64SettingsStoreResult s64_settings_store_save(
    S64Dos* dos,
    S64SettingsExchange* settings
)
{
    return save_named(dos, S64_SETTINGS_FILENAME, settings);
}

S64SettingsStoreResult s64_settings_profile_load(
    S64Dos* dos,
    uint8_t slot,
    S64SettingsExchange* settings
)
{
    char filename[13];

    if (!profile_filename(slot, filename)) {
        return S64_SETTINGS_STORE_INVALID;
    }
    return load_named(dos, filename, settings);
}

S64SettingsStoreResult s64_settings_profile_save(
    S64Dos* dos,
    uint8_t slot,
    S64SettingsExchange* settings
)
{
    char filename[13];

    if (!profile_filename(slot, filename)) {
        return S64_SETTINGS_STORE_INVALID;
    }
    return save_named(dos, filename, settings);
}

S64SettingsStoreResult s64_settings_import_direct_key(
    S64Dos* dos,
    char direct_key[65]
)
{
    S64DosResult result;
    S64DosResult close_result;
    S64DirectKeyRead read;
    uint16_t bytes_read = 0u;

    if (dos == 0 || direct_key == 0) {
        return S64_SETTINGS_STORE_INVALID;
    }

    /*
     * Keep this path deliberately simple for stock C64U firmware. The file is
     * exactly 64 bytes in the normal case, so FILE_STAT, over-reading past
     * EOF, and multi-read EOF probing only add firmware-dependent behavior.
     */
    result = s64_dos_open_read(dos, S64_DIRECT_KEY_FILENAME);
    if (result == S64_DOS_NOT_FOUND) return S64_SETTINGS_STORE_NOT_FOUND;
    if (result != S64_DOS_OK) return S64_SETTINGS_STORE_IO_ERROR;

    memset(&read, 0, sizeof(read));
    result = s64_dos_read(
        dos,
        64u,
        direct_key_read,
        &read,
        &bytes_read
    );
    close_result = s64_dos_close(dos);

    if (read.invalid) {
        memset(&read, 0, sizeof(read));
        return S64_SETTINGS_STORE_INVALID;
    }
    if (result != S64_DOS_OK || close_result != S64_DOS_OK) {
        memset(&read, 0, sizeof(read));
        return S64_SETTINGS_STORE_IO_ERROR;
    }
    if (read.count != 64u || bytes_read < 64u) {
        memset(&read, 0, sizeof(read));
        return S64_SETTINGS_STORE_INVALID;
    }

    read.key[64] = '\0';
    memcpy(direct_key, read.key, sizeof(read.key));
    memset(&read, 0, sizeof(read));
    return S64_SETTINGS_STORE_OK;
}
