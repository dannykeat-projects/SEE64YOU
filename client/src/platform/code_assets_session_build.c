#include "platform/code_assets_internal.h"
#include "platform/reu.h"
#include <string.h>

static void put16(uint8_t* out, uint16_t value)
{ out[0] = (uint8_t)value; out[1] = (uint8_t)(value >> 8); }
static void put32(uint8_t* out, uint32_t value)
{ put16(out, (uint16_t)value); put16(out + 2, (uint16_t)(value >> 16)); }

uint8_t s64_code_assets_store_verified(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const S64ModuleDescriptor* editor, const S64CodeAssets* assets)
{
    const S64ReuAllocation* allocation = s64_code_assets_session_allocation(control);
    uint8_t i, offset;
    if (!allocation || !s64_code_assets_clear(control)
        || !s64_code_assets_active_session(control, journal)
        || !s64_code_assets_valid(assets, control, editor, 1)) return 0;
    memcpy(s64_code_assets_session_wire, S64_CODE_ASSET_SESSION_MAGIC, 4);
    s64_code_assets_session_wire[4] = 1;
    s64_code_assets_session_wire[5] = S64_CODE_OVERLAY_COUNT;
    put16(s64_code_assets_session_wire + 6, S64_REU_RUNTIME_ABI_VERSION);
    memcpy(s64_code_assets_session_wire + 8, s64_code_assets_session_journal_wire,
        sizeof(s64_code_assets_session_journal_wire));
    put32(s64_code_assets_session_wire + 32, assets->editor_length);
    put32(s64_code_assets_session_wire + 36, assets->editor_integrity);
    for (i = 0; i < S64_CODE_OVERLAY_COUNT; ++i) {
        const S64CodeAsset* asset = &assets->assets[i];
        offset = 40 + i * 16u;
        s64_code_assets_session_wire[offset] = i + 1u;
        s64_code_assets_session_wire[offset + 1] = S64_MODULE_CACHE_READY;
        s64_code_assets_session_wire[offset + 2] = asset->cache.flags;
        put32(s64_code_assets_session_wire + offset + 4, asset->cache.address);
        put32(s64_code_assets_session_wire + offset + 8, asset->cache.length);
        put32(s64_code_assets_session_wire + offset + 12, asset->integrity);
    }
    put16(s64_code_assets_session_wire + 88, S64_CODE_OVERLAY_ADDRESS);
    put16(s64_code_assets_session_wire + 90, S64_CODE_OVERLAY_CAPACITY);
    put16(s64_code_assets_session_wire + 94,
        s64_code_assets_session_checksum(s64_code_assets_session_wire));
    if (!s64_reu_store((uint8_t)(allocation->address >> 16), (uint16_t)allocation->address,
            s64_code_assets_session_wire, sizeof(s64_code_assets_session_wire))
        || !s64_reu_load((uint8_t)(allocation->address >> 16), (uint16_t)allocation->address,
            s64_code_assets_session_readback, sizeof(s64_code_assets_session_readback))
        || memcmp(s64_code_assets_session_wire, s64_code_assets_session_readback,
            sizeof(s64_code_assets_session_wire))) {
        /* Best effort revocation. The owner must block handoff after failure
         * until readiness can be cleared/readback-verified. */
        s64_code_assets_clear(control);
        return 0;
    }
    return 1;
}
