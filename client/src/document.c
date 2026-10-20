#include "document.h"
#include "charset.h"

#include <limits.h>
#include <string.h>

#define S64_DOCUMENT_WIRE_CHUNK 64u

static uint8_t wire_chunk[S64_DOCUMENT_WIRE_CHUNK];

#ifndef S64_INDEPENDENT_EDITOR
static void format_int32(int32_t value, char* output)
{
    char digits[10];
    uint32_t magnitude;
    uint8_t count = 0u;

    if (value < 0) {
        *output++ = '-';
        magnitude = (uint32_t)(-(value + 1));
        ++magnitude;
    } else {
        magnitude = (uint32_t)value;
    }

    do {
        digits[count++] = (char)('0' + (magnitude % 10u));
        magnitude /= 10u;
    } while (magnitude != 0u);

    while (count != 0u) {
        *output++ = digits[--count];
    }
    *output = '\0';
}


static uint8_t native_string_wire_valid(const char* text)
{
    uint8_t wire;

    if (text == 0) return 0u;

    while (*text != '\0') {
        if (!s64_charset_native_to_ascii((uint8_t)*text, &wire)) return 0u;
        ++text;
    }
    return 1u;
}

static uint8_t native_body_wire_valid(const S64Buffer* body)
{
    unsigned int i;
    uint8_t wire;

    if (body == 0) return 0u;

    for (i = 0u; i < s64_buffer_length(body); ++i) {
        if (!s64_charset_native_to_ascii(s64_buffer_char_at(body, i), &wire)) return 0u;
    }
    return 1u;
}

#endif

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

#ifndef S64_INDEPENDENT_EDITOR
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

#endif

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

#ifndef S64_INDEPENDENT_EDITOR
static uint8_t emit(S64DocumentSink sink, void* context, const char* text)
{
    size_t length = strlen(text);
    size_t offset = 0u;
    uint16_t chunk;
    uint16_t i;

    while (offset < length) {
        chunk = (uint16_t)(length - offset);
        if (chunk > S64_DOCUMENT_WIRE_CHUNK) {
            chunk = S64_DOCUMENT_WIRE_CHUNK;
        }

        for (i = 0u; i < chunk; ++i) {
            if (!s64_charset_native_to_ascii((uint8_t)text[offset + i], &wire_chunk[i])) {
                return 0u;
            }
        }
        if (!sink(wire_chunk, chunk, context)) return 0u;
        offset += chunk;
    }

    return 1u;
}

#endif

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
#ifdef S64_INDEPENDENT_EDITOR
    if (body != 0 && s64_buffer_length(body) == 0u) {
        const char* initial = "---\n8tml: 1\ntitle: Untitled\n---\n";
        while (*initial != '\0') (void)s64_buffer_insert(body, (uint8_t)*initial++);
        /* Start typing in the body; the header remains editable above it. */
    }
#endif
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

#ifndef S64_INDEPENDENT_EDITOR
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
        if (!s64_charset_ascii_to_native(data[i], &byte)) {
            reader->result = S64_DOCUMENT_INVALID_METADATA;
            return 0u;
        }

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
        || !header_text_valid(document->date, S64_DOCUMENT_DATE_CAPACITY, 1u)
        || !native_string_wire_valid(document->slug)
        || !native_string_wire_valid(document->title)
        || !native_string_wire_valid(document->date)
        || !native_body_wire_valid(document->body)) {
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

    format_int32(document->order, order);
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
            if (!s64_charset_native_to_ascii(
                    s64_buffer_char_at(document->body, offset + i),
                    &wire_chunk[i]
                )) {
                return S64_DOCUMENT_INVALID_METADATA;
            }
        }
        if (!sink(wire_chunk, chunk, context)) {
            return S64_DOCUMENT_SINK_ERROR;
        }
        offset += chunk;
    }

    return S64_DOCUMENT_OK;
}

#else

/* The Editor owns literal source, including the header. Never reconstruct
 * metadata or interpret markup on a disk round trip. */
static uint8_t source_byte(const S64Buffer* body, unsigned int position)
{
    uint8_t byte = 0u;
    (void)s64_charset_native_to_ascii(s64_buffer_char_at(body, position), &byte);
    return byte;
}

