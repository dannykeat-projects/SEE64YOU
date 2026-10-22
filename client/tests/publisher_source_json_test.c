#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __CC65__
#include <conio.h>
#endif
#include "charset.h"
#include "publisher/source_json.h"

static S64Buffer body;
static S64Document document;
static uint8_t output[512], expected[512];
static unsigned int used, calls, fail_call;

static void require(int condition, const char* message)
{
    if (condition) return;
#ifdef __CC65__
    cprintf("FAIL: %s\r\n", message);
    for (;;) {}
#else
    fprintf(stderr, "FAIL: %s\n", message);
    exit(1);
#endif
}

static uint8_t collect(const uint8_t* data, uint16_t length, void* context)
{
    (void)context;
    ++calls;
    if (calls == fail_call || length > sizeof(output) - used) return 0u;
    memcpy(output + used, data, length);
    used += length;
    return 1u;
}

static unsigned int ascii_fixture(const char* native, uint8_t* ascii)
{
    unsigned int length = 0u;
    while (*native) {
        require(s64_charset_native_to_ascii((uint8_t)*native++, ascii + length), "fixture charset");
        ++length;
    }
    return length;
}

int main(void)
{
    static const char source[] = "---\n8TmL: 1\nX-Future: Keep : CASE\n---\n\t\"quoted\" \\ *text*\n";
    static const char json[] = "{\"type\":\"post\",\"slug\":\"native-json\",\"status\":\"published\","
        "\"raw8tml\":\"---\\n8TmL: 1\\nX-Future: Keep : CASE\\n---\\n\\t\\\"quoted\\\" \\\\ *text*\\n\"}";
    unsigned int i, length, successful_calls;
    S64DocumentWireReader reader;
#ifdef __CC65__
    clrscr();
#endif
    s64_buffer_init(&body);
    s64_document_init(&document, &body);
    require(s64_buffer_length(&body) == 0u, "Publisher init does not create Editor source");
    length = ascii_fixture(source, expected);
    s64_document_wire_reader_init(&reader, &document);
    require(s64_document_wire_reader_write(expected, (uint16_t)length, &reader), "source read");
    require(s64_document_wire_reader_finish(&reader) == S64_DOCUMENT_OK, "source valid");
    document.type = S64_DOCUMENT_POST;
    document.status = S64_DOCUMENT_PUBLISHED;
    require(s64_document_set_slug(&document, "native-json"), "metadata slug");
    require(s64_source_write_json(&document, collect, 0) == S64_DOCUMENT_OK, "serialize source");
    length = ascii_fixture(json, expected);
    require(used == length && !memcmp(output, expected, length), "exact ASCII JSON and escaping");
    for (i = 0u; i < used; ++i) require(output[i] >= 32u && output[i] <= 126u, "printable JSON only");
    successful_calls = calls;
    for (fail_call = 1u; fail_call <= successful_calls; ++fail_call) {
        used = calls = 0u;
        require(s64_source_write_json(&document, collect, 0) == S64_DOCUMENT_SINK_ERROR,
            "failed writer propagates");
        require(calls == fail_call, "no writes after failure");
    }
    fail_call = 0u;
    used = calls = 0u;
    document.status = (S64DocumentStatus)0;
    require(s64_source_write_json(&document, collect, 0) == S64_DOCUMENT_INVALID_METADATA,
        "invalid publication status");
    require(calls == 0u, "bad metadata writes nothing");
    require(s64_source_write_json(0, collect, 0) == S64_DOCUMENT_INVALID_ARGUMENT, "null document");
#ifdef __CC65__
    cputs("8TML JSON tests passed");
    while (cgetc() != 'x') {}
#else
    puts("8TML JSON tests passed");
#endif
    return 0;
}
