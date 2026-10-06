#include "launcher/code_assets_dual.h"
#include "launcher/code_assets_scratch.h"

#include "platform/crc32.h"
#include "platform/module_integrity.h"
#include "platform/reu.h"

#include <string.h>

static uint16_t get16(const uint8_t* in)
{ return (uint16_t)(in[0] | ((uint16_t)in[1] << 8)); }

static uint32_t get32(const uint8_t* in)
{ return (uint32_t)get16(in) | ((uint32_t)get16(in + 2) << 16); }

static void put16(uint8_t* out, uint16_t value)
{ out[0] = (uint8_t)value; out[1] = (uint8_t)(value >> 8); }

static void put32(uint8_t* out, uint32_t value)
{ put16(out, (uint16_t)value); put16(out + 2u, (uint16_t)(value >> 16)); }

uint8_t s64_dual_code_assets_count(S64ModuleId module)
{
    if (module == S64_MODULE_EDITOR) return 4u;
    if (module == S64_MODULE_PUBLISHER) return 3u;
    return 0u;
}

static uint8_t session_flag(S64ModuleId module)
{ return module == S64_MODULE_PUBLISHER ? 5u : 3u; }

static uint8_t cache_flag(S64ModuleId module)
{ return module == S64_MODULE_PUBLISHER ? 0x31u : 0x21u; }

static const char* asset_filename(S64ModuleId module, uint8_t index)
{
    if (module == S64_MODULE_PUBLISHER) {
        if (index == 0u) return "S64PCNT.BIN";
        if (index == 1u) return "S64PNET.BIN";
        if (index == 2u) return "S64PRUN.BIN";
        return 0;
    }
    if (module == S64_MODULE_EDITOR) {
        if (index == 0u) return "S64MODEL.BIN";
        if (index == 1u) return "S64FILE.BIN";
        if (index == 2u) return "S64RUN.BIN";
        if (index == 3u) return "S64PREP.BIN";
    }
    return 0;
}

static uint16_t session_size(S64ModuleId module)
{ return (uint16_t)(48u + (uint16_t)s64_dual_code_assets_count(module) * 16u); }

static uint8_t descriptor_valid(
    S64ModuleId module,
    const S64ModuleDescriptor* descriptor
)
{
    return s64_dual_code_assets_count(module) != 0u
        && descriptor != 0
        && descriptor->id == module
        && s64_module_descriptor_loadable(descriptor)
        && descriptor->load_address == 0x0801u
        && descriptor->byte_length - 2UL
            <= S64_CODE_OVERLAY_ADDRESS - 0x0801UL
        && descriptor->startup_address >= descriptor->load_address
        && (uint32_t)descriptor->startup_address
            < descriptor->load_address + descriptor->byte_length - 2UL;
}

static uint8_t assets_valid(
    S64ModuleId module,
    const S64CodeAssets* assets,
    const S64ReuRuntimeControl* control,
    const S64ModuleDescriptor* descriptor,
    uint8_t require_ready
)
{
    uint8_t count = s64_dual_code_assets_count(module);
    uint8_t base = cache_flag(module);
    uint8_t i, j, matches;

    if (!count || !assets || !s64_reu_runtime_valid(control)
        || !descriptor_valid(module, descriptor)
        || descriptor->cache_state != S64_MODULE_CACHE_READY
        || assets->abi_version != S64_REU_RUNTIME_ABI_VERSION
        || assets->app_version != control->app_version
        || assets->app_version != descriptor->app_version
        || assets->editor_length != descriptor->byte_length
        || assets->editor_integrity != descriptor->integrity) return 0u;

    for (i = 0u; i < count; ++i) {
        const S64CodeAsset* asset = &assets->assets[i];
        if (!asset->integrity || !asset->cache.length
            || asset->cache.length > S64_CODE_OVERLAY_CAPACITY
            || asset->cache.owner != S64_REU_OWNER_MODULE
            || asset->cache.flags != (uint8_t)(base + i)
            || (require_ready
                && asset->cache_state != S64_MODULE_CACHE_READY)) return 0u;
        matches = 0u;
        for (j = 0u; j < control->allocation_count; ++j) {
            const S64ReuAllocation* item = &control->allocations[j];
            if (item->owner == S64_REU_OWNER_MODULE
                && item->flags == asset->cache.flags) {
                if (item->address != asset->cache.address
                    || item->length != asset->cache.length) return 0u;
                ++matches;
            }
        }
        if (matches != 1u) return 0u;
    }
    return 1u;
}

