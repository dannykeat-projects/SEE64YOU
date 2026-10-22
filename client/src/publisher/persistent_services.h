#ifndef SEE64YOU_PUBLISHER_PERSISTENT_SERVICES_H
#define SEE64YOU_PUBLISHER_PERSISTENT_SERVICES_H

#include "document.h"
#include "platform/reu_runtime.h"

/* One reusable allocator entry: headers, body, then original physical buffer. */
#define S64_OPEN_BODY_OFFSET 4096UL
#define S64_OPEN_BACKUP_OFFSET 65536UL
#define S64_OPEN_STORAGE_SIZE (S64_OPEN_BACKUP_OFFSET + S64_REU_DOCUMENT_CAPACITY)
#define S64_OPEN_STORAGE_FLAG 0x4fu

/*
 * Narrow services shared by persistent Publisher overlays without making them
 * depend directly on Editor overlay code. The diagnostic implementation keeps
 * document streaming explicitly gated until the shared codec slice is migrated.
 */

typedef struct {
    uint32_t address;
    uint8_t complete;
    uint8_t recovery_required;
    uint16_t header_length;
    uint16_t body_length;
    uint8_t header_chunk_length;
    uint8_t line_empty;
    uint8_t body_started;
    S64DocumentResult result;
} S64PersistentOpenStage;

/* Uses the same live allocator as the owner; like runtime_allocate, this does
 * not publish a control record. The owner handles persistence in its lifecycle
 * namespace, never by nesting an Editor overlay beneath a Publisher overlay.
 * Call before network reception. One synchronous
 * operation at a time. Never reinitialize a stage with recovery_required set. */
uint8_t s64_persistent_open_stage_init(
    S64PersistentOpenStage* stage, S64ReuRuntimeControl* control);
uint8_t s64_persistent_open_stage_finish(S64PersistentOpenStage* stage);
/* Retry restoration after a failed commit; retain stage and stop editing until
 * this succeeds. No journal/power-loss recovery is implied. */
uint8_t s64_persistent_open_stage_restore(S64PersistentOpenStage* stage);
uint8_t s64_persistent_open_stage_write(
    const uint8_t* data,
    uint16_t length,
    void* context
);
S64DocumentResult s64_persistent_open_stage_commit(
    S64PersistentOpenStage* stage,
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
