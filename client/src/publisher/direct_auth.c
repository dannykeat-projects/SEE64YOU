#include "publisher/direct_auth.h"
#include "charset.h"

#include <string.h>

static char hex_digit(uint8_t value)
{
    value &= 0x0fu;
    if (value < 10u) return (char)('0' + value);
    return (char)('a' + (value - 10u));
}

static uint8_t hmac_native_text(
    S64HmacSha256* hmac,
    const char* text,
    uint16_t length
)
{
    uint16_t i;
    uint8_t ascii;

    if (hmac == 0 || text == 0) return 0u;

    for (i = 0u; i < length; ++i) {
        if (!s64_charset_native_to_ascii((uint8_t)text[i], &ascii)) {
            return 0u;
        }
        s64_hmac_sha256_update(hmac, &ascii, 1u);
    }
    return 1u;
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
    if (!hmac_native_text(&hmac, version, (uint16_t)(sizeof(version) - 1u))
        || !hmac_native_text(&hmac, challenge, 32u)
        || !hmac_native_text(&hmac, newline, 1u)
        || !hmac_native_text(&hmac, method, (uint16_t)strlen(method))
        || !hmac_native_text(&hmac, newline, 1u)
        || !hmac_native_text(&hmac, request_uri, (uint16_t)strlen(request_uri))
        || !hmac_native_text(&hmac, newline, 1u)
        || !hmac_native_text(&hmac, body_hex, 64u)) {
        memset(hmac.key, 0, sizeof(hmac.key));
        return 0;
    }
    s64_hmac_sha256_final(&hmac, digest);
    s64_direct_digest_hex(digest, signature);

    memset(digest, 0, sizeof(digest));
    return 1;
}
