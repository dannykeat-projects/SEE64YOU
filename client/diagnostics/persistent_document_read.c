#include "publisher/persistent_services.h"

#include "buffer.h"
#include "charset.h"

#include <limits.h>
#include <string.h>

#define S64_WIRE_PHASE_VERSION 0u
#define S64_WIRE_PHASE_TYPE 1u
#define S64_WIRE_PHASE_SLUG 2u
#define S64_WIRE_PHASE_TITLE 3u
#define S64_WIRE_PHASE_STATUS 4u
#define S64_WIRE_PHASE_DATE 5u
#define S64_WIRE_PHASE_ORDER 6u
#define S64_WIRE_PHASE_EXTRA 7u
#define S64_WIRE_PHASE_BODY 8u

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

    if (!text || !output) return 0u;
    if (*text == '-') {
        negative = 1u;
        ++text;
    } else if (*text == '+') {
        ++text;
    }

    while (*text >= '0' && *text <= '9') {
        digit = (uint8_t)(*text - '0');
        if (value > 214748364UL
            || (value == 214748364UL
                && digit > (uint8_t)(negative ? 8u : 7u))) {
            return 0u;
        }
        value = value * 10u + digit;
        ++digits;
        ++text;
    }
    if (!digits || *text) return 0u;

    if (negative && value == 2147483648UL) {
        *output = (int32_t)INT32_MIN;
    } else {
        *output = negative ? -(int32_t)value : (int32_t)value;
    }
    return 1u;
}

static uint8_t has_prefix(
    const char* line, const char* prefix, uint8_t length)
{
    return !strncmp(line, prefix, length);
}

static uint8_t finish_line(S64DocumentWireReader* reader)
{
    const char* value;

    reader->line[reader->line_length] = 0;

    switch (reader->phase) {
        case S64_WIRE_PHASE_VERSION:
            if (strcmp(reader->line, "SEE64YOU/1")) return 0u;
            reader->phase = S64_WIRE_PHASE_TYPE;
            break;

        case S64_WIRE_PHASE_TYPE:
            if (!has_prefix(reader->line, "Type: ", 6u)) return 0u;
            value = reader->line + 6;
            if (!strcmp(value, "page")) reader->document->type = S64_DOCUMENT_PAGE;
            else if (!strcmp(value, "post")) reader->document->type = S64_DOCUMENT_POST;
            else return 0u;
            reader->phase = S64_WIRE_PHASE_SLUG;
            break;

        case S64_WIRE_PHASE_SLUG:
            if (!has_prefix(reader->line, "Slug: ", 6u)) return 0u;
            value = reader->line + 6;
            if (!s64_persistent_document_slug_valid(value)
                || !copy_text(reader->document->slug,
                    S64_DOCUMENT_SLUG_CAPACITY, value)) return 0u;
            reader->phase = S64_WIRE_PHASE_TITLE;
            break;

        case S64_WIRE_PHASE_TITLE:
            if (!has_prefix(reader->line, "Title: ", 7u)) return 0u;
            value = reader->line + 7;
            if (!*value || !copy_text(reader->document->title,
                S64_DOCUMENT_TITLE_CAPACITY, value)) return 0u;
            reader->phase = S64_WIRE_PHASE_STATUS;
            break;

        case S64_WIRE_PHASE_STATUS:
            if (!has_prefix(reader->line, "Status: ", 8u)) return 0u;
            value = reader->line + 8;
            if (!strcmp(value, "draft")) reader->document->status = S64_DOCUMENT_DRAFT;
            else if (!strcmp(value, "published")) {
                reader->document->status = S64_DOCUMENT_PUBLISHED;
            } else return 0u;
            reader->phase = S64_WIRE_PHASE_DATE;
            break;

        case S64_WIRE_PHASE_DATE:
            if (!has_prefix(reader->line, "Date: ", 6u)) return 0u;
            value = reader->line + 6;
            if (!copy_text(reader->document->date,
                S64_DOCUMENT_DATE_CAPACITY, value)) return 0u;
            reader->phase = S64_WIRE_PHASE_ORDER;
            break;

        case S64_WIRE_PHASE_ORDER:
            if (!has_prefix(reader->line, "Order: ", 7u)
                || !parse_order(reader->line + 7, &reader->document->order)) {
                return 0u;
            }
            reader->phase = S64_WIRE_PHASE_EXTRA;
            break;

        case S64_WIRE_PHASE_EXTRA:
            if (!reader->line_length) {
                reader->phase = S64_WIRE_PHASE_BODY;
            } else {
                char* colon = strchr(reader->line, ':');
                if (!colon || colon[1] != ' ') return 0u;
            }
            break;

        default:
            return 0u;
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
