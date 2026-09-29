#include "publisher/settings_exchange.h"

#include "platform/reu.h"
#include "platform/reu_layout.h"

#include <stddef.h>
#include <string.h>

#define S64_SETTINGS_MAGIC_0 'S'
#define S64_SETTINGS_MAGIC_1 '6'
#define S64_SETTINGS_MAGIC_2 '4'
#define S64_SETTINGS_MAGIC_3 'S'

static uint8_t text_terminated(const char* value, uint16_t capacity)
{
    uint16_t i;

    if (value == 0 || capacity == 0u) return 0u;
    for (i = 0u; i < capacity; ++i) {
        if (value[i] == '\0') return 1u;
    }
    return 0u;
}

static uint8_t key_valid(const char* value)
{
    uint16_t i;
    char c;

    if (value == 0) return 0u;
    if (value[0] == '\0') return 1u;

    for (i = 0u; i < 64u; ++i) {
        c = value[i];
        if (c == '\0'
            || !((c >= '0' && c <= '9')
                || (c >= 'a' && c <= 'f')
                || (c >= 'A' && c <= 'F'))) {
            return 0u;
        }
    }
    return value[64] == '\0';
}

static uint16_t settings_checksum(const S64SettingsExchange* settings)
{
    const uint8_t* bytes = (const uint8_t*)settings;
    uint8_t sum1 = 0x53u;
    uint8_t sum2 = 0x36u;
    uint16_t i;

    for (i = 0u; i < (uint16_t)offsetof(S64SettingsExchange, checksum); ++i) {
        sum1 = (uint8_t)(sum1 + bytes[i]);
        sum2 = (uint8_t)(sum2 + sum1);
    }
    return (uint16_t)(((uint16_t)sum2 << 8) | sum1);
}

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
    settings->checksum = settings_checksum(settings);
}

int s64_settings_exchange_values_valid(const S64SettingsExchange* settings)
{
    uint8_t type;

    if (settings == 0
        || settings->allow_insecure_http > 1u
        || !text_terminated(
            settings->base_url,
            (uint16_t)sizeof(settings->base_url)
        )
        || !text_terminated(
            settings->relay_key,
            (uint16_t)sizeof(settings->relay_key)
        )
        || !text_terminated(
            settings->direct_key,
            (uint16_t)sizeof(settings->direct_key)
        )
        || !key_valid(settings->relay_key)
        || !key_valid(settings->direct_key)) {
        return 0;
    }

    type = settings->destination_type;
    if (type == (uint8_t)S64_DESTINATION_NONE) {
        return settings->base_url[0] == '\0';
    }
    return (type == (uint8_t)S64_DESTINATION_STANDALONE
            || type == (uint8_t)S64_DESTINATION_WORDPRESS)
        && settings->base_url[0] != '\0';
}

int s64_settings_exchange_validate(const S64SettingsExchange* settings)
{
    if (settings == 0
        || settings->magic[0] != S64_SETTINGS_MAGIC_0
        || settings->magic[1] != S64_SETTINGS_MAGIC_1
        || settings->magic[2] != S64_SETTINGS_MAGIC_2
        || settings->magic[3] != S64_SETTINGS_MAGIC_3
        || settings->version != S64_SETTINGS_VERSION
        || (settings->state != S64_SETTINGS_STATE_LOAD_REQUEST
            && settings->state != S64_SETTINGS_STATE_EDIT_REQUEST
            && settings->state != S64_SETTINGS_STATE_READY)
        || settings->result > S64_SETTINGS_RESULT_INVALID_FILE
        || !s64_settings_exchange_values_valid(settings)) {
        return 0;
    }

    if (settings->checksum == 0u
        && (settings->state == S64_SETTINGS_STATE_LOAD_REQUEST
            || settings->state == S64_SETTINGS_STATE_EDIT_REQUEST)) {
        return 1;
    }
    return settings->checksum == settings_checksum(settings);
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

int s64_settings_exchange_store(S64SettingsExchange* settings)
{
    if (settings == 0 || !s64_settings_exchange_values_valid(settings)) {
        return 0;
    }

    settings->magic[0] = S64_SETTINGS_MAGIC_0;
    settings->magic[1] = S64_SETTINGS_MAGIC_1;
    settings->magic[2] = S64_SETTINGS_MAGIC_2;
    settings->magic[3] = S64_SETTINGS_MAGIC_3;
    settings->version = S64_SETTINGS_VERSION;
    settings->checksum = settings_checksum(settings);

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
