#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "crypto/sha256.h"
#include "publisher/direct_auth.h"

int main(void)
{
    static const char key_hex[] =
        "000102030405060708090a0b0c0d0e0f"
        "101112131415161718191a1b1c1d1e1f";
    static const char challenge[] = "0123456789abcdef0123456789abcdef";
    static const char expected[] =
        "7d6554f87377e7ccbb1fec868b56b9e3"
        "2107d9c39c480c1780aad061298dc19a";
    uint8_t key[32];
    uint8_t key_before[32];
    uint8_t body_digest[32];
    char signature[65];
    S64Sha256 sha;

    assert(s64_direct_key_parse(key_hex, key));
    memcpy(key_before, key, sizeof(key));
    assert(!s64_direct_key_parse("bad", key));
    assert(memcmp(key, key_before, sizeof(key)) == 0);

    s64_sha256_init(&sha);
    s64_sha256_final(&sha, body_digest);
    {
        static const uint8_t expected_empty[32] = {
            0xe3,0xb0,0xc4,0x42,0x98,0xfc,0x1c,0x14,
            0x9a,0xfb,0xf4,0xc8,0x99,0x6f,0xb9,0x24,
            0x27,0xae,0x41,0xe4,0x64,0x9b,0x93,0x4c,
            0xa4,0x95,0x99,0x1b,0x78,0x52,0xb8,0x55
        };
        assert(memcmp(body_digest, expected_empty, sizeof(body_digest)) == 0);
    }

    {
        static const char canonical[] =
            "see64you-direct/1\n"
            "0123456789abcdef0123456789abcdef\n"
            "GET\n"
            "/api.php?action=list&type=page\n"
            "e3b0c44298fc1c149afbf4c8996fb924"
            "27ae41e4649b934ca495991b7852b855";
        S64HmacSha256 direct_hmac;
        uint8_t direct_digest[32];
        char direct_hex[65];

        s64_hmac_sha256_init(&direct_hmac, key);
        s64_hmac_sha256_update(
            &direct_hmac,
            (const uint8_t*)canonical,
            (uint16_t)(sizeof(canonical) - 1u)
        );
        s64_hmac_sha256_final(&direct_hmac, direct_digest);
        s64_direct_digest_hex(direct_digest, direct_hex);
        if (strcmp(direct_hex, expected) != 0) {
            fprintf(stderr, "one-shot got: %s\n", direct_hex);
            fprintf(stderr, "one-shot exp: %s\n", expected);
            assert(0);
        }
    }

    assert(s64_direct_sign(
        key,
        challenge,
        "GET",
        "/api.php?action=list&type=page",
        body_digest,
        signature
    ));
    if (strcmp(signature, expected) != 0) {
        fprintf(stderr, "signature got: %s\n", signature);
        fprintf(stderr, "signature exp: %s\n", expected);
        assert(0);
    }

    puts("direct auth signing tests passed");
    return 0;
}
