#include "publisher/persistent_settings_mode.h"

S64PersistentSettingsAction s64_persistent_settings_dispatch_key(
    S64SettingsExchange* settings,
    S64Dos* dos,
    uint8_t* advanced_view,
    uint8_t key,
    S64PersistentSettingsCommitResult* transaction_result)
{
    S64PersistentSettingsAction action;

    if (transaction_result) {
        *transaction_result = S64_SETTINGS_COMMIT_OK;
    }
    if (!settings || !dos || !advanced_view || !transaction_result) {
        if (transaction_result) {
            *transaction_result = S64_SETTINGS_COMMIT_INVALID;
        }
        return S64_SETTINGS_UI_NONE;
    }

    action = s64_persistent_settings_key(settings, advanced_view, key);
    if (action == S64_SETTINGS_UI_SAVE_RETURN) {
        *transaction_result = s64_persistent_settings_commit(dos, settings);
    } else if (action == S64_SETTINGS_UI_CANCEL_RETURN) {
        *transaction_result = s64_persistent_settings_restore_original(settings);
    }
    return action;
}
