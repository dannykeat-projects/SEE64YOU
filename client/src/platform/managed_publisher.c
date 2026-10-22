#include "platform/managed_publisher.h"
#include "platform/application_state.h"
#include "platform/code_assets.h"
#include "platform/reu.h"
#include "publisher/workspace.h"
/* One-shot authority for the separately placed CONTENT decoder. */
S64ModuleRuntime* s64_managed_publisher_document_owner;
uint8_t s64_managed_publisher_adopt(S64ModuleRuntime* runtime, S64Dos* dos,
    uint16_t version)
{
    s64_managed_publisher_document_owner = 0;
    if (!runtime || !dos || !version) return 0u;
    s64_module_runtime_init_hardware(runtime, dos);
    if (!(s64_module_runtime_adopt(runtime, S64_MODULE_PUBLISHER, version, 1u)
        && s64_system_state_set_mode(&runtime->system_state,
            S64_SYSTEM_MODE_IO_SAFE) == S64_SYSTEM_OK
        && s64_code_assets_bind(&runtime->reu_control,
            &runtime->lifecycle.journal, &runtime->module_manifest.modules[1],
            1u, (void*)S64_CODE_OVERLAY_ADDRESS,
            S64_CODE_OVERLAY_CAPACITY, s64_reu_load)
        && s64_application_state_load_wire(&runtime->reu_control,
            &runtime->lifecycle.journal, s64_publisher_workspace.bytes))) return 0u;
    s64_managed_publisher_document_owner = runtime;
    return 1u;
}
