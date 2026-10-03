#include "platform/code_assets_internal.h"
#include "platform/crc32.h"
#include "platform/module_integrity.h"
#include <string.h>

static const char* const filenames[S64_CODE_OVERLAY_COUNT] = {
    "S64MODEL.BIN", "S64FILE.BIN", "S64RUN.BIN"
};
static uint16_t get16(const uint8_t* in)
{ return (uint16_t)(in[0] | ((uint16_t)in[1] << 8)); }
static uint32_t get32(const uint8_t* in)
{ return (uint32_t)get16(in) | ((uint32_t)get16(in + 2) << 16); }

uint8_t s64_code_assets_parse(S64CodeAssets* assets, const uint8_t* wire,
    uint16_t length, const S64ModuleDescriptor* editor)
{
    S64CodeAssets parsed;
    uint8_t i;
    if (!assets || !wire || length != S64_CODE_ASSET_MANIFEST_SIZE
        || !s64_code_assets_editor_valid(editor) || memcmp(wire, "S64O", 4) != 0
        || wire[4] != 1 || wire[5] != S64_CODE_OVERLAY_COUNT
        || get16(wire + 6) != S64_REU_RUNTIME_ABI_VERSION
        || get16(wire + 8) != editor->app_version
        || get16(wire + 10) != S64_CODE_OVERLAY_ADDRESS
        || get16(wire + 12) != S64_CODE_OVERLAY_CAPACITY
        || get16(wire + 14) != 0
        || get32(wire + 16) != editor->byte_length
        || get32(wire + 20) != editor->integrity
        || get32(wire + 48) != (s64_crc32_update(0xffffffffUL, wire, 48) ^ 0xffffffffUL))
        return 0;
    memset(&parsed, 0, sizeof(parsed));
    parsed.abi_version = get16(wire + 6);
    parsed.app_version = get16(wire + 8);
    parsed.editor_length = get32(wire + 16);
    parsed.editor_integrity = get32(wire + 20);
    for (i = 0; i < S64_CODE_OVERLAY_COUNT; ++i) {
        parsed.assets[i].cache.length = get32(wire + 24 + i * 8u);
        parsed.assets[i].integrity = get32(wire + 28 + i * 8u);
        if (!parsed.assets[i].cache.length
            || parsed.assets[i].cache.length > S64_CODE_OVERLAY_CAPACITY
            || !parsed.assets[i].integrity) return 0;
    }
    *assets = parsed;
    return 1;
}

uint8_t s64_code_assets_allocate(S64CodeAssets* assets, S64ReuRuntimeControl* control)
{
    S64ReuRuntimeControl planned;
    S64CodeAssets planned_assets;
    uint8_t i;
    if (!assets || !s64_reu_runtime_valid(control)
        || assets->app_version != control->app_version || !assets->app_version
        || assets->abi_version != S64_REU_RUNTIME_ABI_VERSION
        || !assets->editor_length || !assets->editor_integrity) return 0;
    for (i = 0; i < control->allocation_count; ++i) {
        const S64ReuAllocation* item = &control->allocations[i];
        if ((item->owner == S64_REU_OWNER_RUNTIME && item->flags == S64_CODE_ASSET_SESSION_FLAG)
            || (item->owner == S64_REU_OWNER_MODULE && item->flags >= S64_CODE_ASSET_CACHE_FLAG
                && item->flags < S64_CODE_ASSET_CACHE_FLAG + S64_CODE_OVERLAY_COUNT)) return 0;
    }
    planned = *control;
    planned_assets = *assets;
    for (i = 0; i < S64_CODE_OVERLAY_COUNT; ++i) {
        S64CodeAsset* asset = &planned_assets.assets[i];
        if (!asset->cache.length || asset->cache.length > S64_CODE_OVERLAY_CAPACITY
            || !asset->integrity || asset->cache_state != S64_MODULE_CACHE_EMPTY
            || asset->cache.owner != S64_REU_OWNER_NONE
            || !s64_reu_runtime_allocate(&planned, S64_REU_OWNER_MODULE,
                asset->cache.length, 256, &asset->cache)) return 0;
        asset->cache.flags = S64_CODE_ASSET_CACHE_FLAG + i;
        planned.allocations[planned.allocation_count - 1u].flags = asset->cache.flags;
    }
    if (!s64_reu_runtime_allocate(&planned, S64_REU_OWNER_RUNTIME,
        S64_CODE_ASSET_SESSION_SIZE, 256, 0)) return 0;
    planned.allocations[planned.allocation_count - 1u].flags = S64_CODE_ASSET_SESSION_FLAG;
    *control = planned;
    *assets = planned_assets;
    return 1;
}

uint8_t s64_code_assets_preload(S64Dos* dos, const S64ReuRuntimeControl* control,
    const S64ModuleDescriptor* editor, S64CodeAssets* assets, S64SystemState* system)
{
    uint8_t i;
    uint32_t size;
    S64DosResult result, closed;
    if (!assets) return 0;
    for (i = 0; i < S64_CODE_OVERLAY_COUNT; ++i) assets->assets[i].cache_state = S64_MODULE_CACHE_EMPTY;
    if (!dos || !system || !system->captured || system->restored
        || system->mode != S64_SYSTEM_MODE_IO_SAFE
        || !s64_code_assets_clear(control)
        || !s64_code_assets_valid(assets, control, editor, 0)
        || s64_dos_change_dir(dos, "/sd/") != S64_DOS_OK) return 0;
    for (i = 0; i < S64_CODE_OVERLAY_COUNT; ++i) {
        S64CodeAsset* asset = &assets->assets[i];
        if (s64_dos_open_read(dos, filenames[i]) != S64_DOS_OK) return 0;
        result = s64_dos_file_size(dos, &size);
        if (result == S64_DOS_OK && size != asset->cache.length) result = S64_DOS_TARGET_ERROR;
        if (result == S64_DOS_OK) result = s64_dos_load_reu(dos, asset->cache.address, asset->cache.length);
        closed = s64_dos_close(dos);
        if (result != S64_DOS_OK || closed != S64_DOS_OK) return 0;
        asset->cache_state = S64_MODULE_CACHE_LOADED_UNVERIFIED;
        if (!s64_reu_crc32_matches(asset->cache.address, asset->cache.length, asset->integrity, system)) {
            s64_system_state_set_mode(system, S64_SYSTEM_MODE_IO_SAFE);
            return 0;
        }
    }
    for (i = 0; i < S64_CODE_OVERLAY_COUNT; ++i) assets->assets[i].cache_state = S64_MODULE_CACHE_READY;
    return 1;
}
