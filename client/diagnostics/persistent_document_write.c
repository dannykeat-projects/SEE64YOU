#include "publisher/persistent_services.h"

#include "buffer.h"
#include "charset.h"
#include "publisher/workspace.h"

#include <string.h>

#define S64_DOCUMENT_WIRE_CHUNK 64u
#define wire_chunk ((uint8_t*)&s64_publisher_workspace.bytes[896])

static uint8_t header_text_valid(
    const char* text,
    uint16_t capacity,
    uint8_t allow_empty
)
{
    uint16_t i;

    if (text == 0 || capacity == 0u) return 0u;
    for (i = 0u; i < capacity; ++i) {
        if (text[i] == '\0') return allow_empty || i != 0u;
        if (text[i] == '\r' || text[i] == '\n') return 0u;
    }
    return 0u;
}

static uint8_t emit(
    S64DocumentSink sink,
    void* context,
    const char* text
)
{
    uint16_t length;
    uint16_t offset = 0u;
    uint16_t chunk;
    uint16_t i;

    length = (uint16_t)strlen(text);
    while (offset < length) {
        chunk = (uint16_t)(length - offset);
        if (chunk > S64_DOCUMENT_WIRE_CHUNK) chunk = S64_DOCUMENT_WIRE_CHUNK;
        for (i = 0u; i < chunk; ++i) {
            if (!s64_charset_native_to_ascii(
                    (uint8_t)text[offset + i],
                    &wire_chunk[i]
                )) {
                return 0u;
            }
        }
        if (!sink(wire_chunk, chunk, context)) return 0u;
        offset = (uint16_t)(offset + chunk);
    }
    return 1u;
}

static void format_order(int32_t value, char output[16])
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
    while (count != 0u) *output++ = digits[--count];
    *output = '\0';
}

S64DocumentResult s64_persistent_document_write_wire(
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

    type = s64_persistent_document_type_name(document->type);
    status = document->status == S64_DOCUMENT_DRAFT
        ? "draft"
        : (document->status == S64_DOCUMENT_PUBLISHED ? "published" : 0);
    if (type == 0 || status == 0
        || !s64_persistent_document_slug_valid(document->slug)
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

    format_order(document->order, order);
    if (!emit(sink, context, order) || !emit(sink, context, "\n\n")) {
        return S64_DOCUMENT_SINK_ERROR;
    }

    body_length = s64_buffer_length(document->body);
    while (offset < body_length) {
        chunk = (uint16_t)(body_length - offset);
        if (chunk > S64_DOCUMENT_WIRE_CHUNK) chunk = S64_DOCUMENT_WIRE_CHUNK;
        for (i = 0u; i < chunk; ++i) {
            if (!s64_charset_native_to_ascii(
                    s64_buffer_char_at(document->body, offset + i),
                    &wire_chunk[i]
                )) {
                return S64_DOCUMENT_INVALID_METADATA;
            }
        }
        if (!sink(wire_chunk, chunk, context)) return S64_DOCUMENT_SINK_ERROR;
        offset += chunk;
    }
    return S64_DOCUMENT_OK;
}
