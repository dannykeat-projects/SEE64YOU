#include "publisher/persistent_services.h"

#include <string.h>

int s64_persistent_document_slug_valid(const char* slug)
{
    uint16_t i;
    uint8_t expect_alnum = 1u;

    if (slug == 0 || slug[0] == '\0') return 0;
    for (i = 0u; i < S64_DOCUMENT_SLUG_CAPACITY; ++i) {
        if (slug[i] == '\0') return !expect_alnum;
        if ((slug[i] >= 'a' && slug[i] <= 'z')
            || (slug[i] >= '0' && slug[i] <= '9')) {
            expect_alnum = 0u;
        } else if (slug[i] == '-' && !expect_alnum) {
            expect_alnum = 1u;
        } else {
            return 0;
        }
    }
    return 0;
}

const char* s64_persistent_document_type_name(S64DocumentType type)
{
    if (type == S64_DOCUMENT_PAGE) return "page";
    if (type == S64_DOCUMENT_POST) return "post";
    return 0;
}

/*
 * Read-side streaming remains deliberately gated until the next migration
 * slice. Write-side streaming lives in the PNETWORK overlay so it adds no
 * permanent Editor resident footprint.
 */
void s64_persistent_document_wire_reader_init(
    S64DocumentWireReader* reader,
    S64Document* document
)
{
    if (reader == 0) return;
    memset(reader, 0, sizeof(*reader));
    reader->document = document;
    reader->result = S64_DOCUMENT_INVALID_ARGUMENT;
}

uint8_t s64_persistent_document_wire_reader_write(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    (void)data;
    (void)length;
    (void)context;
    return 0u;
}

S64DocumentResult s64_persistent_document_wire_reader_finish(
    S64DocumentWireReader* reader
)
{
    (void)reader;
    return S64_DOCUMENT_INVALID_ARGUMENT;
}
