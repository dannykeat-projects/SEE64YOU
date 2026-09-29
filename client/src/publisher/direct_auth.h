#ifndef SEE64YOU_PUBLISHER_DIRECT_AUTH_H
#define SEE64YOU_PUBLISHER_DIRECT_AUTH_H

#include <stdint.h>

#include "crypto/sha256.h"

#define S64_DIRECT_KEY_HEX_CAPACITY 65u
#define S64_DIRECT_CHALLENGE_CAPACITY 33u
#define S64_DIRECT_SIGNATURE_CAPACITY 65u

int s64_direct_key_parse(
    const char* hex,
    uint8_t key[S64_HMAC_SHA256_KEY_SIZE]
);

void s64_direct_digest_hex(
    const uint8_t digest[S64_SHA256_DIGEST_SIZE],
    char output[65]
);

int s64_direct_sign(
    const uint8_t key[S64_HMAC_SHA256_KEY_SIZE],
    const char* challenge,
    const char* method,
    const char* request_uri,
    const uint8_t body_digest[S64_SHA256_DIGEST_SIZE],
    char signature[S64_DIRECT_SIGNATURE_CAPACITY]
);

#endif
