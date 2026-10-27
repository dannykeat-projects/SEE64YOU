#ifndef SEE64YOU_PUBLISHER_SETTINGS_STORE_H
#define SEE64YOU_PUBLISHER_SETTINGS_STORE_H

#include "platform/dos.h"
#include "publisher/settings_exchange.h"

#define S64_SETTINGS_FILENAME "SEE64YOU.CFG"
#define S64_DIRECT_KEY_FILENAME "SEE64KEY.TXT"
#define S64_DIRECT_KEY_FILENAME_ALT "SEE64KEY.txt"
#define S64_PUBLISH_KEY_FILENAME "8TMLKEY.TXT"
#define S64_SETTINGS_PROFILE_COUNT 4u

typedef enum {
    S64_SETTINGS_STORE_OK = 0,
    S64_SETTINGS_STORE_NOT_FOUND,
    S64_SETTINGS_STORE_INVALID,
    S64_SETTINGS_STORE_IO_ERROR,
    S64_SETTINGS_STORE_VERIFY_ERROR
} S64SettingsStoreResult;

/* Standard applications never migrate/delete legacy root configuration. */
S64SettingsStoreResult s64_settings_publish_store_load(
    S64Dos* dos, S64SettingsExchange* settings);

S64SettingsStoreResult s64_settings_store_load(
    S64Dos* dos,
    S64SettingsExchange* settings
);

S64SettingsStoreResult s64_settings_store_save(
    S64Dos* dos,
    S64SettingsExchange* settings
);

S64SettingsStoreResult s64_settings_profile_load(
    S64Dos* dos,
    uint8_t slot,
    S64SettingsExchange* settings
);

S64SettingsStoreResult s64_settings_profile_save(
    S64Dos* dos,
    uint8_t slot,
    S64SettingsExchange* settings
);

/* Failed imports leave settings unchanged; no legacy key fallback. */
S64SettingsStoreResult s64_settings_import_publish_key(
    S64Dos* dos, S64SettingsExchange* settings);

S64SettingsStoreResult s64_settings_import_direct_key(
    S64Dos* dos,
    char direct_key[65]
);

#endif
