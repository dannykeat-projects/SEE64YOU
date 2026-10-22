#include "publisher/persistent_settings_control.h"

#include "publisher/destination.h"
#include "publisher/settings_store.h"

#include <string.h>

S64PersistentSettingsControlResult s64_persistent_settings_validate_values(
    S64SettingsExchange* settings)
{
    S64PublishDestination checked;
    S64DestinationResult result;

    if (!settings) return S64_SETTINGS_CONTROL_INVALID_ARGUMENT;

    if (settings->destination_type == (uint8_t)S64_DESTINATION_NONE) {
        settings->base_url[0] = 0;
        settings->allow_insecure_http = 0u;
        return s64_settings_exchange_values_valid(settings)
            ? S64_SETTINGS_CONTROL_OK
            : S64_SETTINGS_CONTROL_INVALID_VALUES;
    }

    result = s64_destination_configure(
        &checked,
        (S64DestinationType)settings->destination_type,
        settings->base_url,
        settings->allow_insecure_http
    );
    if (result != S64_DESTINATION_OK) {
        return S64_SETTINGS_CONTROL_INVALID_DESTINATION;
    }

    memcpy(settings->base_url, checked.base_url, sizeof(settings->base_url));
    settings->allow_insecure_http = checked.allow_insecure_http;
    return s64_settings_exchange_values_valid(settings)
        ? S64_SETTINGS_CONTROL_OK
        : S64_SETTINGS_CONTROL_INVALID_VALUES;
}

static S64PersistentSettingsProfileResult map_profile_result(
    S64SettingsStoreResult result)
{
    if (result == S64_SETTINGS_STORE_OK) return S64_SETTINGS_PROFILE_OK;
    if (result == S64_SETTINGS_STORE_NOT_FOUND)
        return S64_SETTINGS_PROFILE_NOT_FOUND;
    if (result == S64_SETTINGS_STORE_INVALID)
        return S64_SETTINGS_PROFILE_INVALID;
    return S64_SETTINGS_PROFILE_IO_ERROR;
}

S64PersistentSettingsProfileResult s64_persistent_settings_profile_load(
    S64Dos* dos,
    uint8_t slot,
    S64SettingsExchange* settings,
    S64SettingsExchange* scratch)
{
    S64SettingsStoreResult store_result;

    if (!dos || !settings || !scratch || slot >= S64_SETTINGS_PROFILE_COUNT) {
        return S64_SETTINGS_PROFILE_INVALID_ARGUMENT;
    }

    store_result = s64_settings_profile_load(dos, slot, scratch);
    if (store_result != S64_SETTINGS_STORE_OK) {
        return map_profile_result(store_result);
    }
    if (s64_persistent_settings_validate_values(scratch)
            != S64_SETTINGS_CONTROL_OK) {
        return S64_SETTINGS_PROFILE_INVALID;
    }

    memcpy(settings, scratch, sizeof(*settings));
    settings->state = S64_SETTINGS_STATE_EDIT_REQUEST;
    return S64_SETTINGS_PROFILE_OK;
}

S64PersistentSettingsProfileResult s64_persistent_settings_profile_save(
    S64Dos* dos,
    uint8_t slot,
    const S64SettingsExchange* settings,
    S64SettingsExchange* scratch)
{
    S64SettingsStoreResult store_result;

    if (!dos || !settings || !scratch || slot >= S64_SETTINGS_PROFILE_COUNT) {
        return S64_SETTINGS_PROFILE_INVALID_ARGUMENT;
    }

    memcpy(scratch, settings, sizeof(*scratch));
    if (s64_persistent_settings_validate_values(scratch)
            != S64_SETTINGS_CONTROL_OK) {
        return S64_SETTINGS_PROFILE_INVALID;
    }

    store_result = s64_settings_profile_save(dos, slot, scratch);
    return map_profile_result(store_result);
}

S64PersistentSettingsKeyImportResult s64_persistent_settings_import_direct_key(
    S64Dos* dos,
    S64SettingsExchange* settings,
    char scratch[65])
{
    S64SettingsStoreResult result;

    if (!dos || !settings || !scratch) {
        return S64_SETTINGS_KEY_IMPORT_INVALID_ARGUMENT;
    }

    memset(scratch, 0, 65u);
    result = s64_settings_import_direct_key(dos, scratch);
    if (result == S64_SETTINGS_STORE_NOT_FOUND) {
        return S64_SETTINGS_KEY_IMPORT_NOT_FOUND;
    }
    if (result == S64_SETTINGS_STORE_INVALID) {
        memset(scratch, 0, 65u);
        return S64_SETTINGS_KEY_IMPORT_INVALID;
    }
    if (result != S64_SETTINGS_STORE_OK) {
        memset(scratch, 0, 65u);
        return S64_SETTINGS_KEY_IMPORT_IO_ERROR;
    }

    memcpy(settings->direct_key, scratch, sizeof(settings->direct_key));
    memset(scratch, 0, 65u);
    return S64_SETTINGS_KEY_IMPORT_OK;
}

S64PersistentSettingsCommitResult s64_persistent_settings_commit(
    S64Dos* dos,
    S64SettingsExchange* settings)
{
    S64SettingsStoreResult result;

    if (!dos || !settings) return S64_SETTINGS_COMMIT_INVALID;
    if (s64_persistent_settings_validate_values(settings)
            != S64_SETTINGS_CONTROL_OK) {
        return S64_SETTINGS_COMMIT_INVALID;
    }

    settings->state = S64_SETTINGS_STATE_READY;
    settings->result = S64_SETTINGS_RESULT_OK;
    result = s64_settings_store_save(dos, settings);
    if (result == S64_SETTINGS_STORE_VERIFY_ERROR) {
        settings->result = S64_SETTINGS_RESULT_IO_ERROR;
        return S64_SETTINGS_COMMIT_VERIFY_ERROR;
    }
    if (result != S64_SETTINGS_STORE_OK) {
        settings->result = S64_SETTINGS_RESULT_IO_ERROR;
        return S64_SETTINGS_COMMIT_STORE_ERROR;
    }
    if (!s64_settings_exchange_store(settings)) {
        return S64_SETTINGS_COMMIT_EXCHANGE_ERROR;
    }
    return S64_SETTINGS_COMMIT_OK;
}

S64PersistentSettingsCommitResult s64_persistent_settings_restore_original(
    S64SettingsExchange* settings)
{
    if (!settings) return S64_SETTINGS_COMMIT_INVALID;
    if (!s64_settings_exchange_load(settings)) {
        return S64_SETTINGS_COMMIT_EXCHANGE_ERROR;
    }
    settings->state = S64_SETTINGS_STATE_READY;
    return s64_settings_exchange_store(settings)
        ? S64_SETTINGS_COMMIT_OK
        : S64_SETTINGS_COMMIT_EXCHANGE_ERROR;
}
