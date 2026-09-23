#include "document.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#define S64_DOCUMENT_WIRE_CHUNK 128u

static uint8_t wire_chunk[S64_DOCUMENT_WIRE_CHUNK];

static uint8_t copy_text(char* destination, uint16_t capacity, const char* source)
{
    uint16_t length = 0u;

    if (destination == 0 || source == 0 || capacity == 0u) {
        return 0u;
    }

    while (source[length] != '\0') {
        if (source[length] == '\r' || source[length] == '\n' || length + 1u >= capacity) {
            return 0u;
        }
        ++length;
    }

    memcpy(destination, source, length + 1u);
    return 1u;
}

static uint8_t header_text_valid(const char* text, uint16_t capacity, uint8_t allow_empty)
{
    uint16_t i;

    if (text == 0 || capacity == 0u) {
        return 0u;
    }

    for (i = 0u; i < capacity; ++i) {
        if (text[i] == '\0') {
            return allow_empty || i != 0u;
        }
        if (text[i] == '\r' || text[i] == '\n') {
            return 0u;
        }
    }

    return 0u;
}

int s64_document_slug_valid(const char* slug)
{
    uint16_t i;
    uint8_t expect_alnum = 1u;

    if (slug == 0 || slug[0] == '\0') {
        return 0u;
    }

    for (i = 0u; i < S64_DOCUMENT_SLUG_CAPACITY; ++i) {
        if (slug[i] == '\0') {
            return !expect_alnum;
        }
        if ((slug[i] >= 'a' && slug[i] <= 'z') || (slug[i] >= '0' && slug[i] <= '9')) {
            expect_alnum = 0u;
        } else if (slug[i] == '-' && !expect_alnum) {
            expect_alnum = 1u;
        } else {
            return 0u;
        }
    }

    return 0u;
}

static uint8_t emit(S64DocumentSink sink, void* context, const char* text)
{
    size_t length = strlen(text);
    return sink((const uint8_t*)text, (uint16_t)length, context);
}

void s64_document_init(S64Document* document, S64Buffer* body)
{
    if (document == 0) {
        return;
    }

    document->type = S64_DOCUMENT_PAGE;
    document->slug[0] = '\0';
    document->title[0] = '\0';
    document->status = S64_DOCUMENT_DRAFT;
    document->date[0] = '\0';
    document->order = 0;
    document->body = body;
}

int s64_document_set_slug(S64Document* document, const char* slug)
{
    if (document == 0 || !s64_document_slug_valid(slug)) {
        return 0;
    }
    return copy_text(document->slug, S64_DOCUMENT_SLUG_CAPACITY, slug);
}

int s64_document_set_title(S64Document* document, const char* title)
{
    if (document == 0 || title == 0 || *title == '\0') {
        return 0;
    }
    return copy_text(document->title, S64_DOCUMENT_TITLE_CAPACITY, title);
}

int s64_document_set_date(S64Document* document, const char* date)
{
    if (document == 0) {
        return 0;
    }
    return copy_text(document->date, S64_DOCUMENT_DATE_CAPACITY, date);
}

const char* s64_document_type_name(S64DocumentType type)
{
    if (type == S64_DOCUMENT_PAGE) return "page";
    if (type == S64_DOCUMENT_POST) return "post";
    return 0;
}

const char* s64_document_status_name(S64DocumentStatus status)
{
    if (status == S64_DOCUMENT_DRAFT) return "draft";
    if (status == S64_DOCUMENT_PUBLISHED) return "published";
    return 0;
}

int s64_document_type_from_name(const char* name, S64DocumentType* type)
{
    if (name == 0 || type == 0) return 0;
    if (strcmp(name, "page") == 0) {
        *type = S64_DOCUMENT_PAGE;
        return 1;
    }
    if (strcmp(name, "post") == 0) {
        *type = S64_DOCUMENT_POST;
        return 1;
    }
    return 0;
}

int s64_document_status_from_name(const char* name, S64DocumentStatus* status)
{
    if (name == 0 || status == 0) return 0;
    if (strcmp(name, "draft") == 0) {
        *status = S64_DOCUMENT_DRAFT;
        return 1;
    }
    if (strcmp(name, "published") == 0) {
        *status = S64_DOCUMENT_PUBLISHED;
        return 1;
    }
    return 0;
}

#define S64_WIRE_PHASE_VERSION 0u
#define S64_WIRE_PHASE_HEADERS 1u
#define S64_WIRE_PHASE_BODY 2u

