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
    uint8_t body_digest[32];
    char signature[65];
    S64Sha256 sha;

    assert(s64_direct_key_parse(key_hex, key));
    assert(!s64_direct_key_parse("bad", key));

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
