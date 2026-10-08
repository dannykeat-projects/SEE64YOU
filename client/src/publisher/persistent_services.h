#ifndef SEE64YOU_PUBLISHER_PERSISTENT_SERVICES_H
#define SEE64YOU_PUBLISHER_PERSISTENT_SERVICES_H

#include "document.h"

/*
 * Narrow services shared by persistent Publisher overlays without making them
 * depend directly on Editor overlay code. The diagnostic implementation keeps
 * document streaming explicitly gated until the shared codec slice is migrated.
 */

typedef struct {
    uint16_t header_length;
    uint16_t body_length;
    uint8_t header_chunk_length;
    uint8_t line_empty;
    uint8_t body_started;
    S64DocumentResult result;
} S64PersistentOpenStage;

void s64_persistent_open_stage_init(S64PersistentOpenStage* stage);
uint8_t s64_persistent_open_stage_write(
    const uint8_t* data,
    uint16_t length,
    void* context
);
S64DocumentResult s64_persistent_open_stage_commit(
    const S64PersistentOpenStage* stage,
    S64Document* document
);

uint8_t s64_persistent_list_summary_parse(
    S64DocumentType type,
    uint16_t index,
    S64DocumentSummary* summary,
    uint16_t* total_count
);

int s64_persistent_document_slug_valid(const char* slug);
const char* s64_persistent_document_type_name(S64DocumentType type);
S64DocumentResult s64_persistent_document_write_wire(
    const S64Document* document,
    S64DocumentSink sink,
    void* context
);

#endif
