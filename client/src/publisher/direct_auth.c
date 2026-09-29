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

static char hex_digit(uint8_t value)
{
    value &= 0x0fu;
    return (char)(value < 10u ? '0' + value : 'a' + (value - 10u));
}

int s64_direct_key_parse(
    const char* hex,
    uint8_t key[S64_HMAC_SHA256_KEY_SIZE]
)
{
    uint8_t high;
    uint8_t low;
    uint8_t i;

    if (hex == 0 || key == 0) return 0;

    for (i = 0u; i < S64_HMAC_SHA256_KEY_SIZE; ++i) {
        if (!hex_value(hex[(uint16_t)i * 2u], &high)
            || !hex_value(hex[(uint16_t)i * 2u + 1u], &low)) {
            return 0;
        }
        key[i] = (uint8_t)((high << 4) | low);
    }
    return hex[64] == '\0';
}

void s64_direct_digest_hex(
    const uint8_t digest[S64_SHA256_DIGEST_SIZE],
    char output[65]
)
{
    uint8_t i;

    if (digest == 0 || output == 0) return;

    for (i = 0u; i < S64_SHA256_DIGEST_SIZE; ++i) {
        output[(uint16_t)i * 2u] = hex_digit((uint8_t)(digest[i] >> 4));
        output[(uint16_t)i * 2u + 1u] = hex_digit(digest[i]);
    }
    output[64] = '\0';
}

int s64_direct_sign(
    const uint8_t key[S64_HMAC_SHA256_KEY_SIZE],
    const char* challenge,
    const char* method,
    const char* request_uri,
    const uint8_t body_digest[S64_SHA256_DIGEST_SIZE],
    char signature[S64_DIRECT_SIGNATURE_CAPACITY]
)
{
    static const char version[] = "see64you-direct/1\n";
    static const char newline[] = "\n";
    S64HmacSha256 hmac;
    uint8_t digest[S64_SHA256_DIGEST_SIZE];
    char body_hex[65];

    if (key == 0 || challenge == 0 || method == 0
        || request_uri == 0 || body_digest == 0 || signature == 0
        || strlen(challenge) != 32u
        || method[0] == '\0'
        || request_uri[0] != '/') {
        return 0;
    }

    s64_direct_digest_hex(body_digest, body_hex);
    s64_hmac_sha256_init(&hmac, key);
    s64_hmac_sha256_update(&hmac, (const uint8_t*)version, (uint16_t)(sizeof(version) - 1u));
    s64_hmac_sha256_update(&hmac, (const uint8_t*)challenge, 32u);
    s64_hmac_sha256_update(&hmac, (const uint8_t*)newline, 1u);
    s64_hmac_sha256_update(&hmac, (const uint8_t*)method, (uint16_t)strlen(method));
    s64_hmac_sha256_update(&hmac, (const uint8_t*)newline, 1u);
    s64_hmac_sha256_update(&hmac, (const uint8_t*)request_uri, (uint16_t)strlen(request_uri));
    s64_hmac_sha256_update(&hmac, (const uint8_t*)newline, 1u);
    s64_hmac_sha256_update(&hmac, (const uint8_t*)body_hex, 64u);
    s64_hmac_sha256_final(&hmac, digest);
    s64_direct_digest_hex(digest, signature);

    memset(digest, 0, sizeof(digest));
    return 1;
}
