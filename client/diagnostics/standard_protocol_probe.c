/* Native charset coverage: wire bytes must not use cc65 control literals. */
#include <stdint.h>
#include <string.h>
#include "charset.h"
#include "publisher/standard_keyfile.h"
#include "publisher/standard_publish.h"
#include "publisher/standard_reply.h"
#include "publisher/standard_request.h"

static uint8_t bytes[512], key[32];
static S64PublishKeyFile file;
static S64PublishSigner signer;
static S64PublishReplyReader reader;
static char signature[65], path[256];
#define RESULT (*(volatile uint8_t*)0xc061)
#define DETAIL (*(volatile uint8_t*)0xc062)
#define CHECK(test, number) do { if (!(test)) { DETAIL = number; RESULT = 0x5a; for (;;) {} } } while (0)
static uint16_t wire(const char* native)
{
    uint16_t i;
    for (i = 0u; native[i]; ++i)
        if (!s64_charset_native_to_ascii((uint8_t)native[i], &bytes[i])) return 0u;
    return i;
}
int main(void)
{
    uint16_t length;
    uint8_t i;
    RESULT = DETAIL = 0u;
    length = wire("8tml-publish: 1\nurl: http://example.com/publish\nid: test-1\nkey: 000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f\n");
    CHECK(s64_publish_keyfile_parse(bytes, length, &file), 1);
    CHECK(strcmp(file.id, "test-1") == 0, 2);
    CHECK(strcmp(file.url, "http://example.com/publish") == 0, 3);
    length = wire("8tml-publish: 1\r\nurl: http://example.com/publish\r\nid: test-1\r\nkey: 000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f\r\n");
    CHECK(s64_publish_keyfile_parse(bytes, length, &file), 4);
    length = wire("---\n8tml: 1\n---\n# Hello\n");
    CHECK(length == 24u, 5);
    for (i = 0u; i < 32u; ++i) key[i] = i;
    CHECK(s64_publish_sign_begin(&signer, key, "put", "test-1", "00112233445566778899aabbccddeeff", "index.8tml", length), 6);
    CHECK(s64_publish_sign_update(&signer, bytes, length), 7);
    CHECK(s64_publish_sign_finish(&signer, signature), 8);
    CHECK(strcmp(signature, "f61702e908e31462519ef7a1a89b5fa9b723524deb5e7d20806dbc5a257e62db") == 0, 9);
    CHECK(s64_publish_sign_begin(&signer, key, "list", "test-1", "00112233445566778899aabbccddeeff", "", 0u), 10);
    CHECK(s64_publish_sign_finish(&signer, signature), 11);
    CHECK(strcmp(signature, "91b810d221b4176e501565e1fb910f1cba860000a74e6fc5d6db218d6166d512") == 0, 12);
    length = wire("status: ok\napi: 1\n");
    s64_publish_reply_init(&reader);
    CHECK(s64_publish_reply_feed(&reader, bytes, length, 0, 0), 13);
    CHECK(s64_publish_reply_finish(&reader) && !reader.is_error, 14);
    CHECK(s64_publish_request_path(path, sizeof(path), "/publish", S64_PUBLISH_REQUEST_PUT, "test-1", "index.8tml", 24u, "00112233445566778899aabbccddeeff"), 15);
    CHECK(strcmp(path, "/publish?op=put&id=test-1&name=index.8tml&size=24&c=00112233445566778899aabbccddeeff") == 0, 16);
    RESULT = 0xa5;
    for (;;) {}
    return 0;
}
