#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __CC65__
#include <conio.h>
#endif
#include "document.h"
#include "charset.h"
#ifdef S64_REU_BUFFER
#include "platform/reu.h"
static uint8_t fake_reu[S64_BUFFER_CAPACITY];
uint8_t s64_reu_store(uint8_t bank, uint16_t address, const void* source, uint16_t length)
{
    if (bank != S64_REU_DOCUMENT_BANK || (unsigned long)address + length > sizeof(fake_reu)) return 0u;
    memcpy(fake_reu + address, source, length);
    return 1u;
}
uint8_t s64_reu_load(uint8_t bank, uint16_t address, void* destination, uint16_t length)
{
    if (bank != S64_REU_DOCUMENT_BANK || (unsigned long)address + length > sizeof(fake_reu)) return 0u;
    memcpy(destination, fake_reu + address, length);
    return 1u;
}
#endif

static S64Buffer body;
static S64Document document;
static S64DocumentWireReader reader;
static uint8_t output[S64_BUFFER_CAPACITY];
static uint8_t input[512];
static unsigned int output_length;
static unsigned int sink_calls;
static const char* invalid_sources[] = {
    "", "plain text", "SEE64YOU/1\n", "---\n---\n",
    "---\ntitle: Wrong first key\n8tml: 1\n---\n",
    "---\n8tml: 2\n---\n", "---\n8tml: 1x\n---\n",
    "---\n8tml: 1\n8TmL: 2\n---\n",
    "---\n8tml: 1\n", "--- \n8tml: 1\n---\n",
    "---\n8tml: 1\nbad line\n---\n"
};

static void require(int condition, const char* message)
{
    if (condition) return;
#ifdef __CC65__
    cprintf("FAIL: %s\r\n", message);
    while (1) {}
#else
    fprintf(stderr, "FAIL: %s\n", message);
    exit(1);
#endif
}

static uint8_t collect(const uint8_t* data, uint16_t length, void* unused)
{
    (void)unused;
    ++sink_calls;
    if (length > sizeof(output) - output_length) return 0u;
    memcpy(output + output_length, data, length);
    output_length += length;
    return 1u;
}

static void save(void)
{
    output_length = 0u;
    sink_calls = 0u;
    require(s64_document_write_wire(&document, collect, 0) == S64_DOCUMENT_OK, "save source");
}

/* Literal fixtures are native strings under cc65, ASCII under the host. */
static unsigned int fixture(const char* text)
{
    unsigned int length = 0u;
    while (*text) {
        require(s64_charset_native_to_ascii((uint8_t)*text++, &input[length]), "fixture charset");
        ++length;
    }
    return length;
}

