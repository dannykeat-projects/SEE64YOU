#include "publisher/settings_store.h"
#ifdef S64_PERSISTENT_RUNTIME_FIT
#include "publisher/workspace.h"
#endif
#include "charset.h"
#include "platform/package_paths.h"

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

static S64SettingsStoreResult select_storage_path(
    S64Dos* dos,
    const char* path
)
{
    if (dos == 0 || path == 0) return S64_SETTINGS_STORE_INVALID;
    return s64_dos_change_dir(dos, path) == S64_DOS_OK
        ? S64_SETTINGS_STORE_OK
        : S64_SETTINGS_STORE_IO_ERROR;
}

static S64SettingsStoreResult load_named(
    S64Dos* dos,
    const char* path,
    const char* filename,
    S64SettingsExchange* settings
)
{
    S64DosResult result;
    S64DosResult close_result;
    S64SettingsRead read;
    uint8_t exists;
    uint16_t bytes_read;

    if (dos == 0 || path == 0 || filename == 0 || settings == 0) {
        return S64_SETTINGS_STORE_INVALID;
    }
    if (select_storage_path(dos, path) != S64_SETTINGS_STORE_OK) {
        return S64_SETTINGS_STORE_IO_ERROR;
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

static S64DosResult open_create_new(
    S64Dos* dos,
    const char* filename
)
{
    uint8_t prefix[3];
    uint16_t length = 0u;
    S64UciCommandPart parts[2];
    S64UciReply reply;
    S64UciResult transport;
    char status[32];

    if (dos == 0 || dos->execute == 0 || filename == 0 || filename[0] == '\0') {
        return S64_DOS_INVALID_ARGUMENT;
    }
    while (filename[length] != '\0') {
        if (filename[length] == '\r' || filename[length] == '\n') {
            return S64_DOS_INVALID_ARGUMENT;
        }
        ++length;
    }

    prefix[0] = dos->target;
    prefix[1] = 0x02u; /* OPEN_FILE */
    prefix[2] = 0x06u; /* WRITE | CREATE_NEW */

    parts[0].data = prefix;
    parts[0].length = 3u;
    parts[0].flags = 0u;
    parts[1].data = (const uint8_t*)filename;
    parts[1].length = length;
    parts[1].flags = S64_UCI_PART_NATIVE_TEXT;

    reply.data_sink = 0;
    reply.data_context = 0;
    reply.status = status;
    reply.status_capacity = (uint16_t)sizeof(status);
    reply.status_length = 0u;
    status[0] = '\0';

    transport = dos->execute(parts, 2u, &reply);
    if (transport != S64_UCI_OK) return S64_DOS_TRANSPORT_ERROR;
    if (reply.status_length == 0u) return S64_DOS_OK;
    if (reply.status_length >= 2u && status[0] == '0' && status[1] == '0') {
        return S64_DOS_OK;
    }
    return S64_DOS_TARGET_ERROR;
}

static S64SettingsStoreResult save_named(
    S64Dos* dos,
    const char* path,
    const char* filename,
    S64SettingsExchange* settings
)
{
    S64DosResult result;
    S64SettingsStoreResult verified_result;
    S64SettingsExchange* verified;
#ifndef S64_PERSISTENT_RUNTIME_FIT
    static S64SettingsExchange verified_storage;
#endif

#ifdef S64_PERSISTENT_RUNTIME_FIT
    if (S64_SETTINGS_WORK_VERIFY_OFFSET + sizeof(S64SettingsExchange)
            > S64_PUBLISH_WORKSPACE_CAPACITY) {
        return S64_SETTINGS_STORE_INVALID;
    }
    verified = (S64SettingsExchange*)
        &s64_publisher_workspace.bytes[S64_SETTINGS_WORK_VERIFY_OFFSET];
#else
    verified = &verified_storage;
#endif

    if (dos == 0 || path == 0 || filename == 0 || settings == 0) {
        return S64_SETTINGS_STORE_INVALID;
    }
    if (select_storage_path(dos, path) != S64_SETTINGS_STORE_OK) {
        return S64_SETTINGS_STORE_IO_ERROR;
    }

    settings->state = S64_SETTINGS_STATE_READY;
    settings->result = S64_SETTINGS_RESULT_OK;
    if (!s64_settings_exchange_prepare(settings)) {
        return S64_SETTINGS_STORE_INVALID;
    }

    /*
     * C64 Ultimate firmware can report non-fatal filesystem statuses for
     * delete/close operations. Follow the proven Ultimate application pattern:
     * treat cleanup deletes and CLOSE as best effort, and use the operations
     * that establish correctness as gates, then reopen and validate the final
     * file. Status-only success cannot prove persistence.
     *
     * The existing destination remains untouched until the replacement temp
     * file has been completely written.
     */
    (void)s64_dos_delete_file(dos, S64_SETTINGS_TEMP_FILENAME);

    result = open_create_new(dos, S64_SETTINGS_TEMP_FILENAME);
    if (result != S64_DOS_OK) {
        return S64_SETTINGS_STORE_IO_ERROR;
    }

    result = s64_dos_write(
        dos,
        (const uint8_t*)settings,
        (uint16_t)sizeof(*settings)
    );
    (void)s64_dos_close(dos);
    if (result != S64_DOS_OK) {
        (void)s64_dos_delete_file(dos, S64_SETTINGS_TEMP_FILENAME);
        return S64_SETTINGS_STORE_IO_ERROR;
    }

    (void)s64_dos_delete_file(dos, filename);

    result = s64_dos_rename_file(
        dos,
        S64_SETTINGS_TEMP_FILENAME,
        filename
    );
    if (result != S64_DOS_OK) {
        return S64_SETTINGS_STORE_IO_ERROR;
    }

    verified_result = load_named(dos, path, filename, verified);
    if (verified_result != S64_SETTINGS_STORE_OK
        || memcmp(settings, verified, sizeof(*verified)) != 0) {
        memset(verified, 0, sizeof(*verified));
        return S64_SETTINGS_STORE_VERIFY_ERROR;
    }
    memset(verified, 0, sizeof(*verified));
    return S64_SETTINGS_STORE_OK;
}

S64SettingsStoreResult s64_settings_store_load(
    S64Dos* dos,
    S64SettingsExchange* settings
)
{
    S64SettingsStoreResult result;
    S64SettingsStoreResult migration;

    result = load_named(
        dos,
        S64_PACKAGE_DIRECTORY,
        S64_SETTINGS_FILENAME,
        settings
    );
    if (result != S64_SETTINGS_STORE_NOT_FOUND) return result;

    result = load_named(
        dos,
        S64_SD_ROOT_DIRECTORY,
        S64_SETTINGS_FILENAME,
        settings
    );
    if (result != S64_SETTINGS_STORE_OK) return result;

    migration = save_named(
        dos,
        S64_PACKAGE_DIRECTORY,
        S64_SETTINGS_FILENAME,
        settings
    );
    if (migration != S64_SETTINGS_STORE_OK) return migration;

    /* The verified app-folder copy is authoritative. Retiring the old root
     * copy is best-effort so firmware-specific delete status cannot invalidate
     * an already verified migration. */
    if (select_storage_path(dos, S64_SD_ROOT_DIRECTORY)
            == S64_SETTINGS_STORE_OK) {
        (void)s64_dos_delete_file(dos, S64_SETTINGS_FILENAME);
    }
    return S64_SETTINGS_STORE_OK;
}

S64SettingsStoreResult s64_settings_store_save(
    S64Dos* dos,
    S64SettingsExchange* settings
)
{
    return save_named(
        dos,
        S64_PACKAGE_DIRECTORY,
        S64_SETTINGS_FILENAME,
        settings
    );
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
    return load_named(dos, S64_PACKAGE_DIRECTORY, filename, settings);
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
    return save_named(dos, S64_PACKAGE_DIRECTORY, filename, settings);
}

static S64SettingsStoreResult import_direct_key_from(
    S64Dos* dos,
    const char* path,
    char direct_key[65]
)
{
    S64DosResult result;
    S64DosResult close_result;
    S64DirectKeyRead read;
    uint16_t bytes_read = 0u;

    if (dos == 0 || path == 0 || direct_key == 0) {
        return S64_SETTINGS_STORE_INVALID;
    }
    if (select_storage_path(dos, path) != S64_SETTINGS_STORE_OK) {
        return S64_SETTINGS_STORE_IO_ERROR;
    }

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
    if (result != S64_DOS_OK) {
        memset(&read, 0, sizeof(read));
        return S64_SETTINGS_STORE_IO_ERROR;
    }
    (void)close_result;
    if (read.count != 64u || bytes_read < 64u) {
        memset(&read, 0, sizeof(read));
        return S64_SETTINGS_STORE_INVALID;
    }

    read.key[64] = '\0';
    memcpy(direct_key, read.key, sizeof(read.key));
    memset(&read, 0, sizeof(read));
    return S64_SETTINGS_STORE_OK;
}

S64SettingsStoreResult s64_settings_import_direct_key(
    S64Dos* dos,
    char direct_key[65]
)
{
    S64SettingsStoreResult result;

    result = import_direct_key_from(dos, S64_PACKAGE_DIRECTORY, direct_key);
    if (result != S64_SETTINGS_STORE_NOT_FOUND) return result;
    return import_direct_key_from(dos, S64_SD_ROOT_DIRECTORY, direct_key);
}
