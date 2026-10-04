#ifndef SEE64YOU_MANAGED_EDITOR_STATE_H
#define SEE64YOU_MANAGED_EDITOR_STATE_H
#include "platform/managed_editor.h"
#include "platform/application_state.h"

typedef struct {
    uint32_t generation;
    uint8_t source_module;
    uint8_t document_replaced;
} S64ManagedEditorState;

/* After trusted adoption, while IO_SAFE. The synchronous wire workspace must
 * survive the RUNTIME -> FILE switch unchanged. Failure retains ACTIVE.
 * Returns 0 for failure, 1 for fresh entry, 2 for authenticated resumption. */
uint8_t s64_managed_editor_state_prepare(S64ModuleRuntime* runtime, uint8_t* wire);
/* One-shot decode. Caller supplies 1 KiB synchronous scratch; returned control
 * state occupies its second half and must be consumed before scratch reuse. */
S64ApplicationState* s64_managed_editor_state_decode(S64ModuleRuntime* runtime, uint8_t* wire);
#endif
