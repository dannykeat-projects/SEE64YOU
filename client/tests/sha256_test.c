#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "crypto/sha256.h"

static uint8_t from_hex(char c)
{
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
    return (uint8_t)(c - 'A' + 10);
}

static void expect_hex(const uint8_t* bytes, uint16_t length, const char* hex)
{
    uint16_t i;
    for (i = 0u; i < length; ++i) {
        uint8_t expected = (uint8_t)((from_hex(hex[i * 2u]) << 4) | from_hex(hex[i * 2u + 1u]));
        assert(bytes[i] == expected);
    }
}

int main(void)
{
    static const uint8_t abc[] = {'a','b','c'};
    uint8_t key[32];
    uint8_t digest[32];
    uint8_t i;
    S64Sha256 sha;
    S64HmacSha256 hmac;

    s64_sha256_init(&sha);
    s64_sha256_update(&sha, abc, sizeof(abc));
    s64_sha256_final(&sha, digest);
    expect_hex(
        digest,
        sizeof(digest),
        "ba7816bf8f01cfea414140de5dae2223"
        "b00361a396177a9cb410ff61f20015ad"
    );

    for (i = 0u; i < sizeof(key); ++i) key[i] = i;
    s64_hmac_sha256_init(&hmac, key);
    s64_hmac_sha256_update(&hmac, abc, sizeof(abc));
    s64_hmac_sha256_final(&hmac, digest);
    expect_hex(
        digest,
        sizeof(digest),
        "f0133729c4163dede81e21cd47839256"
        "da58171238c8a0d874397c73b14e1e47"
    );

    {
        uint8_t long_message[80];
        memset(long_message, 'a', sizeof(long_message));

        s64_sha256_init(&sha);
        s64_sha256_update(&sha, long_message, sizeof(long_message));
        s64_sha256_final(&sha, digest);
        expect_hex(
            digest,
            sizeof(digest),
            "0f45e858fbc4176cdf4e411f88281ede"
            "fc390ae5afe7df0f44cd9297f0a64580"
        );

        for (i = 0u; i < sizeof(key); ++i) key[i] = i;
        s64_hmac_sha256_init(&hmac, key);
        s64_hmac_sha256_update(&hmac, long_message, sizeof(long_message));
        s64_hmac_sha256_final(&hmac, digest);
        expect_hex(
            digest,
            sizeof(digest),
            "c50a9b678143cd17bcf142287cb4f604"
            "db7d6bc1f6a9b26b582c0aa79bb20567"
        );
    }

    puts("sha256/hmac tests passed");
    return 0;
}
