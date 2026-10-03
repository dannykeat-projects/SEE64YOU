#include "platform/module_manifest.h"
#include <string.h>

static void init_module(
    S64ModuleDescriptor* module,
    S64ModuleId id,
    const char* filename,
    uint16_t app_version
)
{
    memset(module, 0, sizeof(*module));
    module->id = (uint8_t)id;
    strncpy(module->filename, filename, sizeof(module->filename) - 1u);
    module->abi_version = S64_REU_RUNTIME_ABI_VERSION;
    module->app_version = app_version;
    module->cache_capacity = S64_MODULE_CACHE_CAPACITY;
}

void s64_module_manifest_init(S64ModuleManifest* manifest, uint16_t app_version)
{
    if (manifest == 0) return;
    memset(manifest, 0, sizeof(*manifest));
    init_module(&manifest->modules[0], S64_MODULE_EDITOR, "S64EDIT.PRG", app_version);
    init_module(&manifest->modules[1], S64_MODULE_PUBLISHER, "S64PUB.PRG", app_version);
    init_module(&manifest->modules[2], S64_MODULE_SETTINGS, "S64CFG.PRG", app_version);
}

uint8_t s64_module_descriptor_identity_valid(const S64ModuleDescriptor* module)
{
    return module != 0
        && module->id >= S64_MODULE_EDITOR
        && module->id <= S64_MODULE_SETTINGS
        && module->filename[0] != '\0'
        && module->abi_version == S64_REU_RUNTIME_ABI_VERSION
        && module->app_version != 0u
        && module->cache_capacity != 0UL;
}

uint8_t s64_module_descriptor_loadable(const S64ModuleDescriptor* module)
{
    return s64_module_descriptor_identity_valid(module)
        && module->load_address != 0u
        && module->startup_address != 0u
        && module->byte_length > 2UL
        && module->byte_length <= module->cache_capacity
        && module->integrity != 0UL
        && module->cache_address >= S64_REU_RUNTIME_HEAP_START;
}

uint8_t s64_module_manifest_loadable(const S64ModuleManifest* manifest)
{
    uint8_t i;
    if (manifest == 0) return 0u;
    for (i = 0u; i < S64_MODULE_COUNT; ++i) {
        if (!s64_module_descriptor_loadable(&manifest->modules[i])) {
            return 0u;
        }
    }
    return 1u;
}
