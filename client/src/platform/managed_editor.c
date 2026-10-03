#include "platform/managed_editor.h"
#include "platform/code_assets.h"
#include "platform/reu.h"

uint8_t s64_managed_editor_adopt(S64ModuleRuntime* runtime, S64Dos* dos,
    uint16_t version)
{
    if (!runtime || !dos || !version) return 0u;
    s64_module_runtime_init_hardware(runtime, dos);
    return s64_module_runtime_adopt(runtime, S64_MODULE_EDITOR, version, 1u)
        && s64_system_state_set_mode(&runtime->system_state,
            S64_SYSTEM_MODE_IO_SAFE) == S64_SYSTEM_OK
        && s64_code_assets_bind(&runtime->reu_control,
            &runtime->lifecycle.journal, &runtime->module_manifest.modules[0],
            1u, (void*)S64_CODE_OVERLAY_ADDRESS,
            S64_CODE_OVERLAY_CAPACITY, s64_reu_load);
}

S64SessionLifecycleResult s64_managed_editor_end(S64ModuleRuntime* runtime)
{
    if (!runtime) return S64_SESSION_LIFECYCLE_INVALID;
    /* Once all shared state is revoked, retry only restore/CLEAN. A failed
     * CLEAN save may already have restored the snapshot and closed I/O scope. */
    if (runtime->shared_invalidated) return s64_module_runtime_end(runtime);
    if (s64_system_state_set_mode(&runtime->system_state,
            S64_SYSTEM_MODE_IO_SAFE) != S64_SYSTEM_OK
        || !s64_code_assets_clear(&runtime->reu_control))
        return S64_SESSION_LIFECYCLE_RESTORE_FAILED;
    return s64_module_runtime_end(runtime);
}
