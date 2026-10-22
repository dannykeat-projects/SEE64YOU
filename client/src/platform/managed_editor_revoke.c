#include "platform/managed_editor_state.h"

uint8_t s64_managed_editor_state_revoke(S64ModuleRuntime* runtime)
{
    if (!runtime || !s64_reu_runtime_valid(&runtime->reu_control)
        || runtime->system_state.mode != S64_SYSTEM_MODE_IO_SAFE) return 0;
    /* Persist even when the local bit is clear: a prior failed clear/readback
     * may have left a set bit in REU. Never infer authority from stale bytes. */
    runtime->reu_control.state_flags &= (uint8_t)~S64_REU_RUNTIME_STATE_APPLICATION_VALID;
    return s64_reu_runtime_store_verified(&runtime->reu_control);
}
