#ifndef SEE64YOU_PUBLISHER_PERSISTENT_SETTINGS_UI_H
#define SEE64YOU_PUBLISHER_PERSISTENT_SETTINGS_UI_H

#include <stdint.h>

#include "publisher/settings_exchange.h"

#define S64_SETTINGS_SCREEN_MAX_LINES 18u
#define S64_SETTINGS_SCREEN_DYNAMIC_LINES 5u
#define S64_SETTINGS_SCREEN_COLUMNS 40u

typedef struct {
    uint8_t row;
    const char* text;
} S64PersistentSettingsScreenLine;

typedef struct {
    const char* title;
    uint8_t line_count;
    uint8_t dynamic_count;
    S64PersistentSettingsScreenLine lines[S64_SETTINGS_SCREEN_MAX_LINES];
    char dynamic[S64_SETTINGS_SCREEN_DYNAMIC_LINES][S64_SETTINGS_SCREEN_COLUMNS + 1u];
    const char* status;
} S64PersistentSettingsScreen;

typedef enum {
    S64_SETTINGS_UI_NONE = 0,
    S64_SETTINGS_UI_EDIT_SITE,
    S64_SETTINGS_UI_IMPORT_DIRECT_KEY,
    S64_SETTINGS_UI_REPLACE_DIRECT_KEY,
    S64_SETTINGS_UI_CHECK,
    S64_SETTINGS_UI_CHOOSE_DESTINATION,
    S64_SETTINGS_UI_TOGGLE_HTTP,
    S64_SETTINGS_UI_REPLACE_RELAY_KEY,
    S64_SETTINGS_UI_LOAD_PROFILE,
    S64_SETTINGS_UI_SAVE_PROFILE,
    S64_SETTINGS_UI_DIAGNOSTICS,
    S64_SETTINGS_UI_SAVE_RETURN,
    S64_SETTINGS_UI_CANCEL_RETURN
} S64PersistentSettingsAction;

S64PersistentSettingsAction s64_persistent_settings_key(
    S64SettingsExchange* settings,
    uint8_t* advanced_view,
    uint8_t key
);

uint8_t s64_persistent_settings_screen(
    const S64SettingsExchange* settings,
    uint8_t advanced_view,
    const char* status,
    S64PersistentSettingsScreen* screen
);

#endif
