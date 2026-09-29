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

    assert(s64_direct_sign(
        key,
        challenge,
        "GET",
        "/api.php?action=list&type=page",
        body_digest,
        signature
    ));
    assert(strcmp(signature, expected) == 0);

    puts("direct auth signing tests passed");
    return 0;
}