static uint8_t source_valid(const S64Buffer* body)
{
    unsigned int length = s64_buffer_length(body);
    unsigned int start = 4u;
    unsigned int end;
    unsigned int position;
    unsigned int colon;
    uint8_t byte;
    uint8_t version_seen = 0u;
    uint8_t header_closed = 0u;

    for (position = 0u; position < length; ++position) {
        if (!s64_charset_native_to_ascii(s64_buffer_char_at(body, position), &byte)
            || (byte != 9u && byte != 10u && (byte < 32u || byte > 126u))) return 0u;
    }
    if (length < 4u || source_byte(body, 0u) != 45u
        || source_byte(body, 1u) != 45u || source_byte(body, 2u) != 45u
        || source_byte(body, 3u) != 10u) return 0u;

    while (start < length && !header_closed) {
        end = start;
        while (end < length && source_byte(body, end) != 10u) ++end;
        if (end - start == 3u && source_byte(body, start) == 45u
            && source_byte(body, start + 1u) == 45u
            && source_byte(body, start + 2u) == 45u) {
            header_closed = 1u;
        } else if (end != start && source_byte(body, start) != 35u) {
            colon = start;
            while (colon < end && source_byte(body, colon) != 58u) ++colon;
            if (colon == start || colon == end) return 0u;
            if (!version_seen) {
                if (colon - start != 4u || source_byte(body, start) != 56u
                    || (source_byte(body, start + 1u) | 32u) != 116u
                    || (source_byte(body, start + 2u) | 32u) != 109u
                    || (source_byte(body, start + 3u) | 32u) != 108u) return 0u;
                position = colon + 1u;
                while (position < end && (source_byte(body, position) == 32u
                    || source_byte(body, position) == 9u)) ++position;
                if (position == end || source_byte(body, position++) != 49u) return 0u;
                while (position < end && (source_byte(body, position) == 32u
                    || source_byte(body, position) == 9u)) ++position;
                if (position != end) return 0u;
                version_seen = 1u;
            }
        }
        start = end + 1u;
    }
    return version_seen && header_closed;
}

void s64_document_wire_reader_init(S64DocumentWireReader* reader, S64Document* document)
{
    if (reader == 0) return;
    reader->document = document;
    reader->seen = 0u; /* previous input byte was CR */
    reader->result = S64_DOCUMENT_INVALID_ARGUMENT;
    if (document == 0 || document->body == 0) return;
    s64_document_init(document, document->body);
    s64_buffer_init(document->body);
    reader->result = S64_DOCUMENT_OK;
}

uint8_t s64_document_wire_reader_write(const uint8_t* data, uint16_t length, void* context)
{
    S64DocumentWireReader* reader = (S64DocumentWireReader*)context;
    uint16_t i;
    uint8_t byte;
    uint8_t native;
    if (reader == 0 || (length != 0u && data == 0)
        || reader->result != S64_DOCUMENT_OK) return 0u;
    for (i = 0u; i < length; ++i) {
        byte = data[i];
        if (byte == 10u && reader->seen) {
            reader->seen = 0u;
            continue;
        }
        reader->seen = byte == 13u;
        if (byte == 13u) byte = 10u;
        if ((byte != 9u && byte != 10u && (byte < 32u || byte > 126u))
            || !s64_charset_ascii_to_native(byte, &native)) {
            reader->result = S64_DOCUMENT_INVALID_METADATA;
            return 0u;
        }
        if (!s64_buffer_insert(reader->document->body, native)) {
            reader->result = S64_DOCUMENT_SINK_ERROR;
            return 0u;
        }
    }
    return 1u;
}

S64DocumentResult s64_document_wire_reader_finish(S64DocumentWireReader* reader)
{
    if (reader == 0 || reader->document == 0 || reader->document->body == 0)
        return S64_DOCUMENT_INVALID_ARGUMENT;
    if (reader->result == S64_DOCUMENT_OK && !source_valid(reader->document->body))
        reader->result = S64_DOCUMENT_INVALID_METADATA;
    if (reader->result != S64_DOCUMENT_OK) s64_buffer_init(reader->document->body);
    (void)s64_buffer_move_cursor(reader->document->body, 0u);
    return reader->result;
}

S64DocumentResult s64_document_write_wire(
    const S64Document* document, S64DocumentSink sink, void* context)
{
    unsigned int offset = 0u;
    unsigned int length;
    uint16_t chunk;
    uint16_t i;
    if (document == 0 || document->body == 0 || sink == 0)
        return S64_DOCUMENT_INVALID_ARGUMENT;
    if (!source_valid(document->body)) return S64_DOCUMENT_INVALID_METADATA;
    length = s64_buffer_length(document->body);
    while (offset < length) {
        chunk = length - offset > S64_DOCUMENT_WIRE_CHUNK
            ? S64_DOCUMENT_WIRE_CHUNK : (uint16_t)(length - offset);
        for (i = 0u; i < chunk; ++i)
            wire_chunk[i] = source_byte(document->body, offset + i);
        if (!sink(wire_chunk, chunk, context)) return S64_DOCUMENT_SINK_ERROR;
        offset += chunk;
    }
    return S64_DOCUMENT_OK;
}
#endif
