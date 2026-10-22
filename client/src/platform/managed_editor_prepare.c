#include "platform/managed_editor_state.h"

uint8_t s64_managed_editor_state_cancel(S64ModuleRuntime* runtime)
{
    if (!runtime) return 0;
    if (!(runtime->reu_control.state_flags & S64_REU_RUNTIME_STATE_APPLICATION_VALID)) return 1;
    return s64_system_state_set_mode(&runtime->system_state,
            S64_SYSTEM_MODE_IO_SAFE) == S64_SYSTEM_OK
        && s64_managed_editor_state_revoke(runtime)
        && s64_system_state_set_mode(&runtime->system_state,
            S64_SYSTEM_MODE_INTERACTIVE) == S64_SYSTEM_OK;
}

uint8_t s64_managed_editor_state_prepare_publish(S64ModuleRuntime* runtime,
    S64ManagedEditorState* authority, S64Editor* editor,
    const S64Document* document, const S64LocalDocument* local,
    const S64Clipboard* clipboard, const char* search, uint8_t search_valid,
    uint8_t* scratch)
{
    uint8_t published;
    if (!runtime || runtime->current_module != S64_MODULE_EDITOR
        || !runtime->lifecycle.active_record_written
        || runtime->lifecycle.clean_record_written || runtime->shared_invalidated
        || runtime->system_state.mode != S64_SYSTEM_MODE_INTERACTIVE) return 2;
    if (s64_system_state_set_mode(&runtime->system_state,
            S64_SYSTEM_MODE_IO_SAFE) != S64_SYSTEM_OK) return 2;
    published = s64_managed_editor_state_publish(runtime, authority, editor,
        document, local, clipboard, search, search_valid, scratch);
    /* Capture/history can fail before the producer revokes old authority.
     * A failed or abandoned preparation must never expose that old snapshot. */
    if (!published && !s64_managed_editor_state_revoke(runtime)) return 2;
    if (s64_system_state_set_mode(&runtime->system_state,
            S64_SYSTEM_MODE_INTERACTIVE) != S64_SYSTEM_OK) return 2;
    return published;
}
