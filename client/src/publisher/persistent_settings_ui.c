#include "publisher/persistent_settings_ui.h"

#include <conio.h>

S64PersistentSettingsAction s64_persistent_settings_key(
    S64SettingsExchange* settings,
    uint8_t* advanced_view,
    uint8_t key)
{
    if (!settings || !advanced_view || *advanced_view > 1u) {
        return S64_SETTINGS_UI_NONE;
    }

    if (!*advanced_view) {
        if (key == '1') return S64_SETTINGS_UI_EDIT_SITE;
        if (key == '2') return S64_SETTINGS_UI_IMPORT_DIRECT_KEY;
        if (key == '3') return S64_SETTINGS_UI_REPLACE_DIRECT_KEY;
        if (key == '4') {
            settings->direct_key[0] = 0;
            return S64_SETTINGS_UI_NONE;
        }
        if (key == 't' || key == 'T') return S64_SETTINGS_UI_CHECK;
        if (key == 'a' || key == 'A') {
            *advanced_view = 1u;
            return S64_SETTINGS_UI_NONE;
        }
        if (key == 's' || key == 'S') return S64_SETTINGS_UI_SAVE_RETURN;
        if (key == CH_STOP) return S64_SETTINGS_UI_CANCEL_RETURN;
        return S64_SETTINGS_UI_NONE;
    }

    if (key == '1') return S64_SETTINGS_UI_CHOOSE_DESTINATION;
    if (key == '2') return S64_SETTINGS_UI_EDIT_SITE;
    if (key == '3') return S64_SETTINGS_UI_TOGGLE_HTTP;
    if (key == '4') return S64_SETTINGS_UI_IMPORT_DIRECT_KEY;
    if (key == 'a' || key == 'A') return S64_SETTINGS_UI_REPLACE_DIRECT_KEY;
    if (key == '5') {
        settings->direct_key[0] = 0;
        return S64_SETTINGS_UI_NONE;
    }
    if (key == '6') return S64_SETTINGS_UI_REPLACE_RELAY_KEY;
    if (key == '7') {
        settings->relay_key[0] = 0;
        return S64_SETTINGS_UI_NONE;
    }
    if (key == '8') return S64_SETTINGS_UI_LOAD_PROFILE;
    if (key == '9') return S64_SETTINGS_UI_SAVE_PROFILE;
    if (key == 'd' || key == 'D') return S64_SETTINGS_UI_DIAGNOSTICS;
    if (key == 'b' || key == 'B') {
        *advanced_view = 0u;
        return S64_SETTINGS_UI_NONE;
    }
    if (key == 's' || key == 'S') return S64_SETTINGS_UI_SAVE_RETURN;
    if (key == CH_STOP) return S64_SETTINGS_UI_CANCEL_RETURN;
    return S64_SETTINGS_UI_NONE;
}
