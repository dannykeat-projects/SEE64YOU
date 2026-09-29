#include "crypto/sha256.h"

#include <string.h>

/*
 * Compact, non-reentrant SHA-256 workspace. See64You runs crypto on one
 * foreground request at a time, so sharing this 96-byte transform workspace
 * avoids putting a 64-word schedule on the C64 software stack.
 */
static uint32_t schedule[16];
static uint32_t work[8];

static const uint32_t constants[64] = {
    0x428a2f98UL,0x71374491UL,0xb5c0fbcfUL,0xe9b5dba5UL,
    0x3956c25bUL,0x59f111f1UL,0x923f82a4UL,0xab1c5ed5UL,
    0xd807aa98UL,0x12835b01UL,0x243185beUL,0x550c7dc3UL,
    0x72be5d74UL,0x80deb1feUL,0x9bdc06a7UL,0xc19bf174UL,
    0xe49b69c1UL,0xefbe4786UL,0x0fc19dc6UL,0x240ca1ccUL,
    0x2de92c6fUL,0x4a7484aaUL,0x5cb0a9dcUL,0x76f988daUL,
    0x983e5152UL,0xa831c66dUL,0xb00327c8UL,0xbf597fc7UL,
    0xc6e00bf3UL,0xd5a79147UL,0x06ca6351UL,0x14292967UL,
    0x27b70a85UL,0x2e1b2138UL,0x4d2c6dfcUL,0x53380d13UL,
    0x650a7354UL,0x766a0abbUL,0x81c2c92eUL,0x92722c85UL,
    0xa2bfe8a1UL,0xa81a664bUL,0xc24b8b70UL,0xc76c51a3UL,
    0xd192e819UL,0xd6990624UL,0xf40e3585UL,0x106aa070UL,
    0x19a4c116UL,0x1e376c08UL,0x2748774cUL,0x34b0bcb5UL,
    0x391c0cb3UL,0x4ed8aa4aUL,0x5b9cca4fUL,0x682e6ff3UL,
    0x748f82eeUL,0x78a5636fUL,0x84c87814UL,0x8cc70208UL,
    0x90befffaUL,0xa4506cebUL,0xbef9a3f7UL,0xc67178f2UL
};

static uint32_t rotr(uint32_t value, uint8_t count)
{
    return (value >> count) | (value << (32u - count));
}

static uint32_t load_be32(const uint8_t* data)
{
    return ((uint32_t)data[0] << 24)
        | ((uint32_t)data[1] << 16)
        | ((uint32_t)data[2] << 8)
        | (uint32_t)data[3];
}

static void store_be32(uint8_t* data, uint32_t value)
{
    data[0] = (uint8_t)(value >> 24);
    data[1] = (uint8_t)(value >> 16);
    data[2] = (uint8_t)(value >> 8);
    data[3] = (uint8_t)value;
}

static void transform(S64Sha256* context, const uint8_t block[64])
{
    uint8_t i;
    uint32_t s0;
    uint32_t s1;
    uint32_t choice;
    uint32_t majority;
    uint32_t temp1;
    uint32_t temp2;

    for (i = 0u; i < 8u; ++i) {
        work[i] = context->state[i];
    }

    for (i = 0u; i < 64u; ++i) {
        if (i < 16u) {
            schedule[i] = load_be32(block + ((uint16_t)i * 4u));
        } else {
            s0 = rotr(schedule[(uint8_t)((i - 15u) & 15u)], 7u)
                ^ rotr(schedule[(uint8_t)((i - 15u) & 15u)], 18u)
                ^ (schedule[(uint8_t)((i - 15u) & 15u)] >> 3);
            s1 = rotr(schedule[(uint8_t)((i - 2u) & 15u)], 17u)
                ^ rotr(schedule[(uint8_t)((i - 2u) & 15u)], 19u)
                ^ (schedule[(uint8_t)((i - 2u) & 15u)] >> 10);
            schedule[i & 15u] = schedule[(uint8_t)((i - 16u) & 15u)]
                + s0
                + schedule[(uint8_t)((i - 7u) & 15u)]
                + s1;
        }

        s1 = rotr(work[4], 6u) ^ rotr(work[4], 11u) ^ rotr(work[4], 25u);
        choice = (work[4] & work[5]) ^ ((~work[4]) & work[6]);
        temp1 = work[7] + s1 + choice + constants[i] + schedule[i & 15u];
        s0 = rotr(work[0], 2u) ^ rotr(work[0], 13u) ^ rotr(work[0], 22u);
        majority = (work[0] & work[1]) ^ (work[0] & work[2]) ^ (work[1] & work[2]);
        temp2 = s0 + majority;

        work[7] = work[6];
        work[6] = work[5];
        work[5] = work[4];
        work[4] = work[3] + temp1;
        work[3] = work[2];
        work[2] = work[1];
        work[1] = work[0];
        work[0] = temp1 + temp2;
    }

    for (i = 0u; i < 8u; ++i) {
        context->state[i] += work[i];
    }
}

