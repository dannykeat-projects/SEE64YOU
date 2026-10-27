#include "publisher/settings_store.h"
#include "publisher/standard_keyfile.h"
#include "platform/package_paths.h"
#include <string.h>

typedef struct {
    uint8_t bytes[S64_PUBLISH_KEYFILE_MAX_BYTES + 1u];
    uint16_t length;
    uint8_t overflow;
} S64PublishKeyRead;

static uint8_t publish_key_read(const uint8_t* data, uint16_t length, void* context)
{
    S64PublishKeyRead* read = (S64PublishKeyRead*)context;
    if ((uint32_t)read->length + length > sizeof(read->bytes)) {
        read->overflow = 1u;
        return 0u;
    }
    memcpy(read->bytes + read->length, data, length);
    read->length += length;
    return 1u;
}

S64SettingsStoreResult s64_settings_import_publish_key(
    S64Dos* dos, S64SettingsExchange* settings)
{
    static S64PublishKeyRead read;
    static S64PublishKeyFile parsed;
    S64DosResult result, closed;
    S64SettingsStoreResult outcome = S64_SETTINGS_STORE_INVALID;
    uint16_t bytes_read = 0u;
    if (!dos || !settings) return S64_SETTINGS_STORE_INVALID;
    memset(&read, 0, sizeof(read));
    memset(&parsed, 0, sizeof(parsed));
    if (s64_dos_change_dir(dos, S64_PACKAGE_DIRECTORY) != S64_DOS_OK)
        return S64_SETTINGS_STORE_IO_ERROR;
    result = s64_dos_open_read(dos, S64_PUBLISH_KEY_FILENAME);
    if (result == S64_DOS_NOT_FOUND) return S64_SETTINGS_STORE_NOT_FOUND;
    if (result != S64_DOS_OK) return S64_SETTINGS_STORE_IO_ERROR;
    result = s64_dos_read(dos, sizeof(read.bytes), publish_key_read, &read, &bytes_read);
    closed = s64_dos_close(dos);
    if (result != S64_DOS_OK || closed != S64_DOS_OK) {
        outcome = S64_SETTINGS_STORE_IO_ERROR;
    } else if (!read.overflow && read.length == bytes_read
        && s64_publish_keyfile_parse(read.bytes, read.length, &parsed)
        && s64_settings_publish_url_valid(parsed.url)) {
        /* Commit the complete identity only after validation.
           A new destination requires a fresh unencrypted-HTTP decision. */
        memset(settings->relay_key, 0, sizeof(settings->relay_key));
        memcpy(settings->relay_key, parsed.id, sizeof(parsed.id));
        memcpy(settings->base_url, parsed.url, sizeof(parsed.url));
        memcpy(settings->direct_key, parsed.key, sizeof(parsed.key));
        settings->destination_type = S64_DESTINATION_STANDARD;
        settings->allow_insecure_http = 0u;
        outcome = S64_SETTINGS_STORE_OK;
    }
    memset(&read, 0, sizeof(read));
    memset(&parsed, 0, sizeof(parsed));
    return outcome;
}
