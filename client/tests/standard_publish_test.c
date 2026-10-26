#include "publisher/standard_publish.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    static const uint8_t source[] = "---\n8tml: 1\n---\n# Hello\n";
    static const char challenge[] = "00112233445566778899aabbccddeeff";
    static const char put[] = "f61702e908e31462519ef7a1a89b5fa9b723524deb5e7d20806dbc5a257e62db";
    static const char list[] = "91b810d221b4176e501565e1fb910f1cba860000a74e6fc5d6db218d6166d512";
    uint8_t key[32], i;
    char result[65];
    S64PublishSigner signer;
    for (i = 0; i < 32; ++i) key[i] = i;
    assert(s64_publish_id_valid("test-1"));
    assert(!s64_publish_id_valid("Bad"));
    assert(!s64_publish_id_valid(""));
    assert(s64_publish_name_valid("dir/INDEX.8tml"));
    assert(s64_publish_name_valid("a..b.8tml"));
    assert(!s64_publish_name_valid("/index.8tml"));
    assert(!s64_publish_name_valid("dir//index.8tml"));
    assert(!s64_publish_name_valid("a/../b.8tml"));
    assert(!s64_publish_name_valid("a/./b"));
    assert(!s64_publish_name_valid("a/"));
    assert(!s64_publish_name_valid("a\\b"));
    assert(!s64_publish_name_valid("a?b"));
    assert(s64_publish_sign_begin(&signer, key, "put", "test-1",
        challenge, "index.8tml", 24));
    assert(s64_publish_sign_update(&signer, source, 5));
    assert(!s64_publish_sign_finish(&signer, result));
    assert(!s64_publish_sign_update(&signer, source, 24));
    assert(s64_publish_sign_update(&signer, source + 5, 19));
    assert(s64_publish_sign_finish(&signer, result));
    assert(strcmp(result, put) == 0);
    assert(!s64_publish_sign_finish(&signer, result));
    assert(s64_publish_sign_begin(&signer, key, "list", "test-1",
        challenge, "", 0));
    assert(s64_publish_sign_finish(&signer, result));
    assert(strcmp(result, list) == 0);
    assert(!s64_publish_sign_begin(&signer, key, "list", "test-1",
        challenge, "x", 0));
    assert(!s64_publish_sign_begin(&signer, key, "put", "test-1",
        challenge, "../x", 1));
    puts("standard publishing signing tests passed");
    return 0;
}
