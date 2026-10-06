#include "platform/code_assets_internal.h"

uint8_t s64_code_assets_editor_valid(const S64ModuleDescriptor* editor)
{
    return editor != 0 && editor->id == S64_CODE_ASSET_MODULE
        && s64_module_descriptor_loadable(editor)
        && editor->load_address == 0x0801u
        && editor->byte_length - 2UL <= S64_CODE_OVERLAY_ADDRESS - 0x0801UL
        && editor->startup_address >= editor->load_address
        && (uint32_t)editor->startup_address < editor->load_address + editor->byte_length - 2UL;
}

uint8_t s64_code_assets_valid(const S64CodeAssets* assets,
    const S64ReuRuntimeControl* control, const S64ModuleDescriptor* editor,
    uint8_t require_ready)
{
    uint8_t i, j, matches;
    if (!assets || !s64_reu_runtime_valid(control) || !s64_code_assets_editor_valid(editor)
        || editor->cache_state != S64_MODULE_CACHE_READY
        || assets->abi_version != S64_REU_RUNTIME_ABI_VERSION
        || assets->app_version != control->app_version
        || assets->app_version != editor->app_version
        || assets->editor_length != editor->byte_length
        || assets->editor_integrity != editor->integrity) return 0;
    for (i = 0; i < S64_CODE_OVERLAY_COUNT; ++i) {
        const S64CodeAsset* asset = &assets->assets[i];
        if (!asset->integrity || !asset->cache.length
            || asset->cache.length > S64_CODE_OVERLAY_CAPACITY
            || asset->cache.owner != S64_REU_OWNER_MODULE
            || asset->cache.flags != S64_CODE_ASSET_CACHE_FLAG + i
            || (require_ready && asset->cache_state != S64_MODULE_CACHE_READY)) return 0;
        matches = 0;
        for (j = 0; j < control->allocation_count; ++j) {
            const S64ReuAllocation* item = &control->allocations[j];
            if (item->owner == S64_REU_OWNER_MODULE && item->flags == asset->cache.flags) {
                if (item->address != asset->cache.address || item->length != asset->cache.length) return 0;
                ++matches;
            }
        }
        if (matches != 1) return 0;
    }
    return 1;
}
