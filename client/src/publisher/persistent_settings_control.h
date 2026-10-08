#ifndef SEE64YOU_PUBLISHER_PERSISTENT_SETTINGS_CONTROL_H
#define SEE64YOU_PUBLISHER_PERSISTENT_SETTINGS_CONTROL_H

#include <stdint.h>

#include "platform/dos.h"
#include "publisher/settings_exchange.h"

typedef enum {
    S64_SETTINGS_CONTROL_OK = 0,
    S64_SETTINGS_CONTROL_INVALID_ARGUMENT,
    S64_SETTINGS_CONTROL_INVALID_DESTINATION,
    S64_SETTINGS_CONTROL_INVALID_VALUES
} S64PersistentSettingsControlResult;

S64PersistentSettingsControlResult s64_persistent_settings_validate_values(
    S64SettingsExchange* settings
);

typedef enum {
    S64_SETTINGS_PROFILE_OK = 0,
    S64_SETTINGS_PROFILE_INVALID_ARGUMENT,
    S64_SETTINGS_PROFILE_NOT_FOUND,
    S64_SETTINGS_PROFILE_INVALID,
    S64_SETTINGS_PROFILE_IO_ERROR
} S64PersistentSettingsProfileResult;

S64PersistentSettingsProfileResult s64_persistent_settings_profile_load(
    S64Dos* dos,
    uint8_t slot,
    S64SettingsExchange* settings,
    S64SettingsExchange* scratch
);

S64PersistentSettingsProfileResult s64_persistent_settings_profile_save(
    S64Dos* dos,
    uint8_t slot,
    const S64SettingsExchange* settings,
    S64SettingsExchange* scratch
);

typedef enum {
    S64_SETTINGS_KEY_IMPORT_OK = 0,
    S64_SETTINGS_KEY_IMPORT_INVALID_ARGUMENT,
    S64_SETTINGS_KEY_IMPORT_NOT_FOUND,
    S64_SETTINGS_KEY_IMPORT_INVALID,
    S64_SETTINGS_KEY_IMPORT_IO_ERROR
} S64PersistentSettingsKeyImportResult;

S64PersistentSettingsKeyImportResult s64_persistent_settings_import_direct_key(
    S64Dos* dos,
    S64SettingsExchange* settings,
    char scratch[65]
);

typedef enum {
    S64_SETTINGS_COMMIT_OK = 0,
    S64_SETTINGS_COMMIT_INVALID,
    S64_SETTINGS_COMMIT_STORE_ERROR,
    S64_SETTINGS_COMMIT_VERIFY_ERROR,
    S64_SETTINGS_COMMIT_EXCHANGE_ERROR
} S64PersistentSettingsCommitResult;

S64PersistentSettingsCommitResult s64_persistent_settings_commit(
    S64Dos* dos,
    S64SettingsExchange* settings
);

S64PersistentSettingsCommitResult s64_persistent_settings_restore_original(
    S64SettingsExchange* settings
);

#endif
