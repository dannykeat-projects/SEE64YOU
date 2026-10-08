#include "publisher/persistent_services.h"

#include "buffer.h"
#include "charset.h"

#include <limits.h>
#include <string.h>

#define S64_WIRE_PHASE_VERSION 0u
#define S64_WIRE_PHASE_HEADERS 1u
#define S64_WIRE_PHASE_BODY 2u

#define S64_WIRE_SEEN_TYPE   0x01u
#define S64_WIRE_SEEN_SLUG   0x02u
#define S64_WIRE_SEEN_TITLE  0x04u
#define S64_WIRE_SEEN_STATUS 0x08u
#define S64_WIRE_SEEN_DATE   0x10u
#define S64_WIRE_SEEN_ORDER  0x20u
#define S64_WIRE_SEEN_REQUIRED 0x3fu

static uint8_t copy_text(char* output, uint16_t capacity, const char* text)
{
    uint16_t length;

    if (!output || !text || !capacity) return 0u;
    length = (uint16_t)strlen(text);
    if (length >= capacity) return 0u;
    memcpy(output, text, length + 1u);
    return 1u;
}

static uint8_t parse_order(const char* text, int32_t* output)
{
    uint8_t negative = 0u;
    uint8_t digits = 0u;
    uint8_t digit;
    uint32_t value = 0u;
    uint32_t limit;

    if (!text || !output) return 0u;
    if (*text == '-') {
        negative = 1u;
        ++text;
    } else if (*text == '+') {
        ++text;
    }

    limit = negative ? 2147483648UL : 2147483647UL;
    while (*text >= '0' && *text <= '9') {
        digit = (uint8_t)(*text - '0');
        if (value > (limit - digit) / 10u) return 0u;
        value = value * 10u + digit;
        ++digits;
        ++text;
    }
    if (!digits || *text) return 0u;

    if (negative) {
        *output = value == 2147483648UL
            ? (int32_t)INT32_MIN
            : -(int32_t)value;
    } else {
        *output = (int32_t)value;
    }
    return 1u;
}

static uint8_t parse_header(S64DocumentWireReader* reader)
{
    char* colon;
    const char* value;
    uint8_t bit = 0u;

    colon = strchr(reader->line, ':');
    if (!colon || colon[1] != ' ') return 0u;
    *colon = 0;
    value = colon + 2;

    if (!strcmp(reader->line, "Type")) {
        bit = S64_WIRE_SEEN_TYPE;
        if (!strcmp(value, "page")) reader->document->type = S64_DOCUMENT_PAGE;
        else if (!strcmp(value, "post")) reader->document->type = S64_DOCUMENT_POST;
        else return 0u;
    } else if (!strcmp(reader->line, "Slug")) {
        bit = S64_WIRE_SEEN_SLUG;
        if (!s64_persistent_document_slug_valid(value)
            || !copy_text(reader->document->slug,
                S64_DOCUMENT_SLUG_CAPACITY, value)) {
            return 0u;
        }
    } else if (!strcmp(reader->line, "Title")) {
        bit = S64_WIRE_SEEN_TITLE;
        if (!*value || !copy_text(reader->document->title,
            S64_DOCUMENT_TITLE_CAPACITY, value)) {
            return 0u;
        }
    } else if (!strcmp(reader->line, "Status")) {
        bit = S64_WIRE_SEEN_STATUS;
        if (!strcmp(value, "draft")) reader->document->status = S64_DOCUMENT_DRAFT;
        else if (!strcmp(value, "published")) {
            reader->document->status = S64_DOCUMENT_PUBLISHED;
        } else return 0u;
    } else if (!strcmp(reader->line, "Date")) {
        bit = S64_WIRE_SEEN_DATE;
        if (!copy_text(reader->document->date,
            S64_DOCUMENT_DATE_CAPACITY, value)) {
            return 0u;
        }
    } else if (!strcmp(reader->line, "Order")) {
        bit = S64_WIRE_SEEN_ORDER;
        if (!parse_order(value, &reader->document->order)) return 0u;
    } else {
        return 1u;
    }

    if (reader->seen & bit) return 0u;
    reader->seen = (uint8_t)(reader->seen | bit);
    return 1u;
}

