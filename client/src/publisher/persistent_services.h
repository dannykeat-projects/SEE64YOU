#ifndef SEE64YOU_PUBLISHER_PERSISTENT_SERVICES_H
#define SEE64YOU_PUBLISHER_PERSISTENT_SERVICES_H

#include "document.h"

/*
 * Narrow services shared by persistent Publisher overlays without making them
 * depend directly on Editor overlay code. The diagnostic implementation keeps
 * document streaming explicitly gated until the shared codec slice is migrated.
 */
int s64_persistent_document_slug_valid(const char* slug);
const char* s64_persistent_document_type_name(S64DocumentType type);
S64DocumentResult s64_persistent_document_write_wire(
    const S64Document* document,
    S64DocumentSink sink,
    void* context
);
void s64_persistent_document_wire_reader_init(
    S64DocumentWireReader* reader,
    S64Document* document
);
uint8_t s64_persistent_document_wire_reader_write(
    const uint8_t* data,
    uint16_t length,
    void* context
);
S64DocumentResult s64_persistent_document_wire_reader_finish(
    S64DocumentWireReader* reader
);

#endif
