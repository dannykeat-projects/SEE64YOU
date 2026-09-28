#include "publisher/settings_store.h"

#include <string.h>

#define S64_SETTINGS_TEMP_FILENAME "SEE64YOU.CFG.TMP"

typedef struct {
    S64SettingsExchange* settings;
    uint16_t offset;
    uint8_t overflow;
} S64SettingsRead;

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
    if (read.overflow
        || bytes_read != (uint16_t)sizeof(*settings)
        || read.offset != (uint16_t)sizeof(*settings)
        || !s64_settings_exchange_validate(settings)
        || settings->state != S64_SETTINGS_STATE_READY) {
        return S64_SETTINGS_STORE_INVALID;
    }
    return S64_SETTINGS_STORE_OK;
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
