#include "publisher/settings_exchange.h"

#include "platform/reu.h"
#include "platform/reu_layout.h"

#include <string.h>

#define S64_SETTINGS_MAGIC_0 'S'
#define S64_SETTINGS_MAGIC_1 '6'
#define S64_SETTINGS_MAGIC_2 '4'
#define S64_SETTINGS_MAGIC_3 'S'

void s64_settings_exchange_init(S64SettingsExchange* settings)
{
    if (settings == 0) return;

    memset(settings, 0, sizeof(*settings));
    settings->magic[0] = S64_SETTINGS_MAGIC_0;
    settings->magic[1] = S64_SETTINGS_MAGIC_1;
    settings->magic[2] = S64_SETTINGS_MAGIC_2;
    settings->magic[3] = S64_SETTINGS_MAGIC_3;
    settings->version = S64_SETTINGS_VERSION;
    settings->state = S64_SETTINGS_STATE_READY;
    settings->result = S64_SETTINGS_RESULT_OK;
    settings->destination_type = (uint8_t)S64_DESTINATION_NONE;
    settings->checksum = s64_settings_exchange_checksum(settings);
}

int s64_settings_exchange_load(S64SettingsExchange* settings)
{
    if (settings == 0
        || !s64_reu_load(
            S64_REU_SETTINGS_BANK,
            S64_REU_SETTINGS_BASE,
            settings,
            (uint16_t)sizeof(*settings)
        )) {
        return 0;
    }
    return s64_settings_exchange_validate(settings);
}

int s64_settings_exchange_prepare(S64SettingsExchange* settings)
{
    if (settings == 0 || !s64_settings_exchange_values_valid(settings)) {
        return 0;
    }

    settings->magic[0] = S64_SETTINGS_MAGIC_0;
    settings->magic[1] = S64_SETTINGS_MAGIC_1;
    settings->magic[2] = S64_SETTINGS_MAGIC_2;
    settings->magic[3] = S64_SETTINGS_MAGIC_3;
    settings->version = S64_SETTINGS_VERSION;
    settings->checksum = s64_settings_exchange_checksum(settings);

    return 1;
}

int s64_settings_exchange_store(S64SettingsExchange* settings)
{
    if (!s64_settings_exchange_prepare(settings)) return 0;
    return s64_reu_store(
        S64_REU_SETTINGS_BANK,
        S64_REU_SETTINGS_BASE,
        settings,
        (uint16_t)sizeof(*settings)
    );
}

const char* s64_settings_exchange_result_message(uint8_t result)
{
    switch (result) {
        case S64_SETTINGS_RESULT_OK:
            return "Settings loaded";
        case S64_SETTINGS_RESULT_NOT_FOUND:
            return "Publishing not configured";
        case S64_SETTINGS_RESULT_IO_ERROR:
            return "Settings storage unavailable";
        case S64_SETTINGS_RESULT_INVALID_FILE:
        default:
            return "Publisher settings invalid";
    }
}
