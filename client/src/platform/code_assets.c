#include "platform/code_assets_internal.h"
#include "platform/crc32.h"
#include "platform/module_integrity.h"
#include <string.h>

static uint16_t get16(const uint8_t* in)
{ return (uint16_t)(in[0] | ((uint16_t)in[1] << 8)); }
static uint32_t get32(const uint8_t* in)
{ return (uint32_t)get16(in) | ((uint32_t)get16(in + 2) << 16); }

uint8_t s64_code_assets_parse_for(const S64CodeAssetNamespace* namespace,
    S64CodeAssets* assets, const uint8_t* wire, uint16_t length,
    const S64ModuleDescriptor* editor)
{
    S64CodeAssets parsed;
    uint16_t manifest_size;
    uint16_t crc_offset;
    uint8_t i;
    if (!namespace || !namespace->overlay_count
        || namespace->overlay_count > S64_CODE_OVERLAY_COUNT) return 0;
    manifest_size = (uint16_t)(28u + (uint16_t)namespace->overlay_count * 8u);
    crc_offset = (uint16_t)(manifest_size - 4u);
    if (!assets || !wire || length != manifest_size
        || !s64_code_assets_editor_valid_for(namespace, editor)
        || wire[0] != 0x53u || wire[1] != 0x36u || wire[2] != 0x34u
        || wire[3] != namespace->magic_last
        || wire[4] != S64_CODE_ASSET_FORMAT_VERSION
        || wire[5] != namespace->overlay_count
        || get16(wire + 6) != S64_REU_RUNTIME_ABI_VERSION
        || get16(wire + 8) != editor->app_version
        || get16(wire + 10) != S64_CODE_OVERLAY_ADDRESS
        || get16(wire + 12) != S64_CODE_OVERLAY_CAPACITY
        || get16(wire + 14) != 0
        || get32(wire + 16) != editor->byte_length
        || get32(wire + 20) != editor->integrity
        || get32(wire + crc_offset)
            != (s64_crc32_update(0xffffffffUL, wire, crc_offset) ^ 0xffffffffUL))
        return 0;
    memset(&parsed, 0, sizeof(parsed));
    parsed.abi_version = get16(wire + 6);
    parsed.app_version = get16(wire + 8);
    parsed.editor_length = get32(wire + 16);
    parsed.editor_integrity = get32(wire + 20);
    for (i = 0; i < namespace->overlay_count; ++i) {
        parsed.assets[i].cache.length = get32(wire + 24 + i * 8u);
        parsed.assets[i].integrity = get32(wire + 28 + i * 8u);
        if (!parsed.assets[i].cache.length
            || parsed.assets[i].cache.length > S64_CODE_OVERLAY_CAPACITY
            || !parsed.assets[i].integrity) return 0;
    }
    *assets = parsed;
    return 1;
}

uint8_t s64_code_assets_parse(S64CodeAssets* assets, const uint8_t* wire,
    uint16_t length, const S64ModuleDescriptor* editor)
{
    return s64_code_assets_parse_for(
        s64_code_assets_namespace(S64_CODE_ASSET_MODULE),
        assets, wire, length, editor);
}

