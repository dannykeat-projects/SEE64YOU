#include <conio.h>
#include <string.h>

#include "charset.h"
#include "platform/chain.h"
#include "platform/dos.h"
#include "platform/reu.h"
#include "platform/uci.h"
#include "publisher/destination.h"
#include "publisher/settings_exchange.h"
#include "publisher/settings_store.h"

#define SCREEN_COLUMNS 40u
#define STATUS_ROW 24u

static S64SettingsExchange settings;
static S64SettingsExchange original_settings;
static S64SettingsExchange profile_settings;
static S64PublishDestination checked_destination;
static S64Dos dos;
static S64UciCapabilities capabilities;
static char status_text[SCREEN_COLUMNS + 1u];
static char relay_replacement[S64_RELAY_KEY_CAPACITY];

static void clear_row(unsigned char row)
{
    unsigned char i;
    gotoxy(0u, row);
    for (i = 0u; i < SCREEN_COLUMNS; ++i) cputc(' ');
}

static void set_status(const char* text)
{
    unsigned char i = 0u;
    while (i < SCREEN_COLUMNS && text[i] != '\0') {
        status_text[i] = text[i];
        ++i;
    }
    status_text[i] = '\0';
}

static const char* destination_name(void)
{
    if (settings.destination_type == (uint8_t)S64_DESTINATION_STANDALONE) return "Standalone";
    if (settings.destination_type == (uint8_t)S64_DESTINATION_WORDPRESS) return "WordPress";
    return "None";
}

static void print_value(unsigned char row, const char* label, const char* value)
{
    unsigned char col = 0u;
    clear_row(row);
    gotoxy(0u, row);
    while (col < SCREEN_COLUMNS && *label != '\0') {
        cputc((unsigned char)*label++);
        ++col;
    }
    while (col < SCREEN_COLUMNS && *value != '\0') {
        cputc((unsigned char)*value++);
        ++col;
    }
}

static void render(void)
{
    clrscr();
    revers(1);
    gotoxy(0u, 0u);
    cputs(" SEE64YOU PUBLISHING SETTINGS          ");
    revers(0);
    print_value(3u, "Destination: ", destination_name());
    print_value(5u, "Site URL: ", settings.base_url[0] != '\0' ? settings.base_url : "(not set)");
    print_value(7u, "Plain HTTP: ", settings.allow_insecure_http ? "allowed" : "blocked");
    print_value(9u, "Relay key: ", settings.relay_key[0] != '\0' ? "configured" : "off");
    gotoxy(0u, 12u); cputs("1 DESTINATION TYPE");
    gotoxy(0u, 13u); cputs("2 SITE URL");
    gotoxy(0u, 14u); cputs("3 TOGGLE PLAIN HTTP");
    gotoxy(0u, 15u); cputs("4 REPLACE RELAY KEY");
    gotoxy(0u, 16u); cputs("5 DISABLE RELAY KEY");
    gotoxy(0u, 17u); cputs("6 LOAD PROFILE");
    gotoxy(0u, 18u); cputs("7 SAVE PROFILE");
    gotoxy(0u, 20u); cputs("S SAVE + RETURN");
    gotoxy(0u, 21u); cputs("RUN/STOP CANCEL + RETURN");
    clear_row(STATUS_ROW);
    gotoxy(0u, STATUS_ROW);
    revers(1); cputs(status_text); revers(0);
}

static uint8_t printable(unsigned char key)
{
    uint8_t ascii;
    return s64_charset_native_to_ascii(key, &ascii)
        && ascii >= 0x20u && ascii <= 0x7eu;
}

static unsigned char edit_text(
    const char* label,
    char* value,
    unsigned char capacity,
    unsigned char masked
)
{
    unsigned char key;
    unsigned char length = (unsigned char)strlen(value);
    unsigned char label_length = (unsigned char)strlen(label);
    unsigned char visible;
    unsigned char start;
    unsigned char shown;

    if (capacity == 0u || label_length >= SCREEN_COLUMNS) return 0u;
    visible = (unsigned char)(SCREEN_COLUMNS - label_length - 1u);

    for (;;) {
        start = length > visible ? (unsigned char)(length - visible) : 0u;
        shown = 0u;
        clear_row(STATUS_ROW);
        revers(1);
        gotoxy(0u, STATUS_ROW);
        cputs(label);
        while (shown < visible && (unsigned char)(start + shown) < length) {
            cputc(masked ? '*' : (unsigned char)value[start + shown]);
            ++shown;
        }
        while (shown < visible) {
            cputc(' ');
            ++shown;
        }
        gotoxy((unsigned char)(label_length + (length - start)), STATUS_ROW);

        key = (unsigned char)cgetc();
        if (key == CH_STOP) {
            revers(0);
            return 0u;
        }
        if (key == CH_ENTER) {
            value[length] = '\0';
            revers(0);
            return 1u;
        }
        if (key == CH_DEL) {
            if (length != 0u) {
                --length;
                value[length] = '\0';
            }
        } else if (printable(key) && length + 1u < capacity) {
            value[length++] = (char)key;
            value[length] = '\0';
        }
    }
}