#define S64_WIRE_SEEN_TYPE   0x01u
#define S64_WIRE_SEEN_SLUG   0x02u
#define S64_WIRE_SEEN_TITLE  0x04u
#define S64_WIRE_SEEN_STATUS 0x08u
#define S64_WIRE_SEEN_DATE   0x10u
#define S64_WIRE_SEEN_ORDER  0x20u
#define S64_WIRE_SEEN_REQUIRED 0x3Fu

static uint8_t parse_order(const char* text, int32_t* value)
{
    uint8_t negative = 0u;
    uint8_t digits = 0u;
    uint32_t magnitude = 0u;
    uint32_t limit;
    uint8_t digit;

    if (text == 0 || value == 0) return 0u;

    if (*text == '-') {
        negative = 1u;
        ++text;
    } else if (*text == '+') {
        ++text;
    }

    limit = negative ? 2147483648UL : 2147483647UL;
    while (*text >= '0' && *text <= '9') {
        digit = (uint8_t)(*text - '0');
        if (magnitude > (limit - digit) / 10u) return 0u;
        magnitude = magnitude * 10u + digit;
        ++digits;
        ++text;
    }

    if (digits == 0u || *text != '\0') return 0u;

    if (negative) {
        if (magnitude == 2147483648UL) {
            *value = (int32_t)INT32_MIN;
        } else {
            *value = -(int32_t)magnitude;
        }
    } else {
        *value = (int32_t)magnitude;
    }
    return 1u;
}

static uint8_t parse_wire_header(S64DocumentWireReader* reader)
{
    char* colon;
    char* value;
    uint8_t bit = 0u;

    colon = strchr(reader->line, ':');
    if (colon == 0 || colon[1] != ' ') return 0u;

    *colon = '\0';
    value = colon + 2;

    if (strcmp(reader->line, "Type") == 0) {
        bit = S64_WIRE_SEEN_TYPE;
        if (!s64_document_type_from_name(value, &reader->document->type)) return 0u;
    } else if (strcmp(reader->line, "Slug") == 0) {
        bit = S64_WIRE_SEEN_SLUG;
        if (!s64_document_set_slug(reader->document, value)) return 0u;
    } else if (strcmp(reader->line, "Title") == 0) {
        bit = S64_WIRE_SEEN_TITLE;
        if (!s64_document_set_title(reader->document, value)) return 0u;
    } else if (strcmp(reader->line, "Status") == 0) {
        bit = S64_WIRE_SEEN_STATUS;
        if (!s64_document_status_from_name(value, &reader->document->status)) return 0u;
    } else if (strcmp(reader->line, "Date") == 0) {
        bit = S64_WIRE_SEEN_DATE;
        if (!s64_document_set_date(reader->document, value)) return 0u;
    } else if (strcmp(reader->line, "Order") == 0) {
        bit = S64_WIRE_SEEN_ORDER;
        if (!parse_order(value, &reader->document->order)) return 0u;
    } else {
        return 1u;
    }

    if ((reader->seen & bit) != 0u) return 0u;
    reader->seen = (uint8_t)(reader->seen | bit);
    return 1u;
}

static uint8_t finish_wire_line(S64DocumentWireReader* reader)
{
    reader->line[reader->line_length] = '\0';

    if (reader->phase == S64_WIRE_PHASE_VERSION) {
        if (strcmp(reader->line, "SEE64YOU/1") != 0) return 0u;
        reader->phase = S64_WIRE_PHASE_HEADERS;
    } else if (reader->phase == S64_WIRE_PHASE_HEADERS) {
        if (reader->line_length == 0u) {
            if (reader->seen != S64_WIRE_SEEN_REQUIRED) return 0u;
            reader->phase = S64_WIRE_PHASE_BODY;
        } else if (!parse_wire_header(reader)) {
            return 0u;
        }
    }

    reader->line_length = 0u;
    return 1u;
}

void s64_document_wire_reader_init(S64DocumentWireReader* reader, S64Document* document)
{
    if (reader == 0) return;

    reader->document = document;
    reader->line_length = 0u;
    reader->phase = S64_WIRE_PHASE_VERSION;
    reader->seen = 0u;
    reader->result = S64_DOCUMENT_OK;

    if (document == 0 || document->body == 0) {
        reader->result = S64_DOCUMENT_INVALID_ARGUMENT;
        return;
    }

    s64_buffer_init(document->body);
    s64_document_init(document, document->body);
}

