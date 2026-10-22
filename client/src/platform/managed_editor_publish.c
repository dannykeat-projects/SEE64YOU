#include "platform/managed_editor_state.h"
#include "platform/application_state_internal.h"

typedef char S64ManagedCaptureFits[512 - (int)sizeof(S64ApplicationState)];

uint8_t s64_managed_editor_state_publish(S64ModuleRuntime* runtime,
    S64ManagedEditorState* authority, S64Editor* editor,
    const S64Document* document, const S64LocalDocument* local,
    const S64Clipboard* clipboard, const char* search, uint8_t search_valid,
    uint8_t* scratch)
{
    S64ApplicationState* state;
    if (!runtime || !authority || !scratch
        || authority->generation == 0xffffffffUL
        || runtime->current_module != S64_MODULE_EDITOR
        || !runtime->lifecycle.active_record_written
        || runtime->lifecycle.clean_record_written || runtime->shared_invalidated
        || !runtime->system_state.captured || runtime->system_state.restored
        || runtime->system_state.mode != S64_SYSTEM_MODE_IO_SAFE
        || !s64_application_state_active(&runtime->reu_control,
            &runtime->lifecycle.journal, scratch)
        || !s64_application_state_allocation(&runtime->reu_control)) return 0;
    state = (S64ApplicationState*)(scratch + S64_APPLICATION_STATE_SIZE);
    if (!s64_application_state_capture(state, editor, document, local,
            clipboard, search, search_valid, authority->generation + 1UL)) return 0;
    state->document_replaced = authority->document_replaced;
    /* A resumed record must lose authority durably before any overwrite. */
    if (!s64_managed_editor_state_revoke(runtime)
        || !s64_application_state_store(&runtime->reu_control,
            &runtime->lifecycle.journal, state, scratch)) return 0;
    runtime->reu_control.state_flags |= S64_REU_RUNTIME_STATE_APPLICATION_VALID;
    if (!s64_reu_runtime_store_verified(&runtime->reu_control)) {
        /* Even a failed readback may follow a successful set on hardware.
         * Retry a verified clear; failure still never authorizes transition. */
        s64_managed_editor_state_revoke(runtime);
        return 0;
    }
    authority->generation = state->generation;
    authority->source_module = state->source_module;
    return 1;
}
