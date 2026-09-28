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

S64SettingsStoreResult s64_settings_store_load(
    S64Dos* dos,
    S64SettingsExchange* settings
)
{
    S64DosResult result;
    S64DosResult close_result;
    S64SettingsRead read;
    uint8_t exists;
    uint16_t bytes_read;

    if (dos == 0 || settings == 0) return S64_SETTINGS_STORE_INVALID;

    result = s64_dos_file_exists(dos, S64_SETTINGS_FILENAME, &exists);
    if (result != S64_DOS_OK) return S64_SETTINGS_STORE_IO_ERROR;
    if (!exists) return S64_SETTINGS_STORE_NOT_FOUND;

    result = s64_dos_open_read(dos, S64_SETTINGS_FILENAME);
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

S64SettingsStoreResult s64_settings_store_save(
    S64Dos* dos,
    S64SettingsExchange* settings
)
{
    S64DosResult result;
    S64DosResult close_result;
    uint8_t exists;

    if (dos == 0 || settings == 0) return S64_SETTINGS_STORE_INVALID;

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

    result = s64_dos_file_exists(dos, S64_SETTINGS_FILENAME, &exists);
    if (result != S64_DOS_OK) {
        (void)s64_dos_delete_file(dos, S64_SETTINGS_TEMP_FILENAME);
        return S64_SETTINGS_STORE_IO_ERROR;
    }
    if (exists) {
        result = s64_dos_delete_file(dos, S64_SETTINGS_FILENAME);
        if (result != S64_DOS_OK) {
            (void)s64_dos_delete_file(dos, S64_SETTINGS_TEMP_FILENAME);
            return S64_SETTINGS_STORE_IO_ERROR;
        }
    }

    result = s64_dos_rename_file(
        dos,
        S64_SETTINGS_TEMP_FILENAME,
        S64_SETTINGS_FILENAME
    );
    if (result != S64_DOS_OK) {
        (void)s64_dos_delete_file(dos, S64_SETTINGS_TEMP_FILENAME);
        return S64_SETTINGS_STORE_IO_ERROR;
    }
    return S64_SETTINGS_STORE_OK;
}