uint8_t s64_code_assets_allocate_for(const S64CodeAssetNamespace* namespace,
    S64CodeAssets* assets, S64ReuRuntimeControl* control)
{
    S64ReuRuntimeControl planned;
    S64CodeAssets planned_assets;
    uint16_t session_size;
    uint8_t i;
    if (!namespace || !assets || !s64_reu_runtime_valid(control)
        || !namespace->overlay_count
        || namespace->overlay_count > S64_CODE_OVERLAY_COUNT
        || assets->app_version != control->app_version || !assets->app_version
        || assets->abi_version != S64_REU_RUNTIME_ABI_VERSION
        || !assets->editor_length || !assets->editor_integrity) return 0;
    for (i = 0; i < control->allocation_count; ++i) {
        const S64ReuAllocation* item = &control->allocations[i];
        if ((item->owner == S64_REU_OWNER_RUNTIME
                && item->flags == namespace->session_flag)
            || (item->owner == S64_REU_OWNER_MODULE
                && item->flags >= namespace->cache_flag
                && item->flags < namespace->cache_flag + namespace->overlay_count))
            return 0;
    }
    planned = *control;
    planned_assets = *assets;
    for (i = 0; i < namespace->overlay_count; ++i) {
        S64CodeAsset* asset = &planned_assets.assets[i];
        if (!asset->cache.length || asset->cache.length > S64_CODE_OVERLAY_CAPACITY
            || !asset->integrity || asset->cache_state != S64_MODULE_CACHE_EMPTY
            || asset->cache.owner != S64_REU_OWNER_NONE
            || !s64_reu_runtime_allocate(&planned, S64_REU_OWNER_MODULE,
                asset->cache.length, 256, &asset->cache)) return 0;
        asset->cache.flags = (uint8_t)(namespace->cache_flag + i);
        planned.allocations[planned.allocation_count - 1u].flags = asset->cache.flags;
    }
    session_size = (uint16_t)(48u + (uint16_t)namespace->overlay_count * 16u);
    if (!s64_reu_runtime_allocate(&planned, S64_REU_OWNER_RUNTIME,
        session_size, 256, 0)) return 0;
    planned.allocations[planned.allocation_count - 1u].flags = namespace->session_flag;
    *control = planned;
    *assets = planned_assets;
    return 1;
}

uint8_t s64_code_assets_allocate(S64CodeAssets* assets, S64ReuRuntimeControl* control)
{
    return s64_code_assets_allocate_for(
        s64_code_assets_namespace(S64_CODE_ASSET_MODULE), assets, control);
}

uint8_t s64_code_assets_preload_for(const S64CodeAssetNamespace* namespace,
    S64Dos* dos, const S64ReuRuntimeControl* control,
    const S64ModuleDescriptor* editor, S64CodeAssets* assets,
    S64SystemState* system)
{
    uint8_t i;
    uint32_t size;
    S64DosResult result, closed;
    if (!namespace || !assets || !namespace->overlay_count
        || namespace->overlay_count > S64_CODE_OVERLAY_COUNT) return 0;
    for (i = 0; i < namespace->overlay_count; ++i)
        assets->assets[i].cache_state = S64_MODULE_CACHE_EMPTY;
    if (!dos || !system || !system->captured || system->restored
        || system->mode != S64_SYSTEM_MODE_IO_SAFE
        || !s64_code_assets_clear_for(namespace, control)
        || !s64_code_assets_valid_for(namespace, assets, control, editor, 0)
        || s64_dos_change_dir(dos, "/sd/") != S64_DOS_OK) return 0;
    for (i = 0; i < namespace->overlay_count; ++i) {
        S64CodeAsset* asset = &assets->assets[i];
        if (!namespace->filenames[i]
            || s64_dos_open_read(dos, namespace->filenames[i]) != S64_DOS_OK) return 0;
        result = s64_dos_file_size(dos, &size);
        if (result == S64_DOS_OK && size != asset->cache.length) result = S64_DOS_TARGET_ERROR;
        if (result == S64_DOS_OK)
            result = s64_dos_load_reu(dos, asset->cache.address, asset->cache.length);
        closed = s64_dos_close(dos);
        if (result != S64_DOS_OK || closed != S64_DOS_OK) return 0;
        asset->cache_state = S64_MODULE_CACHE_LOADED_UNVERIFIED;
        if (!s64_reu_crc32_matches(asset->cache.address, asset->cache.length,
                asset->integrity, system)) {
            s64_system_state_set_mode(system, S64_SYSTEM_MODE_IO_SAFE);
            return 0;
        }
    }
    for (i = 0; i < namespace->overlay_count; ++i)
        assets->assets[i].cache_state = S64_MODULE_CACHE_READY;
    return 1;
}

uint8_t s64_code_assets_preload(S64Dos* dos, const S64ReuRuntimeControl* control,
    const S64ModuleDescriptor* editor, S64CodeAssets* assets, S64SystemState* system)
{
    return s64_code_assets_preload_for(
        s64_code_assets_namespace(S64_CODE_ASSET_MODULE),
        dos, control, editor, assets, system);
}
