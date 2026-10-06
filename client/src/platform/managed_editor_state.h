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

/* Caller owns managed IO_SAFE throughout. Scratch is the same synchronous
 * 1 KiB entry workspace, never a loaded BASIC-return snapshot. Success is a
 * prerequisite for a later transition; existing execution gates still apply.
 * Failure leaves the current Editor in charge. */
uint8_t s64_managed_editor_state_publish(S64ModuleRuntime* runtime,
    S64ManagedEditorState* authority, S64Editor* editor,
    const S64Document* document, const S64LocalDocument* local,
    const S64Clipboard* clipboard, const char* search, uint8_t search_valid,
    uint8_t* scratch);
/* IO_SAFE; only control persistence is touched, preserving exit scratch.
 * A failed clear is uncertain and must block both transition and CLEAN. */
uint8_t s64_managed_editor_state_revoke(S64ModuleRuntime* runtime);
/* Preparation only: 0 = failed and safely interactive, 1 = published,
 * 2 = recovery required. Successful authority survives until transition or
 * explicit cancellation before the next live Editor event. No executable
 * transition is exposed by this API. */
uint8_t s64_managed_editor_state_prepare_publish(S64ModuleRuntime* runtime,
    S64ManagedEditorState* authority, S64Editor* editor,
    const S64Document* document, const S64LocalDocument* local,
    const S64Clipboard* clipboard, const char* search, uint8_t search_valid,
    uint8_t* scratch);
/* Revoke a successful preparation before live controls/payloads can change.
 * Failure requires recovery; a clear local bit is used only after successful
 * publication/revocation, never to recover an uncertain failed operation. */
uint8_t s64_managed_editor_state_cancel(S64ModuleRuntime* runtime);
#endif
