#include "platform/module_manifest.h"

uint8_t s64_module_manifest_allocate(
    S64ModuleManifest* manifest,
    S64ReuRuntimeControl* control
)
{
    uint8_t i;
    S64ReuAllocation allocation;
    if (manifest == 0 || !s64_reu_runtime_valid(control)) return 0u;

    for (i = 0u; i < S64_MODULE_COUNT; ++i) {
        if (!s64_module_descriptor_identity_valid(&manifest->modules[i])
            || !s64_reu_runtime_allocate(
                control,
                S64_REU_OWNER_MODULE,
                manifest->modules[i].cache_capacity,
                256u,
                &allocation
            )) {
            return 0u;
        }
        manifest->modules[i].cache_address = allocation.address;
    }
    return 1u;
}
