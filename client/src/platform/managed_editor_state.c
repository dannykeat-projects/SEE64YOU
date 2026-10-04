#include "platform/managed_editor_state.h"
#include "platform/application_state_internal.h"

/* One-shot authorization for the separately placed content decoder. */
S64ModuleRuntime* s64_managed_editor_state_owner;
uint8_t* s64_managed_editor_state_wire;

uint8_t s64_managed_editor_state_prepare(S64ModuleRuntime* runtime, uint8_t* wire)
{
    uint8_t expected[S64_SESSION_JOURNAL_WIRE_SIZE];
    s64_managed_editor_state_owner = 0;
    s64_managed_editor_state_wire = 0;
    if (!runtime || !wire || runtime->current_module != S64_MODULE_EDITOR
        || !s64_application_state_active(&runtime->reu_control,
            &runtime->lifecycle.journal, expected)) return 0;
    if (runtime->reu_control.state_flags & S64_REU_RUNTIME_STATE_APPLICATION_VALID) {
        if (!s64_application_state_load_wire(&runtime->reu_control,
                &runtime->lifecycle.journal, wire)) return 0;
        s64_managed_editor_state_owner = runtime;
        s64_managed_editor_state_wire = wire;
        return 2;
    }
    return 1;
}
