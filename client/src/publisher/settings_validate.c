#include "publisher/settings_exchange.h"

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

const char* s64_settings_publish_id(const S64SettingsExchange* settings)
{
    return settings != 0 && settings->destination_type == S64_DESTINATION_STANDARD
        ? settings->relay_key : "";
}

static uint8_t publish_id_valid(const char* id)
{
    uint8_t i = 0u;
    char ch;
    while ((ch = id[i]) != 0) {
        if (i >= 32u || !((ch >= 'a' && ch <= 'z')
            || (ch >= '0' && ch <= '9') || ch == '-')) return 0u;
        ++i;
    }
    return i != 0u;
}

int s64_settings_publish_url_valid(const char* url)
{
    const char* cursor;
    uint16_t length = 0u, host_length = 0u;
    uint32_t port = 0u;
    if (!url) return 0;
    while (url[length]) {
        if (++length >= S64_DESTINATION_BASE_URL_CAPACITY) return 0;
    }
    if (strncmp(url, "http://", 7u) == 0) cursor = url + 7;
    else if (strncmp(url, "https://", 8u) == 0) cursor = url + 8;
    else return 0;
    while (*cursor && *cursor != '/' && *cursor != ':') {
        char ch = *cursor++;
        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
            || (ch >= '0' && ch <= '9') || ch == '.' || ch == '-')) return 0;
        if (++host_length >= 96u) return 0;
    }
    if (!host_length) return 0;
    if (*cursor == ':') {
        ++cursor;
        if (*cursor < '0' || *cursor > '9') return 0;
        while (*cursor >= '0' && *cursor <= '9') {
            port = port * 10u + (uint8_t)(*cursor++ - '0');
            if (port > 65535u) return 0;
        }
        if (!port) return 0;
    }
    if (*cursor != '/') return 0;
    while (*cursor) {
        char ch = *cursor++;
        /* Native literals work on both host ASCII and cc65 PETSCII. */
        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
            || (ch >= '0' && ch <= '9')
            || strchr("/-._~!$&'()*+,;=:%[]^`{|}", ch) != 0)) return 0;
    }
    return 1;
}

uint16_t s64_settings_exchange_checksum(const S64SettingsExchange* settings)
{
    const uint8_t* bytes = (const uint8_t*)settings;
    uint8_t sum1 = 0x53u;
    uint8_t sum2 = 0x36u;
    uint16_t i;

    if (settings == 0) return 0u;
    for (i = 0u; i < (uint16_t)offsetof(S64SettingsExchange, checksum); ++i) {
        sum1 = (uint8_t)(sum1 + bytes[i]);
        sum2 = (uint8_t)(sum2 + sum1);
    }
    return (uint16_t)(((uint16_t)sum2 << 8) | sum1);
}

int s64_settings_exchange_values_valid(const S64SettingsExchange* settings)
{
    uint8_t type;

    if (settings == 0
        || settings->allow_insecure_http > 1u
        || !text_terminated(settings->base_url,
            (uint16_t)sizeof(settings->base_url))
        || !text_terminated(settings->relay_key,
            (uint16_t)sizeof(settings->relay_key))
        || !text_terminated(settings->direct_key,
            (uint16_t)sizeof(settings->direct_key))
        || (settings->destination_type != S64_DESTINATION_STANDARD
            && !key_valid(settings->relay_key))
        || !key_valid(settings->direct_key)) {
        return 0;
    }

    type = settings->destination_type;
    if (type == S64_DESTINATION_STANDARD) {
        return publish_id_valid(settings->relay_key)
            && settings->direct_key[0] != 0
            && s64_settings_publish_url_valid(settings->base_url);
    }
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
    return settings->checksum == s64_settings_exchange_checksum(settings);
}
