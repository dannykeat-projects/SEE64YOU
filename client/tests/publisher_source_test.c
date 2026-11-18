#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "storage/local.h"
#include "publisher/standard_publish_names.h"

static const char* filename;
static const unsigned char* source;
static unsigned int size, offset, written;
static unsigned char output[256];
static S64UciResult execute(const S64UciCommandPart* parts, uint8_t count, S64UciReply* reply)
{
    uint8_t opcode = parts[0].data[1];
    unsigned int length;
    const char* status = "00,OK";
    (void)count;
    if (opcode == 2u) {
        if (strcmp((const char*)parts[1].data, filename)) status = "88,FILE NOT FOUND";
        offset = 0u;
    } else if (opcode == 4u) {
        length = (unsigned int)(parts[0].data[2] | ((uint16_t)parts[0].data[3] << 8));
        if (length > size - offset) length = size - offset;
        if (length && !reply->data_sink(source + offset, length, reply->data_context))
            return S64_UCI_CONSUMER_ABORTED;
        offset += length;
    }
    strcpy(reply->status, status); reply->status_length = (uint16_t)strlen(status);
    return S64_UCI_OK;
}
static uint8_t collect(const uint8_t* bytes, uint16_t length, void* context)
{
    (void)context;
    assert(written + length <= sizeof(output));
    memcpy(output + written, bytes, length); written += length; return 1u;
}
static void roundtrip(S64LocalDocument* local, const char* name, const char* bytes, uint8_t kind)
{
    filename = name; source = (const unsigned char*)bytes; size = (unsigned int)strlen(bytes);
    assert(s64_local_open(local, name, 1u) == S64_LOCAL_OK);
    assert(local->document->kind == kind);
    written = 0;
    assert(s64_document_write_wire(local->document, collect, 0) == S64_DOCUMENT_OK);
    assert(written == size && !memcmp(output, bytes, size));
}
int main(void)
{
    S64Dos dos;
    S64Buffer body;
    S64Document document;
    S64LocalDocument local;
    s64_dos_init(&dos, execute, S64_DOS_TARGET_PRIMARY);
    s64_buffer_init(&body); s64_document_init(&document, &body); s64_local_init(&local, &dos, &document);
    roundtrip(&local, "PAGE.8TML", "---\n8tml: 1\n---\n# Page\n", 1u);
    roundtrip(&local, "STYLE.8SS", "text: white\nunknown: Keep CASE\n", 2u);
    roundtrip(&local, "NOTES.TXT", "Plain source\n", 3u);
    roundtrip(&local, "EMPTY.txt", "", 3u);
    assert(s64_local_open(&local, "NOTES.md", 1u) == S64_LOCAL_INVALID_ARGUMENT);
    assert(s64_local_open(&local, "S64EDIT.PRG", 1u) == S64_LOCAL_INVALID_ARGUMENT);
    assert(!s64_publish_source_kind("STYLE.8ss.tmp"));
    assert(s64_publish_source_kind("style.8ss") == 2u);
    filename = "BAD.8TML"; source = (const unsigned char*)"no header\n"; size = 10u;
    assert(s64_local_open(&local, filename, 1u) == S64_LOCAL_INVALID_DOCUMENT);
    filename = "BAD.8SS"; source = (const unsigned char*)"bad\200text"; size = 8u;
    assert(s64_local_open(&local, filename, 1u) == S64_LOCAL_INVALID_DOCUMENT);
    puts("Read-only Publisher 8TML/8SS/TXT source and rejection tests passed");
    return 0;
}
