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
    memset(document.title, 'X', sizeof(document.title));
    memset(&sink, 0, sizeof(sink));
    require(s64_document_write_wire(&document, collect, &sink) == S64_DOCUMENT_INVALID_METADATA,
        "reject unterminated metadata");

    puts("document tests passed");
    return 0;
}
