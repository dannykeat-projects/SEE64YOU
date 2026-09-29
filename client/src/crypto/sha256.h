#ifndef SEE64YOU_CRYPTO_SHA256_H
#define SEE64YOU_CRYPTO_SHA256_H

#include <stdint.h>

#define S64_SHA256_DIGEST_SIZE 32u
#define S64_SHA256_BLOCK_SIZE 64u
#define S64_HMAC_SHA256_KEY_SIZE 32u

typedef struct {
    uint32_t state[8];
    uint32_t length;
    uint8_t buffer[S64_SHA256_BLOCK_SIZE];
    uint8_t used;
} S64Sha256;

typedef struct {
    S64Sha256 inner;
    uint8_t key[S64_HMAC_SHA256_KEY_SIZE];
} S64HmacSha256;

void s64_sha256_init(S64Sha256* context);
void s64_sha256_update(S64Sha256* context, const uint8_t* data, uint16_t length);
void s64_sha256_final(S64Sha256* context, uint8_t digest[S64_SHA256_DIGEST_SIZE]);

void s64_hmac_sha256_init(
    S64HmacSha256* context,
    const uint8_t key[S64_HMAC_SHA256_KEY_SIZE]
);
void s64_hmac_sha256_update(
    S64HmacSha256* context,
    const uint8_t* data,
    uint16_t length
);
void s64_hmac_sha256_final(
    S64HmacSha256* context,
    uint8_t digest[S64_SHA256_DIGEST_SIZE]
);

#endif
