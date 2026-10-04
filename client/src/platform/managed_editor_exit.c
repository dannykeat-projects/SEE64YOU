#include "platform/managed_editor.h"
#include "platform/basic_return.h"

uint8_t s64_managed_editor_exit(S64ModuleRuntime* runtime)
{
    uint8_t prepared;
    if (!runtime || s64_system_state_set_mode(&runtime->system_state,
            S64_SYSTEM_MODE_IO_SAFE) != S64_SYSTEM_OK) return 0;
    prepared = s64_basic_return_read(&runtime->reu_control,
        &runtime->lifecycle.journal, S64_BASIC_RETURN_WORKSPACE);
    /* Revocation uses separate scratch: keep the validated original snapshot
     * through asset/capsule teardown and journal I/O until CLEAN succeeds. */
    if (!s64_basic_return_revoke(&runtime->reu_control, S64_BASIC_RETURN_WORKSPACE)
        || s64_managed_editor_end(runtime) != S64_SESSION_LIFECYCLE_OK) return 0;
    if (prepared) s64_basic_return_execute(S64_BASIC_RETURN_WORKSPACE);
    return 1; /* Missing/corrupt context: CLEAN, but retain the safe resident park. */
}