int main(void)
{
    unsigned int length;
    unsigned int split;
    unsigned int i;
    uint8_t native;
#ifndef __CC65__
    FILE* file;
#else
    clrscr();
    cprintf("8TML tests running\r\n");
#endif
    s64_buffer_init(&body);
    s64_document_init(&document, &body);
    require(s64_buffer_cursor(&body) == s64_buffer_length(&body), "new cursor starts in body");
    save();
    length = fixture("---\n8tml: 1\ntitle: Untitled\n---\n");
    require(length == output_length && !memcmp(input, output, length), "new empty document header");
#ifndef __CC65__
    file = fopen("client/build/8tml-empty.8tml", "wb");
    require(file != 0, "create validator fixture");
    require(fwrite(output, 1u, output_length, file) == output_length, "write validator fixture");
    require(!fclose(file), "close validator fixture");
#endif
    length = fixture("---\n# comment\n8TmL: \t1 \nX-Future:  A:B  \ntitle: Mixed CASE\n---\n<future x>Keep</future>\n:::future\nopaque\n:::\n");
    for (i = 32u; i <= 126u; ++i) input[length++] = (uint8_t)i;
    input[length++] = 9u;
    input[length++] = 10u;
    for (split = 0u; split <= length;
#ifdef __CC65__
         split += 17u) {
#else
         ++split) {
#endif
        s64_document_wire_reader_init(&reader, &document);
        require(s64_document_wire_reader_write(input, split, &reader), "first chunk");
        require(s64_document_wire_reader_write(input + split, length - split, &reader), "second chunk");
        require(s64_document_wire_reader_finish(&reader) == S64_DOCUMENT_OK, "finish source");
        save();
        require(output_length == length && !memcmp(input, output, length), "literal metadata markup and printable ASCII");
    }
#ifdef __CC65__
    cprintf("ASCII roundtrip passed\r\n");
#endif
#ifndef __CC65__
    file = fopen("client/build/8tml-roundtrip.8tml", "wb");
    require(file != 0 && fwrite(output, 1u, output_length, file) == output_length, "write roundtrip fixture");
    require(!fclose(file), "close roundtrip fixture");
#endif
    length = fixture("---\r\n8tml: 1\rtitle: CRLF\r\n---\r\na\rb\r\nc\n");
    for (split = 0u; split <= length;
#ifdef __CC65__
         split += 3u) {
#else
         ++split) {
#endif
        s64_document_wire_reader_init(&reader, &document);
        require(s64_document_wire_reader_write(input, split, &reader), "CR first chunk");
        require(s64_document_wire_reader_write(input + split, length - split, &reader), "CR second chunk");
        require(s64_document_wire_reader_finish(&reader) == S64_DOCUMENT_OK, "CR finish");
        save();
        for (i = 0u; i < output_length; ++i) require(output[i] != 13u, "LF output");
    }
    length = fixture("---\n8tml: 1\ntitle: CRLF\n---\na\nb\nc\n");
    require(output_length == length && !memcmp(input, output, length), "CR normalization exact");
#ifdef __CC65__
    cprintf("Newlines passed\r\n");
#endif

    for (split = 0u; split < sizeof(invalid_sources) / sizeof(invalid_sources[0]); ++split) {
        length = fixture(invalid_sources[split]);
        s64_document_wire_reader_init(&reader, &document);
        require(s64_document_wire_reader_write(input, length, &reader), "invalid header bytes readable");
        require(s64_document_wire_reader_finish(&reader) == S64_DOCUMENT_INVALID_METADATA, "invalid header rejected");
        require(s64_buffer_length(&body) == 0u, "invalid header cleared");
    }
    for (i = 0u; i <= 255u; ++i) {
        if (i == 9u || i == 10u || i == 13u || (i >= 32u && i <= 126u)) continue;
        s64_document_wire_reader_init(&reader, &document);
        native = (uint8_t)i;
        require(!s64_document_wire_reader_write(&native, 1u, &reader), "invalid input byte rejected");
        require(s64_document_wire_reader_finish(&reader) == S64_DOCUMENT_INVALID_METADATA, "invalid input result");
        require(s64_buffer_length(&body) == 0u, "invalid read cleared");
    }
    length = fixture("---\n8tml: 1\n---\n");
#ifdef __CC65__
    cprintf("Invalid input passed\r\n");
#endif
    s64_document_wire_reader_init(&reader, &document);
    require(s64_document_wire_reader_write(input, length, &reader), "capacity header");
    require(s64_charset_ascii_to_native(120u, &native), "capacity charset");
    while (s64_buffer_free(&body)) require(s64_buffer_insert(&body, native), "capacity fill");
    require(s64_document_wire_reader_finish(&reader) == S64_DOCUMENT_OK, "full capacity valid");
    save();
    require(output_length == S64_BUFFER_CAPACITY, "full capacity exact length");
    require(!s64_buffer_insert(&body, native), "capacity guard");
    require(s64_buffer_move_cursor(&body, 0u), "invalid edit cursor");
    require(s64_buffer_delete(&body), "invalid edit removes delimiter");
    sink_calls = 0u;
    require(s64_document_write_wire(&document, collect, 0) == S64_DOCUMENT_INVALID_METADATA, "invalid edit rejected");
    require(sink_calls == 0u, "invalid source never reaches sink");
#ifdef __CC65__
    cprintf("8TML native tests passed\r\nX EXIT");
    while (cgetc() != 'x') {}
#else
    puts("8TML document tests passed");
#endif
    return 0;
}
