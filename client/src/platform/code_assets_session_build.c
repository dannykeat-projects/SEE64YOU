#include "platform/code_assets_internal.h"
#include "platform/reu.h"
#include <string.h>

static void put16(uint8_t* out, uint16_t value)
{ out[0] = (uint8_t)value; out[1] = (uint8_t)(value >> 8); }
static void put32(uint8_t* out, uint32_t value)
{ put16(out, (uint16_t)value); put16(out + 2, (uint16_t)(value >> 16)); }

uint8_t s64_code_assets_store_verified_for(
    const S64CodeAssetNamespace* namespace,
    const S64ReuRuntimeControl* control, const S64SessionJournal* journal,
    const S64ModuleDescriptor* editor, const S64CodeAssets* assets)
{
    const S64ReuAllocation* allocation;
    uint16_t trailer;
    uint16_t size;
    uint8_t i;
    uint8_t offset;
    if (!namespace || !namespace->session_magic
        || !namespace->overlay_count
        || namespace->overlay_count > S64_CODE_OVERLAY_COUNT) return 0;
    allocation = s64_code_assets_session_allocation_for(namespace, control);
    size = (uint16_t)(48u + (uint16_t)namespace->overlay_count * 16u);
    trailer = (uint16_t)(40u + (uint16_t)namespace->overlay_count * 16u);
    if (!allocation || size > sizeof(s64_code_assets_session_wire)
        || !s64_code_assets_clear_for(namespace, control)
        || !s64_code_assets_active_session(control, journal)
        || !s64_code_assets_valid_for(namespace, assets, control, editor, 1)) return 0;
    memcpy(s64_code_assets_session_wire, namespace->session_magic, 4);
    s64_code_assets_session_wire[4] = S64_CODE_ASSET_FORMAT_VERSION;
    s64_code_assets_session_wire[5] = namespace->overlay_count;
    put16(s64_code_assets_session_wire + 6, S64_REU_RUNTIME_ABI_VERSION);
    memcpy(s64_code_assets_session_wire + 8, s64_code_assets_session_journal_wire,
        sizeof(s64_code_assets_session_journal_wire));
    put32(s64_code_assets_session_wire + 32, assets->editor_length);
    put32(s64_code_assets_session_wire + 36, assets->editor_integrity);
    for (i = 0; i < namespace->overlay_count; ++i) {
        const S64CodeAsset* asset = &assets->assets[i];
        offset = (uint8_t)(40u + i * 16u);
        s64_code_assets_session_wire[offset] = i + 1u;
        s64_code_assets_session_wire[offset + 1] = S64_MODULE_CACHE_READY;
        s64_code_assets_session_wire[offset + 2] = asset->cache.flags;
        put32(s64_code_assets_session_wire + offset + 4, asset->cache.address);
        put32(s64_code_assets_session_wire + offset + 8, asset->cache.length);
        put32(s64_code_assets_session_wire + offset + 12, asset->integrity);
    }
    put16(s64_code_assets_session_wire + trailer, S64_CODE_OVERLAY_ADDRESS);
    put16(s64_code_assets_session_wire + trailer + 2u, S64_CODE_OVERLAY_CAPACITY);
    put16(s64_code_assets_session_wire + trailer + 6u,
        s64_code_assets_session_checksum_for(namespace, s64_code_assets_session_wire));
    if (!s64_reu_store((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, s64_code_assets_session_wire, size)
        || !s64_code_assets_session_verify_for(namespace, allocation)) {
        s64_code_assets_clear_for(namespace, control);
        return 0;
    }
    return 1;
}

uint8_t s64_code_assets_store_verified(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const S64ModuleDescriptor* editor,
    const S64CodeAssets* assets)
{
    return s64_code_assets_store_verified_for(
        s64_code_assets_namespace(S64_CODE_ASSET_MODULE),
        control, journal, editor, assets);
}