static uint8_t finish_line(S64DocumentWireReader* reader)
{
    reader->line[reader->line_length] = 0;

    if (reader->phase == S64_WIRE_PHASE_VERSION) {
        if (strcmp(reader->line, "SEE64YOU/1")) return 0u;
        reader->phase = S64_WIRE_PHASE_HEADERS;
    } else if (reader->phase == S64_WIRE_PHASE_HEADERS) {
        if (!reader->line_length) {
            if (reader->seen != S64_WIRE_SEEN_REQUIRED) return 0u;
            reader->phase = S64_WIRE_PHASE_BODY;
        } else if (!parse_header(reader)) {
            return 0u;
        }
    }

    reader->line_length = 0u;
    return 1u;
}

void s64_persistent_document_wire_reader_init(
    S64DocumentWireReader* reader,
    S64Document* document)
{
    if (!reader) return;

    memset(reader, 0, sizeof(*reader));
    reader->document = document;
    reader->result = S64_DOCUMENT_OK;

    if (!document || !document->body) {
        reader->result = S64_DOCUMENT_INVALID_ARGUMENT;
        return;
    }

    s64_buffer_init(document->body);
    document->type = S64_DOCUMENT_PAGE;
    document->slug[0] = 0;
    document->title[0] = 0;
    document->status = S64_DOCUMENT_DRAFT;
    document->date[0] = 0;
    document->order = 0;
}

uint8_t s64_persistent_document_wire_reader_write(
    const uint8_t* data,
    uint16_t length,
    void* context)
{
    S64DocumentWireReader* reader = (S64DocumentWireReader*)context;
    uint16_t i;
    uint8_t native;

    if (!reader || (length && !data) || reader->result != S64_DOCUMENT_OK) {
        return 0u;
    }

    for (i = 0u; i < length; ++i) {
        if (!s64_charset_ascii_to_native(data[i], &native)) {
            reader->result = S64_DOCUMENT_INVALID_METADATA;
            return 0u;
        }

        if (reader->phase == S64_WIRE_PHASE_BODY) {
            if (!s64_buffer_insert(reader->document->body, native)) {
                reader->result = S64_DOCUMENT_SINK_ERROR;
                return 0u;
            }
            continue;
        }

        if (native == '\r') continue;
        if (native == '\n') {
            if (!finish_line(reader)) {
                reader->result = S64_DOCUMENT_INVALID_METADATA;
                return 0u;
            }
            continue;
        }

        if (reader->line_length + 1u >= S64_DOCUMENT_WIRE_LINE_CAPACITY) {
            reader->result = S64_DOCUMENT_INVALID_METADATA;
            return 0u;
        }
        reader->line[reader->line_length++] = (char)native;
    }
    return 1u;
}

S64DocumentResult s64_persistent_document_wire_reader_finish(
    S64DocumentWireReader* reader)
{
    if (!reader || !reader->document || !reader->document->body) {
        return S64_DOCUMENT_INVALID_ARGUMENT;
    }

    if (reader->result == S64_DOCUMENT_OK
        && reader->phase != S64_WIRE_PHASE_BODY) {
        reader->result = S64_DOCUMENT_INVALID_METADATA;
    }

    if (reader->result != S64_DOCUMENT_OK) {
        s64_buffer_init(reader->document->body);
        reader->document->type = S64_DOCUMENT_PAGE;
        reader->document->slug[0] = 0;
        reader->document->title[0] = 0;
        reader->document->status = S64_DOCUMENT_DRAFT;
        reader->document->date[0] = 0;
        reader->document->order = 0;
        return reader->result;
    }

    (void)s64_buffer_move_cursor(reader->document->body, 0u);
    return S64_DOCUMENT_OK;
}
