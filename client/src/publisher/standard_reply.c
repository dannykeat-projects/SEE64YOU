#include "publisher/standard_reply.h"
#include "charset.h"

#include <string.h>

void s64_publish_reply_init(S64PublishReplyReader* reader)
{
    if (reader) memset(reader, 0, sizeof(*reader));
}

static int complete_line(S64PublishReplyReader* reader,
    S64PublishReplyLine on_line, void* context)
{
    char* cursor;
    char* value;
    uint16_t i;
    reader->line[reader->length] = 0;
    cursor = strchr(reader->line, ':');
    if (!cursor || cursor == reader->line || cursor[1] != ' ') return 0;
    *cursor++ = 0;
    ++cursor;
    value = cursor;
    if (!*value) return 0;
    for (i = 0u; reader->line[i]; ++i) {
        char ch = reader->line[i];
        if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')
            || ch == '-')) return 0;
    }
    if (!reader->saw_status) {
        if (strcmp(reader->line, "status") != 0) return 0;
        if (strcmp(value, "ok") == 0) reader->is_error = 0u;
        else if (strcmp(value, "error") == 0) reader->is_error = 1u;
        else return 0;
        reader->saw_status = 1u;
    } else if (strcmp(reader->line, "status") == 0
        || (on_line && !on_line(reader->line, value, context))) return 0;
    reader->length = 0u;
    return 1;
}

int s64_publish_reply_feed(S64PublishReplyReader* reader,
    const uint8_t* bytes, uint16_t length,
    S64PublishReplyLine on_line, void* context)
{
    uint16_t i;
    uint8_t converted;
    if (!reader || reader->failed || (length && !bytes)
        || (uint32_t)length > S64_PUBLISH_REPLY_MAX_BYTES - reader->total)
        goto invalid;
    for (i = 0u; i < length; ++i) {
        if (bytes[i] == 10u) {
            if (!complete_line(reader, on_line, context)) goto invalid;
        } else {
            if (reader->length >= S64_PUBLISH_REPLY_LINE_CAPACITY - 1u
                || bytes[i] < 32u || bytes[i] > 126u
                || !s64_charset_ascii_to_native(bytes[i], &converted))
                goto invalid;
            reader->line[reader->length++] = (char)converted;
        }
    }
    reader->total += length;
    return 1;
invalid:
    if (reader) reader->failed = 1u;
    return 0;
}

int s64_publish_reply_finish(const S64PublishReplyReader* reader)
{
    return reader && !reader->failed && reader->saw_status
        && reader->length == 0u;
}
