#include "platform/code_assets_internal.h"
#include "platform/reu.h"
#include <string.h>

static uint8_t wire[S64_CODE_ASSET_SESSION_SIZE];
static uint8_t readback[S64_CODE_ASSET_SESSION_SIZE];
static uint8_t journal_wire[S64_SESSION_JOURNAL_WIRE_SIZE];
static S64CodeAssets decoded;
static S64ReuAllocation handles[S64_CODE_OVERLAY_COUNT];

static void put16(uint8_t* out, uint16_t value)
{ out[0] = (uint8_t)value; out[1] = (uint8_t)(value >> 8); }
static void put32(uint8_t* out, uint32_t value)
{ put16(out, (uint16_t)value); put16(out + 2, (uint16_t)(value >> 16)); }
static uint16_t get16(const uint8_t* in)
{ return (uint16_t)(in[0] | ((uint16_t)in[1] << 8)); }
static uint32_t get32(const uint8_t* in)
{ return (uint32_t)get16(in) | ((uint32_t)get16(in + 2) << 16); }
static uint16_t checksum(void)
{
    uint16_t value = 0x64a5u;
    uint8_t i;
    for (i = 0; i < S64_CODE_ASSET_SESSION_SIZE - 2; ++i) {
        value = (uint16_t)((value << 5) | (value >> 11));
        value ^= wire[i];
    }
    return value ? value : 1;
}

const S64ReuAllocation* s64_code_assets_session_allocation(const S64ReuRuntimeControl* control)
{
    const S64ReuAllocation* found = 0;
    uint8_t i;
    if (!s64_reu_runtime_valid(control)) return 0;
    for (i = 0; i < control->allocation_count; ++i) {
        const S64ReuAllocation* item = &control->allocations[i];
        if (item->owner == S64_REU_OWNER_RUNTIME && item->flags == S64_CODE_ASSET_SESSION_FLAG) {
            if (found || item->length != S64_CODE_ASSET_SESSION_SIZE
                || (item->address & 255UL)) return 0;
            found = item;
        }
    }
    return found;
}

uint8_t s64_code_assets_clear(const S64ReuRuntimeControl* control)
{
    const S64ReuAllocation* allocation = s64_code_assets_session_allocation(control);
    if (!allocation) return 0;
    memset(wire, 0, sizeof(wire));
    return s64_reu_store((uint8_t)(allocation->address >> 16), (uint16_t)allocation->address,
            wire, sizeof(wire))
        && s64_reu_load((uint8_t)(allocation->address >> 16), (uint16_t)allocation->address,
            readback, sizeof(readback))
        && memcmp(wire, readback, sizeof(wire)) == 0;
}

static uint8_t active_session(const S64ReuRuntimeControl* control, const S64SessionJournal* journal)
{
    return journal != 0 && journal->state == S64_SESSION_STATE_ACTIVE && journal->sequence
        && journal->session_id == control->session_id && journal->package_version == control->app_version
        && s64_session_journal_encode(journal, journal_wire);
}

uint8_t s64_code_assets_store_verified(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const S64ModuleDescriptor* editor, const S64CodeAssets* assets)
{
    const S64ReuAllocation* allocation = s64_code_assets_session_allocation(control);
    uint8_t i, offset;
    if (!allocation || !s64_code_assets_clear(control) || !active_session(control, journal)
        || !s64_code_assets_valid(assets, control, editor, 1)) return 0;
    memcpy(wire, "S64C", 4);
    wire[4] = 1; wire[5] = S64_CODE_OVERLAY_COUNT;
    put16(wire + 6, S64_REU_RUNTIME_ABI_VERSION);
    memcpy(wire + 8, journal_wire, sizeof(journal_wire));
    put32(wire + 32, assets->editor_length);
    put32(wire + 36, assets->editor_integrity);
    for (i = 0; i < S64_CODE_OVERLAY_COUNT; ++i) {
        const S64CodeAsset* asset = &assets->assets[i];
        offset = 40 + i * 16u;
        wire[offset] = i + 1u;
        wire[offset + 1] = S64_MODULE_CACHE_READY;
        wire[offset + 2] = asset->cache.flags;
        put32(wire + offset + 4, asset->cache.address);
        put32(wire + offset + 8, asset->cache.length);
        put32(wire + offset + 12, asset->integrity);
    }
    put16(wire + 88, S64_CODE_OVERLAY_ADDRESS);
    put16(wire + 90, S64_CODE_OVERLAY_CAPACITY);
    put16(wire + 94, checksum());
    if (!s64_reu_store((uint8_t)(allocation->address >> 16), (uint16_t)allocation->address,
            wire, sizeof(wire))
        || !s64_reu_load((uint8_t)(allocation->address >> 16), (uint16_t)allocation->address,
            readback, sizeof(readback)) || memcmp(wire, readback, sizeof(wire))) {
        /* Best effort revocation. The owner must block handoff after failure
         * until readiness can be cleared/readback-verified. */
        s64_code_assets_clear(control);
        return 0;
    }
    return 1;
}

uint8_t s64_code_assets_load(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const S64ModuleDescriptor* editor, S64CodeAssets* assets)
{
    const S64ReuAllocation* allocation = s64_code_assets_session_allocation(control);
    uint8_t i, offset;
    if (!allocation || !assets || !active_session(control, journal)
        || !s64_reu_load((uint8_t)(allocation->address >> 16), (uint16_t)allocation->address,
            wire, sizeof(wire))) return 0;
    if (memcmp(wire, "S64C", 4) || wire[4] != 1 || wire[5] != S64_CODE_OVERLAY_COUNT
        || get16(wire + 6) != S64_REU_RUNTIME_ABI_VERSION
        || memcmp(wire + 8, journal_wire, sizeof(journal_wire))
        || get16(wire + 88) != S64_CODE_OVERLAY_ADDRESS
        || get16(wire + 90) != S64_CODE_OVERLAY_CAPACITY
        || get16(wire + 92) || get16(wire + 94) != checksum()) return 0;
    memset(&decoded, 0, sizeof(decoded));
    decoded.abi_version = get16(wire + 6);
    decoded.app_version = control->app_version;
    decoded.editor_length = get32(wire + 32);
    decoded.editor_integrity = get32(wire + 36);
    for (i = 0; i < S64_CODE_OVERLAY_COUNT; ++i) {
        S64CodeAsset* asset = &decoded.assets[i];
        offset = 40 + i * 16u;
        if (wire[offset] != i + 1u || wire[offset + 1] != S64_MODULE_CACHE_READY
            || wire[offset + 3]) return 0;
        asset->cache.owner = S64_REU_OWNER_MODULE;
        asset->cache.flags = wire[offset + 2];
        asset->cache_state = wire[offset + 1];
        asset->cache.address = get32(wire + offset + 4);
        asset->cache.length = get32(wire + offset + 8);
        asset->integrity = get32(wire + offset + 12);
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