void s64_sha256_init(S64Sha256* context)
{
    if (context == 0) return;

    context->state[0] = 0x6a09e667UL;
    context->state[1] = 0xbb67ae85UL;
    context->state[2] = 0x3c6ef372UL;
    context->state[3] = 0xa54ff53aUL;
    context->state[4] = 0x510e527fUL;
    context->state[5] = 0x9b05688cUL;
    context->state[6] = 0x1f83d9abUL;
    context->state[7] = 0x5be0cd19UL;
    context->length = 0u;
    context->used = 0u;
}

void s64_sha256_update(S64Sha256* context, const uint8_t* data, uint16_t length)
{
    uint16_t take;

    if (context == 0 || (length != 0u && data == 0)) return;

    context->length += length;

    while (length != 0u) {
        take = (uint16_t)(S64_SHA256_BLOCK_SIZE - context->used);
        if (take > length) take = length;

        memcpy(context->buffer + context->used, data, take);
        context->used = (uint8_t)(context->used + take);
        data += take;
        length = (uint16_t)(length - take);

        if (context->used == S64_SHA256_BLOCK_SIZE) {
            transform(context, context->buffer);
            context->used = 0u;
        }
    }
}

void s64_sha256_final(S64Sha256* context, uint8_t digest[S64_SHA256_DIGEST_SIZE])
{
    uint32_t bit_length;
    uint8_t i;

    if (context == 0 || digest == 0) return;

    bit_length = context->length << 3;
    context->buffer[context->used++] = 0x80u;

    if (context->used > 56u) {
        while (context->used < 64u) context->buffer[context->used++] = 0u;
        transform(context, context->buffer);
        context->used = 0u;
    }

    while (context->used < 56u) context->buffer[context->used++] = 0u;
    memset(context->buffer + 56u, 0, 4u);
    store_be32(context->buffer + 60u, bit_length);
    transform(context, context->buffer);

    for (i = 0u; i < 8u; ++i) {
        store_be32(digest + ((uint16_t)i * 4u), context->state[i]);
    }
}

void s64_hmac_sha256_init(
    S64HmacSha256* context,
    const uint8_t key[S64_HMAC_SHA256_KEY_SIZE]
)
{
    uint8_t i;
    uint8_t byte;

    if (context == 0 || key == 0) return;

    memcpy(context->key, key, S64_HMAC_SHA256_KEY_SIZE);
    s64_sha256_init(&context->inner);

    for (i = 0u; i < 64u; ++i) {
        byte = (uint8_t)((i < S64_HMAC_SHA256_KEY_SIZE ? key[i] : 0u) ^ 0x36u);
        s64_sha256_update(&context->inner, &byte, 1u);
    }
}

void s64_hmac_sha256_update(
    S64HmacSha256* context,
    const uint8_t* data,
    uint16_t length
)
{
    if (context == 0) return;
    s64_sha256_update(&context->inner, data, length);
}

void s64_hmac_sha256_final(
    S64HmacSha256* context,
    uint8_t digest[S64_SHA256_DIGEST_SIZE]
)
{
    uint8_t inner_digest[S64_SHA256_DIGEST_SIZE];
    uint8_t i;
    uint8_t byte;
    S64Sha256 outer;

    if (context == 0 || digest == 0) return;

    s64_sha256_final(&context->inner, inner_digest);
    s64_sha256_init(&outer);

    for (i = 0u; i < 64u; ++i) {
        byte = (uint8_t)((i < S64_HMAC_SHA256_KEY_SIZE ? context->key[i] : 0u) ^ 0x5cu);
        s64_sha256_update(&outer, &byte, 1u);
    }
    s64_sha256_update(&outer, inner_digest, S64_SHA256_DIGEST_SIZE);
    s64_sha256_final(&outer, digest);

    memset(inner_digest, 0, sizeof(inner_digest));
    memset(context->key, 0, sizeof(context->key));
}