uint8_t s64_document_wire_reader_write(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64DocumentWireReader* reader = (S64DocumentWireReader*)context;
    uint16_t i;
    uint8_t byte;

    if (reader == 0 || (length != 0u && data == 0)
        || reader->result != S64_DOCUMENT_OK) {
        return 0u;
    }

    for (i = 0u; i < length; ++i) {
        byte = data[i];

        if (reader->phase == S64_WIRE_PHASE_BODY) {
            if (!s64_buffer_insert(reader->document->body, byte)) {
                reader->result = S64_DOCUMENT_SINK_ERROR;
                return 0u;
            }
            continue;
        }

        if (byte == '\r') continue;

        if (byte == '\n') {
            if (!finish_wire_line(reader)) {
                reader->result = S64_DOCUMENT_INVALID_METADATA;
                return 0u;
            }
            continue;
        }

        if (reader->line_length + 1u >= S64_DOCUMENT_WIRE_LINE_CAPACITY) {
            reader->result = S64_DOCUMENT_INVALID_METADATA;
            return 0u;
        }
        reader->line[reader->line_length++] = (char)byte;
    }

    return 1u;
}

S64DocumentResult s64_document_wire_reader_finish(S64DocumentWireReader* reader)
{
    if (reader == 0 || reader->document == 0 || reader->document->body == 0) {
        return S64_DOCUMENT_INVALID_ARGUMENT;
    }
    if (reader->result != S64_DOCUMENT_OK) {
        s64_buffer_init(reader->document->body);
        s64_document_init(reader->document, reader->document->body);
        return reader->result;
    }
    if (reader->phase != S64_WIRE_PHASE_BODY) {
        reader->result = S64_DOCUMENT_INVALID_METADATA;
        s64_buffer_init(reader->document->body);
        s64_document_init(reader->document, reader->document->body);
        return reader->result;
    }

    (void)s64_buffer_move_cursor(reader->document->body, 0u);
    return S64_DOCUMENT_OK;
}

S64DocumentResult s64_document_write_wire(
    const S64Document* document,
    S64DocumentSink sink,
    void* context
)
{
    const char* type;
    const char* status;
    unsigned int body_length;
    unsigned int offset = 0u;
    uint16_t chunk;
    uint16_t i;
    char order[16];

    if (document == 0 || document->body == 0 || sink == 0) {
        return S64_DOCUMENT_INVALID_ARGUMENT;
    }

    type = s64_document_type_name(document->type);
    status = s64_document_status_name(document->status);
    if (type == 0 || status == 0 || !s64_document_slug_valid(document->slug)
        || !header_text_valid(document->title, S64_DOCUMENT_TITLE_CAPACITY, 0u)
        || !header_text_valid(document->date, S64_DOCUMENT_DATE_CAPACITY, 1u)) {
        return S64_DOCUMENT_INVALID_METADATA;
    }

    if (!emit(sink, context, "SEE64YOU/1\nType: ")
        || !emit(sink, context, type)
        || !emit(sink, context, "\nSlug: ")
        || !emit(sink, context, document->slug)
        || !emit(sink, context, "\nTitle: ")
        || !emit(sink, context, document->title)
        || !emit(sink, context, "\nStatus: ")
        || !emit(sink, context, status)
        || !emit(sink, context, "\nDate: ")
        || !emit(sink, context, document->date)
        || !emit(sink, context, "\nOrder: ")) {
        return S64_DOCUMENT_SINK_ERROR;
    }

    sprintf(order, "%ld", (long)document->order);
    if (!emit(sink, context, order) || !emit(sink, context, "\n\n")) {
        return S64_DOCUMENT_SINK_ERROR;
    }

    body_length = s64_buffer_length(document->body);
    while (offset < body_length) {
        chunk = (uint16_t)(body_length - offset);
        if (chunk > S64_DOCUMENT_WIRE_CHUNK) {
            chunk = S64_DOCUMENT_WIRE_CHUNK;
        }
        for (i = 0u; i < chunk; ++i) {
            wire_chunk[i] = s64_buffer_char_at(document->body, offset + i);
        }
        if (!sink(wire_chunk, chunk, context)) {
            return S64_DOCUMENT_SINK_ERROR;
        }
        offset += chunk;
    }

    return S64_DOCUMENT_OK;
}
