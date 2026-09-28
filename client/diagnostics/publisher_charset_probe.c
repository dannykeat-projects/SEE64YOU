#include <stdint.h>
#include <string.h>

#include "buffer.h"
#include "charset.h"
#include "document.h"

#define DIAG_PROGRESS (*(volatile uint8_t*)0xC070u)
#define DIAG_RESULT   (*(volatile uint8_t*)0xC071u)
#define DIAG_DETAIL   (*(volatile uint8_t*)0xC072u)
#define DIAG_PASS 0xA5u
#define DIAG_FAIL 0x5Au
#define WIRE_CAPACITY 512u

static uint8_t wire[WIRE_CAPACITY];
static uint16_t wire_length;
static S64Buffer source_body;
static S64Buffer loaded_body;
static S64Document source;
static S64Document loaded;
static S64DocumentWireReader reader;
static char slug[S64_DOCUMENT_SLUG_CAPACITY];
static char title[S64_DOCUMENT_TITLE_CAPACITY];
static char date[S64_DOCUMENT_DATE_CAPACITY];

static const uint8_t slug_ascii[] = {
    0x70,0x65,0x74,0x2d,0x74,0x65,0x73,0x74
};
static const uint8_t title_ascii[] = {
    0x41,0x7a,0x20,0x5b,0x5c,0x5d,0x5e,0x5f,0x60,0x7b,0x7c,0x7d,0x7e
};
static const uint8_t date_ascii[] = {
    0x32,0x30,0x32,0x36,0x2d,0x30,0x39,0x2d,0x32,0x38
};
static const uint8_t body_ascii[] = {
    0x41,0x7a,0x20,0x5b,0x5c,0x5d,0x5e,0x5f,0x60,
    0x7b,0x7c,0x7d,0x7e,0x0a,0x45,0x6e,0x64
};

static void fail(uint8_t detail)
{
    DIAG_DETAIL = detail;
    DIAG_RESULT = DIAG_FAIL;
    for (;;) {
    }
}

static uint8_t wire_sink(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    (void)context;
    if ((uint32_t)wire_length + length > WIRE_CAPACITY) {
        return 0u;
    }
    memcpy(wire + wire_length, data, length);
    wire_length = (uint16_t)(wire_length + length);
    return 1u;
}

static uint8_t ascii_to_native_string(
    const uint8_t* ascii,
    uint8_t length,
    char* native,
    uint8_t capacity
)
{
    uint8_t i;

    if ((uint16_t)length + 1u > capacity) return 0u;
    for (i = 0u; i < length; ++i) {
        if (!s64_charset_ascii_to_native(ascii[i], (uint8_t*)&native[i])) {
            return 0u;
        }
    }
    native[length] = '\0';
    return 1u;
}

static uint8_t native_string_equals_ascii(
    const char* native,
    const uint8_t* ascii,
    uint8_t length
)
{
    uint8_t i;
    uint8_t value;

    for (i = 0u; i < length; ++i) {
        if (native[i] == '\0'
            || !s64_charset_native_to_ascii((uint8_t)native[i], &value)
            || value != ascii[i]) {
            return 0u;
        }
    }
    return native[length] == '\0';
}

static uint8_t body_equals_ascii(
    const S64Buffer* body,
    const uint8_t* ascii,
    uint16_t length
)
{
    uint16_t i;
    uint8_t value;

    if (s64_buffer_length(body) != length) return 0u;
    for (i = 0u; i < length; ++i) {
        if (!s64_charset_native_to_ascii(s64_buffer_char_at(body, i), &value)
            || value != ascii[i]) {
            return 0u;
        }
    }
    return 1u;
}

static int16_t body_wire_offset(void)
{
    uint16_t i;

    for (i = 0u; i + 1u < wire_length; ++i) {
        if (wire[i] == 0x0au && wire[i + 1u] == 0x0au) {
            return (int16_t)(i + 2u);
        }
    }
    return -1;
}

int main(void)
{
    uint16_t i;
    uint16_t offset;
    int16_t body_offset;
    S64DocumentResult result;

    DIAG_PROGRESS = 0u;
    DIAG_RESULT = 0u;
    DIAG_DETAIL = 0u;

    if (!ascii_to_native_string(
            slug_ascii, (uint8_t)sizeof(slug_ascii),
            slug, (uint8_t)sizeof(slug))
        || !ascii_to_native_string(
            title_ascii, (uint8_t)sizeof(title_ascii),
            title, (uint8_t)sizeof(title))
        || !ascii_to_native_string(
            date_ascii, (uint8_t)sizeof(date_ascii),
            date, (uint8_t)sizeof(date))) {
        fail(1u);
    }

    s64_buffer_init(&source_body);
    s64_document_init(&source, &source_body);
    source.type = S64_DOCUMENT_POST;
    source.status = S64_DOCUMENT_PUBLISHED;
    source.order = -7;
    if (!s64_document_set_slug(&source, slug)
        || !s64_document_set_title(&source, title)
        || !s64_document_set_date(&source, date)) {
        fail(2u);
    }

    for (i = 0u; i < (uint16_t)sizeof(body_ascii); ++i) {
        uint8_t native;
        if (!s64_charset_ascii_to_native(body_ascii[i], &native)
            || !s64_buffer_insert(&source_body, native)) {
            fail(3u);
        }
    }

    wire_length = 0u;
    result = s64_document_write_wire(&source, wire_sink, 0);
    if (result != S64_DOCUMENT_OK) {
        fail(4u);
    }
    DIAG_PROGRESS = 0x10u;

    body_offset = body_wire_offset();
    if (body_offset < 0
        || (uint32_t)(uint16_t)body_offset + sizeof(body_ascii) != wire_length
        || memcmp(
            wire + (uint16_t)body_offset,
            body_ascii,
            sizeof(body_ascii)
        ) != 0) {
        fail(5u);
    }

    for (i = 0u; i < wire_length; ++i) {
        if (wire[i] >= 0x80u) {
            fail(6u);
        }
    }
    DIAG_PROGRESS = 0x20u;

    s64_buffer_init(&loaded_body);
    s64_document_init(&loaded, &loaded_body);
    s64_document_wire_reader_init(&reader, &loaded);

    offset = 0u;
    while (offset < wire_length) {
        uint16_t chunk = (uint16_t)(wire_length - offset);
        if (chunk > 7u) chunk = 7u;
        if (!s64_document_wire_reader_write(
                wire + offset,
                chunk,
                &reader
            )) {
            fail(7u);
        }
        offset = (uint16_t)(offset + chunk);
    }

    if (s64_document_wire_reader_finish(&reader) != S64_DOCUMENT_OK) {
        fail(8u);
    }
    if (loaded.type != S64_DOCUMENT_POST
        || loaded.status != S64_DOCUMENT_PUBLISHED
        || loaded.order != -7) {
        fail(9u);
    }
    if (!native_string_equals_ascii(
            loaded.slug, slug_ascii, (uint8_t)sizeof(slug_ascii))
        || !native_string_equals_ascii(
            loaded.title, title_ascii, (uint8_t)sizeof(title_ascii))
        || !native_string_equals_ascii(
            loaded.date, date_ascii, (uint8_t)sizeof(date_ascii))) {
        fail(10u);
    }
    if (!body_equals_ascii(
            &loaded_body,
            body_ascii,
            (uint16_t)sizeof(body_ascii))) {
        fail(11u);
    }

    DIAG_PROGRESS = 0x80u;
    DIAG_RESULT = DIAG_PASS;
    for (;;) {
    }
}