static void choose_destination(void)
{
    unsigned char key;
    clear_row(STATUS_ROW);
    gotoxy(0u, STATUS_ROW);
    revers(1);
    cputs("0=NONE 1=STANDALONE");
    for (;;) {
        key = (unsigned char)cgetc();
        if (key == '0') {
            settings.destination_type = (uint8_t)S64_DESTINATION_NONE;
            settings.base_url[0] = '\0';
            settings.allow_insecure_http = 0u;
            break;
        }
        if (key == '1') {
            settings.destination_type = (uint8_t)S64_DESTINATION_STANDALONE;
            break;
        }
        if (key == CH_STOP) {
            revers(0);
            set_status("Destination unchanged");
            return;
        }
    }
    revers(0);
    set_status("Destination type changed");
}

static uint8_t validate_values(S64SettingsExchange* target)
{
    S64DestinationResult result;

    if (target == 0) return 0u;
    if (target->destination_type == (uint8_t)S64_DESTINATION_NONE) {
        target->base_url[0] = '\0';
        target->allow_insecure_http = 0u;
        return s64_settings_exchange_values_valid(target) ? 1u : 0u;
    }

    result = s64_destination_configure(
        &checked_destination,
        (S64DestinationType)target->destination_type,
        target->base_url,
        target->allow_insecure_http
    );
    if (result != S64_DESTINATION_OK) {
        set_status(s64_destination_result_message(result));
        return 0u;
    }
    memcpy(
        target->base_url,
        checked_destination.base_url,
        sizeof(target->base_url)
    );
    target->allow_insecure_http = checked_destination.allow_insecure_http;
    if (!s64_settings_exchange_values_valid(target)) {
        set_status("Relay key invalid");
        return 0u;
    }
    return 1u;
}

static uint8_t validate_settings(void)
{
    return validate_values(&settings);
}

static uint8_t choose_profile_slot(void)
{
    unsigned char key;

    clear_row(STATUS_ROW);
    gotoxy(0u, STATUS_ROW);
    revers(1);
    cputs("PROFILE 1-4  STOP=CANCEL");

    for (;;) {
        key = (unsigned char)cgetc();
        if (key >= '1' && key <= '4') {
            revers(0);
            return (uint8_t)(key - '1');
        }
        if (key == CH_STOP) {
            revers(0);
            return 0xffu;
        }
    }
}

static void load_profile(void)
{
    S64SettingsStoreResult result;
    uint8_t slot;

    if (!capabilities.dos_target) {
        set_status("Settings storage unavailable");
        return;
    }

    slot = choose_profile_slot();
    if (slot == 0xffu) {
        set_status("Profile load cancelled");
        return;
    }

    result = s64_settings_profile_load(&dos, slot, &profile_settings);
    if (result == S64_SETTINGS_STORE_NOT_FOUND) {
        set_status("Profile slot is empty");
        return;
    }
    if (result == S64_SETTINGS_STORE_INVALID) {
        set_status("Profile data invalid");
        return;
    }
    if (result != S64_SETTINGS_STORE_OK) {
        set_status("Profile load failed");
        return;
    }
    if (!validate_values(&profile_settings)) {
        return;
    }

    settings = profile_settings;
    settings.state = S64_SETTINGS_STATE_EDIT_REQUEST;
    set_status("Profile loaded; S to use");
}

static void save_profile(void)
{
    S64SettingsStoreResult result;
    uint8_t slot;

    if (!capabilities.dos_target) {
        set_status("Settings storage unavailable");
        return;
    }
    if (!validate_settings()) return;

    slot = choose_profile_slot();
    if (slot == 0xffu) {
        set_status("Profile save cancelled");
        return;
    }

    profile_settings = settings;
    result = s64_settings_profile_save(&dos, slot, &profile_settings);
    if (result == S64_SETTINGS_STORE_OK) {
        set_status("Profile saved");
    } else {
        set_status("Profile save failed");
    }
}

static void return_to_publisher(void)
{
    clrscr();
    cputs("RETURNING TO PUBLISHER...\r\n");
    s64_chain_publisher();
}

static void save_and_return(void)
{
    S64SettingsStoreResult result;
    if (!validate_settings()) return;

    settings.state = S64_SETTINGS_STATE_READY;
    settings.result = S64_SETTINGS_RESULT_OK;
    if (capabilities.dos_target) {
        result = s64_settings_store_save(&dos, &settings);
        if (result != S64_SETTINGS_STORE_OK) settings.result = S64_SETTINGS_RESULT_IO_ERROR;
    } else {
        settings.result = S64_SETTINGS_RESULT_IO_ERROR;
    }
    if (!s64_settings_exchange_store(&settings)) {
        set_status("Cannot preserve settings");
        return;
    }
    return_to_publisher();
}

