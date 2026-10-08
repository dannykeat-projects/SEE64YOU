#include "publisher/persistent_settings_ui.h"

#include <conio.h>


static void screen_line(
    S64PersistentSettingsScreen* screen,
    uint8_t row,
    const char* label,
    const char* value)
{
    uint8_t index = screen->line_count;

    if (index >= S64_SETTINGS_SCREEN_MAX_LINES) return;
    screen->lines[index].row = row;
    screen->lines[index].label = label;
    screen->lines[index].value = value;
    screen->line_count = (uint8_t)(index + 1u);
}

uint8_t s64_persistent_settings_screen(
    const S64SettingsExchange* settings,
    uint8_t advanced_view,
    const char* status,
    S64PersistentSettingsScreen* screen)
{
    const char* destination;

    if (!settings || !screen || advanced_view > 1u) return 0u;

    screen->line_count = 0u;
    screen->status = status;
    destination = settings->destination_type == (uint8_t)S64_DESTINATION_STANDALONE
        ? "Standalone"
        : (settings->destination_type == (uint8_t)S64_DESTINATION_WORDPRESS
            ? "WordPress" : "None");

    if (!advanced_view) {
        screen->title = " SEE64YOU SETTINGS ";
        screen_line(screen, 3u, "Website: ",
            settings->base_url[0] ? settings->base_url : "(not set)");
        screen_line(screen, 5u, "Publishing key: ",
            settings->direct_key[0] ? "configured" : "not set");
        screen_line(screen, 7u, "Publishing: ",
            settings->destination_type == (uint8_t)S64_DESTINATION_NONE
                ? "not configured" : destination);
        screen_line(screen, 11u, "1 WEBSITE", 0);
        screen_line(screen, 12u, "2 IMPORT SEE64KEY.TXT", 0);
        screen_line(screen, 13u, "3 TYPE / REPLACE KEY", 0);
        screen_line(screen, 14u, "4 DISABLE PUBLISHING KEY", 0);
        screen_line(screen, 17u, "T CHECK READINESS", 0);
        screen_line(screen, 18u, "A ADVANCED SETTINGS", 0);
        screen_line(screen, 21u, "S SAVE + RETURN", 0);
        screen_line(screen, 22u, "RUN/STOP CANCEL + RETURN", 0);
        return 1u;
    }

    screen->title = " SEE64YOU ADVANCED SETTINGS ";
    screen_line(screen, 2u, "B BASIC SETTINGS", 0);
    screen_line(screen, 3u, "Destination: ", destination);
    screen_line(screen, 5u, "Site URL: ",
        settings->base_url[0] ? settings->base_url : "(not set)");
    screen_line(screen, 7u, "Plain HTTP: ",
        settings->allow_insecure_http ? "allowed" : "blocked");
    screen_line(screen, 9u, "Direct key: ",
        settings->direct_key[0] ? "configured" : "off");
    screen_line(screen, 10u, "Relay key: ",
        settings->relay_key[0] ? "configured" : "off");
    screen_line(screen, 12u, "1 DESTINATION TYPE", 0);
    screen_line(screen, 13u, "2 SITE URL", 0);
    screen_line(screen, 14u, "3 PLAIN HTTP POLICY", 0);
    screen_line(screen, 15u, "4 IMPORT DIRECT KEY FILE", 0);
    screen_line(screen, 16u, "A TYPE DIRECT KEY", 0);
    screen_line(screen, 17u, "5 DISABLE DIRECT KEY", 0);
    screen_line(screen, 18u, "6 REPLACE RELAY KEY", 0);
    screen_line(screen, 19u, "7 DISABLE RELAY KEY", 0);
    screen_line(screen, 20u, "8 LOAD PROFILE", 0);
    screen_line(screen, 21u, "9 SAVE PROFILE", 0);
    screen_line(screen, 22u, "D DIAGNOSTICS", 0);
    screen_line(screen, 23u, "S SAVE  B BASIC  STOP CANCEL", 0);
    return 1u;
}

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