static const S64ReuAllocation* session_allocation(
    S64ModuleId module,
    const S64ReuRuntimeControl* control
)
{
    const S64ReuAllocation* found = 0;
    uint8_t flag = session_flag(module);
    uint16_t length = session_size(module);
    uint8_t i;

    if (!s64_dual_code_assets_count(module) || !s64_reu_runtime_valid(control)) return 0;
    for (i = 0u; i < control->allocation_count; ++i) {
        const S64ReuAllocation* item = &control->allocations[i];
        if (item->owner == S64_REU_OWNER_RUNTIME && item->flags == flag) {
            if (found || item->length != length || (item->address & 255UL))
                return 0;
            found = item;
        }
    }
    return found;
}

static uint8_t verify_record(
    const S64ReuAllocation* allocation,
    const uint8_t* expected,
    uint16_t length
)
{
    if (!allocation || !expected || length > S64_CODE_ASSET_SESSION_SIZE
        || allocation->length != length) return 0u;
    return s64_reu_load((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, S64_DUAL_ASSET_READBACK, length)
        && memcmp(expected, S64_DUAL_ASSET_READBACK, length) == 0;
}

static uint16_t session_checksum(S64ModuleId module, const uint8_t* bytes)
{
    uint16_t value = 0x64a5u;
    uint16_t length = session_size(module);
    uint16_t i;
    for (i = 0u; i < length - 2u; ++i) {
        value = (uint16_t)((value << 5) | (value >> 11));
        value ^= bytes[i];
    }
    return value ? value : 1u;
}

uint8_t s64_dual_code_assets_parse(
    S64ModuleId module,
    S64CodeAssets* assets,
    const uint8_t* wire,
    uint16_t length,
    const S64ModuleDescriptor* descriptor
)
{
    S64CodeAssets parsed;
    uint8_t count = s64_dual_code_assets_count(module);
    uint16_t expected = (uint16_t)(28u
        + (uint16_t)s64_dual_code_assets_count(module) * 8u);
    uint16_t crc_offset = (uint16_t)(expected - 4u);
    uint8_t i;

    if (!count || !assets || !wire || length != expected
        || !descriptor_valid(module, descriptor)
        || wire[0] != 0x53u || wire[1] != 0x36u || wire[2] != 0x34u
        || wire[3] != (module == S64_MODULE_PUBLISHER ? 0x50u : 0x4fu)
        || wire[4] != S64_CODE_ASSET_FORMAT_VERSION || wire[5] != count
        || get16(wire + 6u) != S64_REU_RUNTIME_ABI_VERSION
        || get16(wire + 8u) != descriptor->app_version
        || get16(wire + 10u) != S64_CODE_OVERLAY_ADDRESS
        || get16(wire + 12u) != S64_CODE_OVERLAY_CAPACITY
        || get16(wire + 14u)
        || get32(wire + 16u) != descriptor->byte_length
        || get32(wire + 20u) != descriptor->integrity
        || get32(wire + crc_offset)
            != (s64_crc32_update(0xffffffffUL, wire, crc_offset)
                ^ 0xffffffffUL)) return 0u;

    memset(&parsed, 0, sizeof(parsed));
    parsed.abi_version = get16(wire + 6u);
    parsed.app_version = get16(wire + 8u);
    parsed.editor_length = get32(wire + 16u);
    parsed.editor_integrity = get32(wire + 20u);
    for (i = 0u; i < count; ++i) {
        parsed.assets[i].cache.length = get32(wire + 24u + i * 8u);
        parsed.assets[i].integrity = get32(wire + 28u + i * 8u);
        if (!parsed.assets[i].cache.length
            || parsed.assets[i].cache.length > S64_CODE_OVERLAY_CAPACITY
            || !parsed.assets[i].integrity) return 0u;
    }
    *assets = parsed;
    return 1u;
}

uint8_t s64_dual_code_assets_allocate(
    S64ModuleId module,
    S64CodeAssets* assets,
    S64ReuRuntimeControl* control
)
{
    uint8_t count = s64_dual_code_assets_count(module);
    uint8_t base = cache_flag(module);
    uint8_t flag = session_flag(module);
    uint8_t i;

    if (!count || !assets || !s64_reu_runtime_valid(control)
        || assets->app_version != control->app_version || !assets->app_version
        || assets->abi_version != S64_REU_RUNTIME_ABI_VERSION
        || !assets->editor_length || !assets->editor_integrity) return 0u;

    for (i = 0u; i < control->allocation_count; ++i) {
        const S64ReuAllocation* item = &control->allocations[i];
        if ((item->owner == S64_REU_OWNER_RUNTIME && item->flags == flag)
            || (item->owner == S64_REU_OWNER_MODULE
                && item->flags >= base && item->flags < base + count))
            return 0u;
    }

    for (i = 0u; i < count; ++i) {
        S64CodeAsset* asset = &assets->assets[i];
        if (!asset->cache.length
            || asset->cache.length > S64_CODE_OVERLAY_CAPACITY
            || !asset->integrity
            || asset->cache_state != S64_MODULE_CACHE_EMPTY
            || asset->cache.owner != S64_REU_OWNER_NONE
            || !s64_reu_runtime_allocate(control, S64_REU_OWNER_MODULE,
                asset->cache.length, 256u, &asset->cache)) return 0u;
        asset->cache.flags = (uint8_t)(base + i);
        control->allocations[control->allocation_count - 1u].flags =
            asset->cache.flags;
    }
    if (!s64_reu_runtime_allocate(control, S64_REU_OWNER_RUNTIME,
            session_size(module), 256u, 0)) return 0u;
    control->allocations[control->allocation_count - 1u].flags = flag;
    return 1u;
}

uint8_t s64_dual_code_assets_clear(
    S64ModuleId module,
    const S64ReuRuntimeControl* control
)
{
    const S64ReuAllocation* allocation = session_allocation(module, control);
    uint16_t length = session_size(module);
    if (!allocation || length > S64_CODE_ASSET_SESSION_SIZE) return 0u;
    memset(S64_DUAL_ASSET_RECORD, 0, length);
    return s64_reu_store((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, S64_DUAL_ASSET_RECORD, length)
        && verify_record(allocation, S64_DUAL_ASSET_RECORD, length);
}

uint8_t s64_dual_code_assets_preload(
    S64ModuleId module,
    S64Dos* dos,
    const S64ReuRuntimeControl* control,
    const S64ModuleDescriptor* descriptor,
    S64CodeAssets* assets,
    S64SystemState* system
)
{
    uint8_t count = s64_dual_code_assets_count(module);
    S64DosResult result, closed;
    uint32_t size;
    uint8_t i;

    if (!count || !assets) return 0u;
    for (i = 0u; i < count; ++i)
        assets->assets[i].cache_state = S64_MODULE_CACHE_EMPTY;
    if (!dos || !system || !system->captured || system->restored
        || system->mode != S64_SYSTEM_MODE_IO_SAFE
        || !s64_dual_code_assets_clear(module, control)
        || !assets_valid(module, assets, control, descriptor, 0u)
        || s64_dos_change_dir(dos, "/sd/") != S64_DOS_OK) return 0u;

    for (i = 0u; i < count; ++i) {
        S64CodeAsset* asset = &assets->assets[i];
        const char* filename = asset_filename(module, i);
        if (!filename || s64_dos_open_read(dos, filename) != S64_DOS_OK)
            return 0u;
        result = s64_dos_file_size(dos, &size);
        if (result == S64_DOS_OK && size != asset->cache.length)
            result = S64_DOS_TARGET_ERROR;
        if (result == S64_DOS_OK)
            result = s64_dos_load_reu(dos, asset->cache.address,
                asset->cache.length);
        closed = s64_dos_close(dos);
        if (result != S64_DOS_OK || closed != S64_DOS_OK) return 0u;
        asset->cache_state = S64_MODULE_CACHE_LOADED_UNVERIFIED;
        if (!s64_reu_crc32_matches(asset->cache.address, asset->cache.length,
                asset->integrity, system)) {
            (void)s64_system_state_set_mode(system, S64_SYSTEM_MODE_IO_SAFE);
            return 0u;
        }
    }
    for (i = 0u; i < count; ++i)
        assets->assets[i].cache_state = S64_MODULE_CACHE_READY;
    return 1u;
}

uint8_t s64_dual_code_assets_store_verified(
    S64ModuleId module,
    const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal,
    const S64ModuleDescriptor* descriptor,
    const S64CodeAssets* assets
)
{
    const S64ReuAllocation* allocation = session_allocation(module, control);
    uint8_t count = s64_dual_code_assets_count(module);
    uint16_t length = session_size(module);
    uint16_t trailer = (uint16_t)(40u + (uint16_t)count * 16u);
    uint8_t i, offset;

    if (!allocation || !journal || journal->state != S64_SESSION_STATE_ACTIVE
        || !journal->sequence || journal->session_id != control->session_id
        || journal->package_version != control->app_version
        || !assets_valid(module, assets, control, descriptor, 1u)
        || !s64_dual_code_assets_clear(module, control)) return 0u;

    memset(S64_DUAL_ASSET_RECORD, 0, length);
    S64_DUAL_ASSET_RECORD[0] = 0x53u;
    S64_DUAL_ASSET_RECORD[1] = 0x36u;
    S64_DUAL_ASSET_RECORD[2] = 0x34u;
    S64_DUAL_ASSET_RECORD[3] =
        module == S64_MODULE_PUBLISHER ? 0x51u : 0x43u;
    S64_DUAL_ASSET_RECORD[4] = S64_CODE_ASSET_FORMAT_VERSION;
    S64_DUAL_ASSET_RECORD[5] = count;
    put16(S64_DUAL_ASSET_RECORD + 6u, S64_REU_RUNTIME_ABI_VERSION);
    if (!s64_session_journal_encode(journal, S64_DUAL_ASSET_RECORD + 8u))
        return 0u;
    put32(S64_DUAL_ASSET_RECORD + 32u, assets->editor_length);
    put32(S64_DUAL_ASSET_RECORD + 36u, assets->editor_integrity);

    for (i = 0u; i < count; ++i) {
        const S64CodeAsset* asset = &assets->assets[i];
        offset = (uint8_t)(40u + i * 16u);
        S64_DUAL_ASSET_RECORD[offset] = i + 1u;
        S64_DUAL_ASSET_RECORD[offset + 1u] = S64_MODULE_CACHE_READY;
        S64_DUAL_ASSET_RECORD[offset + 2u] = asset->cache.flags;
        put32(S64_DUAL_ASSET_RECORD + offset + 4u, asset->cache.address);
        put32(S64_DUAL_ASSET_RECORD + offset + 8u, asset->cache.length);
        put32(S64_DUAL_ASSET_RECORD + offset + 12u, asset->integrity);
    }

    put16(S64_DUAL_ASSET_RECORD + trailer, S64_CODE_OVERLAY_ADDRESS);
    put16(S64_DUAL_ASSET_RECORD + trailer + 2u, S64_CODE_OVERLAY_CAPACITY);
    put16(S64_DUAL_ASSET_RECORD + trailer + 6u,
        session_checksum(module, S64_DUAL_ASSET_RECORD));

    if (!s64_reu_store((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, S64_DUAL_ASSET_RECORD, length)
        || !verify_record(allocation, S64_DUAL_ASSET_RECORD, length)) {
        (void)s64_dual_code_assets_clear(module, control);
        return 0u;
    }
    return 1u;
}
