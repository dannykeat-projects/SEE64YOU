#include "launcher/basic_return_runtime.h"
#include "launcher/code_assets_runtime.h"
#include "platform/basic_return.h"

uint8_t s64_launcher_basic_return_prepare(S64LauncherRecoveryRuntime* runtime)
{
    if (!runtime || !runtime->lifecycle.active_record_written
        || runtime->lifecycle.clean_record_written || !runtime->system_state.captured
        || runtime->system_state.restored || runtime->system_state.mode != S64_SYSTEM_MODE_IO_SAFE
        || !s64_basic_return_reserve(&runtime->reu_control)) return 0;
    runtime->basic_return_reserved = 1;
    return s64_reu_runtime_store_verified(&runtime->reu_control)
        && s64_basic_return_publish(&runtime->reu_control,
            &runtime->lifecycle.journal, S64_BASIC_RETURN_WORKSPACE);
}

S64SessionLifecycleResult s64_launcher_basic_return_end(S64LauncherRecoveryRuntime* runtime)
{
    if (!runtime) return S64_SESSION_LIFECYCLE_INVALID;
    if (runtime->basic_return_reserved && !runtime->module_session_invalidated
        && (s64_system_state_set_mode(&runtime->system_state, S64_SYSTEM_MODE_IO_SAFE) != S64_SYSTEM_OK
            || !s64_basic_return_revoke(&runtime->reu_control, S64_BASIC_RETURN_WORKSPACE)))
        return S64_SESSION_LIFECYCLE_RESTORE_FAILED;
    return s64_launcher_code_assets_end(runtime);
}
