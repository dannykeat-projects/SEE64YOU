#include "platform/managed_editor.h"
#include "platform/basic_return.h"
#include "platform/managed_editor_state.h"
#include <string.h>

uint8_t s64_managed_editor_exit(S64ModuleRuntime* runtime)
{
    uint8_t prepared;
    if (!runtime || s64_system_state_set_mode(&runtime->system_state,
            S64_SYSTEM_MODE_IO_SAFE) != S64_SYSTEM_OK) return 0;
    prepared = s64_basic_return_read(&runtime->reu_control,
        &runtime->lifecycle.journal, S64_BASIC_RETURN_WORKSPACE);
    /* Capsule/session teardown aliases the workspace's first half. Preserve
     * the validated context beyond both halves until CLEAN succeeds. */
    if (prepared) memcpy(S64_BASIC_RETURN_SNAPSHOT, S64_BASIC_RETURN_WORKSPACE,
        S64_BASIC_RETURN_RECORD_SIZE);
    if (!s64_managed_editor_state_revoke(runtime)
        || !s64_basic_return_revoke(&runtime->reu_control, S64_BASIC_RETURN_WORKSPACE)
        || s64_managed_editor_end(runtime) != S64_SESSION_LIFECYCLE_OK) return 0;
    if (prepared) s64_basic_return_execute(S64_BASIC_RETURN_SNAPSHOT);
    return 1; /* Missing/corrupt context: CLEAN, but retain the safe resident park. */
}
