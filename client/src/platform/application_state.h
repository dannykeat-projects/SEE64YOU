#ifndef SEE64YOU_PLATFORM_APPLICATION_STATE_H
#define SEE64YOU_PLATFORM_APPLICATION_STATE_H

#include "editor.h"
#include "document.h"
#include "storage/local.h"
#include "platform/module_manifest.h"
#include "platform/session_journal.h"

#ifndef S64_REU_BUFFER
#error "Shared application state requires the REU editor layout"
#endif

#define S64_APPLICATION_STATE_SIZE 512u
#define S64_APPLICATION_STATE_FLAG 6u
#define S64_APPLICATION_SEARCH_CAPACITY 33u

/* Control state only. Payloads remain in the migration-era banks 0-1. */
typedef struct {
    S64Editor editor;
    S64DocumentSummary document;
    char filename[S64_LOCAL_FILENAME_CAPACITY];
    uint16_t clipboard_length;
    char search[S64_APPLICATION_SEARCH_CAPACITY];
    uint8_t search_valid;
    uint8_t document_replaced;
    uint8_t source_module;
    uint32_t generation;
} S64ApplicationState;

uint8_t s64_application_state_reserve(S64ReuRuntimeControl* control);
const S64ReuAllocation* s64_application_state_allocation(const S64ReuRuntimeControl* control);
uint8_t s64_application_state_encode(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const S64ApplicationState* state, uint8_t* wire);
/* Failed decode/load leaves state unchanged. Caller supplies 512-byte scratch. */
uint8_t s64_application_state_decode(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, S64ApplicationState* state, const uint8_t* wire);
/* Caller owns IO_SAFE; store verifies every byte through bounded readback. */
uint8_t s64_application_state_store(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const S64ApplicationState* state, uint8_t* wire);
uint8_t s64_application_state_load(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, S64ApplicationState* state, uint8_t* wire);

/* Split overlay bootstrap: RUNTIME authenticates the wire, then CONTENT decodes
 * its ranges without importing runtime routines. Scratch must remain unchanged
 * between these synchronous stages. decode_payload alone is not authorization. */
uint8_t s64_application_state_load_wire(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, uint8_t* wire);
uint8_t s64_application_state_decode_payload(S64ApplicationState* state, const uint8_t* wire);

/* Editor producer: flush history before capturing any transition state. */
uint8_t s64_application_state_capture(S64ApplicationState* state, S64Editor* editor,
    const S64Document* document, const S64LocalDocument* local,
    const S64Clipboard* clipboard, const char* search, uint8_t search_valid,
    uint32_t generation);
/* Deliberate remote-open/import only; no-op round trips do not call this. */
uint8_t s64_application_state_replace(S64ApplicationState* state,
    const S64Buffer* body, const S64Document* document);

#endif
