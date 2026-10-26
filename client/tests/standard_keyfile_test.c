#include "publisher/standard_keyfile.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int check(const char* text, S64PublishKeyFile* k)
{
    return s64_publish_keyfile_parse((const uint8_t*)text,
        (uint16_t)strlen(text), k);
}

int main(void)
{
    static const char good[] =
        "8tml-publish: 1\r\n"
        "url: http://example.com/publish\r\n"
        "id: anna-1\r\n"
        "extension: ignored\r\n"
        "key: 000102030405060708090a0b0c0d0e0f"
             "101112131415161718191a1b1c1d1e1f\r\n";
    S64PublishKeyFile file;
    assert(check(good, &file));
    assert(strcmp(file.url, "http://example.com/publish") == 0);
    assert(strcmp(file.id, "anna-1") == 0);
    assert(strlen(file.key) == 64u);
    assert(!check("8tml-publish: 1\nurl: http://example.com/publish\nid: anna-1\n", &file));
    assert(file.key[0] == 0);
    assert(!check("8tml-publish: 2\nurl: http://example.com/publish\nid: anna-1\nkey: 00\n", &file));
    assert(!check("8tml-publish: 1\nurl: http://example.com/publish\nurl: http://other/publish\nid: anna-1\nkey: 00\n", &file));
    assert(!check("8tml-publish: 1\nurl: http://@example.com/publish\nid: anna-1\nkey: 00\n", &file));
    assert(!check("8tml-publish: 1\nurl: http://example.com/publish\nid: ANNA\nkey: 00\n", &file));
    puts("standard key-file tests passed");
    return 0;
}
