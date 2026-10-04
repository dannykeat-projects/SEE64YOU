#include "platform/code_assets_internal.h"
#include "platform/reu.h"
#include <string.h>

static S64CodeAssets decoded;
static S64ReuAllocation handles[S64_CODE_OVERLAY_COUNT];

static uint16_t get16(const uint8_t* in)
{ return (uint16_t)(in[0] | ((uint16_t)in[1] << 8)); }
static uint32_t get32(const uint8_t* in)
{ return (uint32_t)get16(in) | ((uint32_t)get16(in + 2) << 16); }

uint8_t s64_code_assets_load(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const S64ModuleDescriptor* editor, S64CodeAssets* assets)
{
    const S64ReuAllocation* allocation = s64_code_assets_session_allocation(control);
    uint8_t i, offset;
    if (!allocation || !assets || !s64_code_assets_active_session(control, journal)
        || !s64_reu_load((uint8_t)(allocation->address >> 16), (uint16_t)allocation->address,
            s64_code_assets_session_wire, sizeof(s64_code_assets_session_wire))) return 0;
    if (memcmp(s64_code_assets_session_wire, S64_CODE_ASSET_SESSION_MAGIC, 4)
        || s64_code_assets_session_wire[4] != 1
        || s64_code_assets_session_wire[5] != S64_CODE_OVERLAY_COUNT
        || get16(s64_code_assets_session_wire + 6) != S64_REU_RUNTIME_ABI_VERSION
        || memcmp(s64_code_assets_session_wire + 8, s64_code_assets_session_journal_wire,
            sizeof(s64_code_assets_session_journal_wire))
        || get16(s64_code_assets_session_wire + 88) != S64_CODE_OVERLAY_ADDRESS
        || get16(s64_code_assets_session_wire + 90) != S64_CODE_OVERLAY_CAPACITY
        || get16(s64_code_assets_session_wire + 92)
        || get16(s64_code_assets_session_wire + 94)
            != s64_code_assets_session_checksum(s64_code_assets_session_wire)) return 0;
    memset(&decoded, 0, sizeof(decoded));
    decoded.abi_version = get16(s64_code_assets_session_wire + 6);
    decoded.app_version = control->app_version;
    decoded.editor_length = get32(s64_code_assets_session_wire + 32);
    decoded.editor_integrity = get32(s64_code_assets_session_wire + 36);
    for (i = 0; i < S64_CODE_OVERLAY_COUNT; ++i) {
        S64CodeAsset* asset = &decoded.assets[i];
        offset = 40 + i * 16u;
        if (s64_code_assets_session_wire[offset] != i + 1u
            || s64_code_assets_session_wire[offset + 1] != S64_MODULE_CACHE_READY
            || s64_code_assets_session_wire[offset + 3]) return 0;
        asset->cache.owner = S64_REU_OWNER_MODULE;
        asset->cache.flags = s64_code_assets_session_wire[offset + 2];
        asset->cache_state = s64_code_assets_session_wire[offset + 1];
        asset->cache.address = get32(s64_code_assets_session_wire + offset + 4);
        asset->cache.length = get32(s64_code_assets_session_wire + offset + 8);
        asset->integrity = get32(s64_code_assets_session_wire + offset + 12);
    }
    if (!s64_code_assets_valid(&decoded, control, editor, 1)) return 0;
    *assets = decoded;
    return 1;
}

uint8_t s64_code_assets_bind(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const S64ModuleDescriptor* editor,
    uint8_t trusted_arrival, void* slot, uint16_t capacity, S64CodeOverlayReader reader)
{
    uint8_t i;
    s64_code_overlay_configure(0, 0, 0, 0, 0);
    if (!trusted_arrival || !s64_code_assets_load(control, journal, editor, &decoded)) return 0;
    for (i = 0; i < S64_CODE_OVERLAY_COUNT; ++i) handles[i] = decoded.assets[i].cache;
    return s64_code_overlay_configure(control, handles, slot, capacity, reader);
}
