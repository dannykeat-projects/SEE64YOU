#include "publisher/direct_auth.h"

#include <string.h>

static uint8_t hex_value(char value, uint8_t* result)
{
    if (value >= '0' && value <= '9') {
        *result = (uint8_t)(value - '0');
        return 1u;
    }
    if (value >= 'a' && value <= 'f') {
        *result = (uint8_t)(value - 'a' + 10);
        return 1u;
    }
    if (value >= 'A' && value <= 'F') {
        *result = (uint8_t)(value - 'A' + 10);
        return 1u;
    }
    return 0u;
}

int s64_direct_key_parse(
    const char* hex,
    uint8_t key[S64_HMAC_SHA256_KEY_SIZE])
{
    uint8_t high;
    uint8_t low;
    uint8_t i;

    if (!hex || !key || strlen(hex) != 64u) return 0;

    for (i = 0u; i < 64u; ++i) {
        if (!hex_value(hex[i], &high)) return 0;
    }

    for (i = 0u; i < S64_HMAC_SHA256_KEY_SIZE; ++i) {
        (void)hex_value(hex[(uint16_t)i * 2u], &high);
        (void)hex_value(hex[(uint16_t)i * 2u + 1u], &low);
        key[i] = (uint8_t)((high << 4) | low);
    }
    return 1;
}
