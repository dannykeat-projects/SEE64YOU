#include "publisher/persistent_services.h"

#include <string.h>

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
