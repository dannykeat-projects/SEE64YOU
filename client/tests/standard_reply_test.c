#include "publisher/standard_reply.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int collect(const char* key, const char* val, void* context)
{
    unsigned int* lines = (unsigned int*)context;
    if (*lines == 0u) {
        assert(strcmp(key, "api") == 0);
        assert(strcmp(val, "1") == 0);
    } else if (*lines == 1u) {
        assert(strcmp(key, "auth") == 0);
        assert(strcmp(val, "hmac https") == 0);
    }
    ++*lines;
    return 1;
}

static int read(const char* text)
{
    S64PublishReplyReader reader;
    s64_publish_reply_init(&reader);
    return s64_publish_reply_feed(&reader, (const uint8_t*)text,
        (uint16_t)strlen(text), 0, 0)
        && s64_publish_reply_finish(&reader);
}

int main(void)
{
    S64PublishReplyReader reader;
    unsigned int lines = 0u;
    const uint8_t part1[] = "status: ok\napi:";
    const uint8_t part2[] = " 1\nauth: hmac https\n";
    s64_publish_reply_init(&reader);
    assert(s64_publish_reply_feed(&reader, part1, sizeof(part1) - 1u,
        collect, &lines));
    assert(s64_publish_reply_feed(&reader, part2, sizeof(part2) - 1u,
        collect, &lines));
    assert(s64_publish_reply_finish(&reader));
    assert(lines == 2u && !reader.is_error);
    assert(read("status: error\nerror: auth\nmessage: Forbidden\n"));
    assert(!read("api: 1\nstatus: ok\n"));
    assert(!read("status: okay\n"));
    assert(!read("status: ok\nstatus: error\n"));
    assert(!read("status: ok\napi: 1"));
    assert(!read("status: ok\nAPi: 1\n"));
    assert(!read("status: ok\r\n"));
    assert(!read("status: ok\nwarning: \n"));
    puts("standard ASCII reply parsing tests passed");
    return 0;
}