static void cancel_and_return(void)
{
    settings = original_settings;
    settings.state = S64_SETTINGS_STATE_READY;
    if (!s64_settings_exchange_store(&settings)) {
        set_status("Cannot preserve settings");
        return;
    }
    return_to_publisher();
}

static void service_load_request(void)
{
    S64SettingsStoreResult result;

    s64_settings_exchange_init(&settings);
    if (!capabilities.dos_target) {
        settings.result = S64_SETTINGS_RESULT_IO_ERROR;
    } else {
        result = s64_settings_store_load(&dos, &settings);
        if (result == S64_SETTINGS_STORE_NOT_FOUND) {
            s64_settings_exchange_init(&settings);
            settings.result = S64_SETTINGS_RESULT_NOT_FOUND;
        } else if (result == S64_SETTINGS_STORE_INVALID) {
            s64_settings_exchange_init(&settings);
            settings.result = S64_SETTINGS_RESULT_INVALID_FILE;
        } else if (result != S64_SETTINGS_STORE_OK) {
            s64_settings_exchange_init(&settings);
            settings.result = S64_SETTINGS_RESULT_IO_ERROR;
        } else if (!validate_settings()) {
            s64_settings_exchange_init(&settings);
            settings.result = S64_SETTINGS_RESULT_INVALID_FILE;
        } else {
            settings.result = S64_SETTINGS_RESULT_OK;
        }
    }
    settings.state = S64_SETTINGS_STATE_READY;
    if (!s64_settings_exchange_store(&settings)) {
        clrscr();
        cputs("SETTINGS HANDOFF FAILED\r\n");
        return;
    }
    return_to_publisher();
}

int main(void)
{
    unsigned char key;
    unsigned char have_exchange;

    bgcolor(COLOR_BLUE);
    bordercolor(COLOR_BLUE);
    textcolor(COLOR_LIGHTBLUE);
    clrscr();
    cursor(0);

    if (!s64_reu_probe() || s64_reu_probe_banks() < 2u) {
        cputs("SEE64YOU SETTINGS\r\n128K REU REQUIRED\r\n");
        return 1;
    }

    s64_uci_probe_capabilities(&capabilities);
    s64_dos_init(&dos, s64_uci_execute, S64_DOS_TARGET_PRIMARY);

    have_exchange = s64_settings_exchange_load(&settings) ? 1u : 0u;
    if (have_exchange && settings.state == S64_SETTINGS_STATE_LOAD_REQUEST) {
        service_load_request();
    }

    if (!have_exchange) {
        s64_settings_exchange_init(&settings);
        if (capabilities.dos_target) {
            S64SettingsStoreResult result = s64_settings_store_load(&dos, &settings);
            if (result != S64_SETTINGS_STORE_OK || !validate_settings()) {
                s64_settings_exchange_init(&settings);
            }
        }
        settings.state = S64_SETTINGS_STATE_EDIT_REQUEST;
    }

    if (settings.state != S64_SETTINGS_STATE_EDIT_REQUEST
        && settings.state != S64_SETTINGS_STATE_READY) {
        s64_settings_exchange_init(&settings);
        settings.state = S64_SETTINGS_STATE_EDIT_REQUEST;
    }

    original_settings = settings;
    status_text[0] = '\0';
    for (;;) {
        render();
        key = (unsigned char)cgetc();
        if (key == '1') {
            choose_destination();
        } else if (key == '2') {
            if (settings.destination_type == (uint8_t)S64_DESTINATION_NONE) {
                set_status("Choose destination type first");
            } else {
                (void)edit_text(
                    "SITE URL: ",
                    settings.base_url,
                    (unsigned char)sizeof(settings.base_url),
                    0u
                );
            }
        } else if (key == '3') {
            settings.allow_insecure_http = settings.allow_insecure_http ? 0u : 1u;
            set_status(settings.allow_insecure_http ? "Plain HTTP allowed" : "Plain HTTP blocked");
        } else if (key == '4') {
            relay_replacement[0] = '\0';
            if (edit_text(
                    "RELAY KEY: ",
                    relay_replacement,
                    (unsigned char)sizeof(relay_replacement),
                    1u
                )) {
                if (relay_replacement[0] == '\0') {
                    set_status("Relay key unchanged");
                } else {
                    memcpy(settings.relay_key, relay_replacement, sizeof(relay_replacement));
                    if (!s64_settings_exchange_values_valid(&settings)) {
                        settings.relay_key[0] = '\0';
                        set_status("Relay key must be 64 hex chars");
                    } else {
                        set_status("Relay key replaced");
                    }
                }
            }
            memset(relay_replacement, 0, sizeof(relay_replacement));
        } else if (key == '5') {
            settings.relay_key[0] = '\0';
            set_status("Relay key disabled");
        } else if (key == '6') {
            load_profile();
        } else if (key == '7') {
            save_profile();
        } else if (key == 's' || key == 'S') {
            save_and_return();
        } else if (key == CH_STOP) {
            cancel_and_return();
        }
    }
}
