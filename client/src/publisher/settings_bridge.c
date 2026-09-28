#include "publisher/settings_bridge.h"

#include "platform/reu.h"
#include "platform/reu_layout.h"

#include <stddef.h>
#include <string.h>

#define S64_SETTINGS_MAGIC_0 'S'
#define S64_SETTINGS_MAGIC_1 '6'
#define S64_SETTINGS_MAGIC_2 '4'
#define S64_SETTINGS_MAGIC_3 'S'

int s64_settings_bridge_load_ready(S64SettingsExchange* settings)
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

    return settings->magic[0] == S64_SETTINGS_MAGIC_0
        && settings->magic[1] == S64_SETTINGS_MAGIC_1
        && settings->magic[2] == S64_SETTINGS_MAGIC_2
        && settings->magic[3] == S64_SETTINGS_MAGIC_3
        && settings->version == S64_SETTINGS_VERSION
        && settings->state == S64_SETTINGS_STATE_READY
        && settings->checksum != 0u
        && settings->destination_type <= (uint8_t)S64_DESTINATION_WORDPRESS
        && settings->allow_insecure_http <= 1u
        && settings->base_url[S64_DESTINATION_BASE_URL_CAPACITY - 1u] == '\0'
        && settings->relay_key[S64_RELAY_KEY_CAPACITY - 1u] == '\0';
}

int s64_settings_bridge_request_load(S64SettingsExchange* settings)
{
    if (settings == 0) return 0;

    memset(settings, 0, sizeof(*settings));
    settings->magic[0] = S64_SETTINGS_MAGIC_0;
    settings->magic[1] = S64_SETTINGS_MAGIC_1;
    settings->magic[2] = S64_SETTINGS_MAGIC_2;
    settings->magic[3] = S64_SETTINGS_MAGIC_3;
    settings->version = S64_SETTINGS_VERSION;
    settings->state = S64_SETTINGS_STATE_LOAD_REQUEST;
    settings->destination_type = (uint8_t)S64_DESTINATION_NONE;
    settings->checksum = 0u;

    return s64_reu_store(
        S64_REU_SETTINGS_BANK,
        S64_REU_SETTINGS_BASE,
        settings,
        (uint16_t)sizeof(*settings)
    );
}

int s64_settings_bridge_request_edit(void)
{
    uint8_t state = S64_SETTINGS_STATE_EDIT_REQUEST;
    uint16_t checksum = 0u;

    return s64_reu_store(
            S64_REU_SETTINGS_BANK,
            (uint16_t)(S64_REU_SETTINGS_BASE
                + offsetof(S64SettingsExchange, state)),
            &state,
            1u
        )
        && s64_reu_store(
            S64_REU_SETTINGS_BANK,
            (uint16_t)(S64_REU_SETTINGS_BASE
                + offsetof(S64SettingsExchange, checksum)),
            &checksum,
            (uint16_t)sizeof(checksum)
        );
}
