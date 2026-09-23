#include "document.h"

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
