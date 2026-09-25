#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "document.h"

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

typedef struct {
    char data[512];
    unsigned int length;
} TextSink;

static uint8_t collect(const uint8_t* data, uint16_t length, void* context)
{
    TextSink* sink = (TextSink*)context;
    if (sink->length + length >= sizeof(sink->data)) return 0u;
    memcpy(sink->data + sink->length, data, length);
    sink->length += length;
    sink->data[sink->length] = '\0';
    return 1u;
}

static void require_buffer_text(const S64Buffer* buffer, const char* expected, const char* message)
{
    unsigned int i;
    size_t length = strlen(expected);

    require(s64_buffer_length(buffer) == length, message);
    for (i = 0u; i < length; ++i) {
        require(s64_buffer_char_at(buffer, i) == (unsigned char)expected[i], message);
    }
}

int main(void)
{
    S64Buffer body;
    S64Document document;
    TextSink sink;
    const char* body_text = "Hello **C64U**\nSecond line.";
    const char* expected =
        "SEE64YOU/1\n"
        "Type: post\n"
        "Slug: hello-world\n"
        "Title: Hello World\n"
        "Status: published\n"
        "Date: 2026-09-23T20:10:00+10:00\n"
        "Order: 7\n\n"
        "Hello **C64U**\nSecond line.";

    s64_buffer_init(&body);
    while (*body_text != '\0') {
        require(s64_buffer_insert(&body, (unsigned char)*body_text++), "insert body");
    }

    s64_document_init(&document, &body);
    document.type = S64_DOCUMENT_POST;
    document.status = S64_DOCUMENT_PUBLISHED;
    document.order = 7;
    require(s64_document_set_slug(&document, "hello-world"), "set slug");
    require(s64_document_set_title(&document, "Hello World"), "set title");
    require(s64_document_set_date(&document, "2026-09-23T20:10:00+10:00"), "set date");

    memset(&sink, 0, sizeof(sink));
    require(s64_document_write_wire(&document, collect, &sink) == S64_DOCUMENT_OK, "write wire");
    require(strcmp(sink.data, expected) == 0, "wire format matches backend");
    require(!s64_document_set_slug(&document, "../bad"), "reject bad slug");
    require(!s64_document_set_title(&document, "bad\ntitle"), "reject header newline");
    require(s64_document_set_title(&document, "ASCII title"), "restore valid title");
    document.title[5] = (char)0x80u;
    memset(&sink, 0, sizeof(sink));
    require(
        s64_document_write_wire(&document, collect, &sink) == S64_DOCUMENT_INVALID_METADATA,
        "reject non-ASCII canonical metadata"
    );
    require(sink.length == 0u, "invalid canonical metadata emits nothing");
    require(s64_document_set_title(&document, "Hello World"), "restore title");
    memset(document.title, 'X', sizeof(document.title));
    memset(&sink, 0, sizeof(sink));
    require(s64_document_write_wire(&document, collect, &sink) == S64_DOCUMENT_INVALID_METADATA,
        "reject unterminated metadata");

    {
        S64Document parsed;
        S64DocumentWireReader reader;
        S64Buffer parsed_body;
        size_t offset = 0u;
        size_t chunk;

        s64_document_init(&parsed, &parsed_body);
        s64_document_wire_reader_init(&reader, &parsed);

        while (offset < strlen(expected)) {
            chunk = strlen(expected) - offset;
            if (chunk > 7u) chunk = 7u;
            require(
                s64_document_wire_reader_write(
                    (const uint8_t*)expected + offset,
                    (uint16_t)chunk,
                    &reader
                ),
                "stream wire chunk"
            );
            offset += chunk;
        }

        require(
            s64_document_wire_reader_finish(&reader) == S64_DOCUMENT_OK,
            "finish streamed wire document"
        );
        require(parsed.type == S64_DOCUMENT_POST, "parsed type");
        require(parsed.status == S64_DOCUMENT_PUBLISHED, "parsed status");
        require(parsed.order == 7, "parsed order");
        require(strcmp(parsed.slug, "hello-world") == 0, "parsed slug");
        require(strcmp(parsed.title, "Hello World") == 0, "parsed title");
        require(strcmp(parsed.date, "2026-09-23T20:10:00+10:00") == 0, "parsed date");
        require_buffer_text(&parsed_body, "Hello **C64U**\nSecond line.", "parsed body");
        require(s64_buffer_cursor(&parsed_body) == 0u, "parsed document opens at start");
    }

    {
        static const char duplicate[] =
            "SEE64YOU/1\n"
            "Type: post\n"
            "Type: page\n"
            "Slug: duplicate\n"
            "Title: Duplicate\n"
            "Status: draft\n"
            "Date: \n"
            "Order: 0\n\n";
        S64Document parsed;
        S64DocumentWireReader reader;
        S64Buffer parsed_body;

        s64_document_init(&parsed, &parsed_body);
        s64_document_wire_reader_init(&reader, &parsed);
        require(
            !s64_document_wire_reader_write(
                (const uint8_t*)duplicate,
                (uint16_t)(sizeof(duplicate) - 1u),
                &reader
            ),
            "duplicate required header rejected"
        );
        require(
            s64_document_wire_reader_finish(&reader) == S64_DOCUMENT_INVALID_METADATA,
            "duplicate header reports invalid metadata"
        );
        require(s64_buffer_length(&parsed_body) == 0u, "invalid wire clears destination body");
    }

    puts("document tests passed");
    return 0;
}
