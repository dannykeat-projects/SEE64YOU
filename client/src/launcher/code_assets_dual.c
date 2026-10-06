#include "launcher/code_assets_dual.h"

#include "platform/crc32.h"
#include "platform/module_integrity.h"
#include "platform/reu.h"

#include <string.h>

static const S64LauncherCodeAssetNamespace editor_namespace = {
    S64_MODULE_EDITOR, 0x4fu, 3u, 0x21u, 4u,
    "S64OVL.MNF", "S64C",
    {"S64MODEL.BIN", "S64FILE.BIN", "S64RUN.BIN", "S64PREP.BIN"}
};

static const S64LauncherCodeAssetNamespace publisher_namespace = {
    S64_MODULE_PUBLISHER, 0x50u, 5u, 0x31u, 3u,
    "S64POVL.MNF", "S64Q",
    {"S64PCNT.BIN", "S64PNET.BIN", "S64PRUN.BIN", 0}
};

static uint16_t get16(const uint8_t* in)
{ return (uint16_t)(in[0] | ((uint16_t)in[1] << 8)); }

static uint32_t get32(const uint8_t* in)
{ return (uint32_t)get16(in) | ((uint32_t)get16(in + 2) << 16); }

static void put16(uint8_t* out, uint16_t value)
{ out[0] = (uint8_t)value; out[1] = (uint8_t)(value >> 8); }

static void put32(uint8_t* out, uint32_t value)
{ put16(out, (uint16_t)value); put16(out + 2u, (uint16_t)(value >> 16)); }

static uint16_t manifest_size(const S64LauncherCodeAssetNamespace* namespace)
{
    return namespace
        ? (uint16_t)(28u + (uint16_t)namespace->overlay_count * 8u)
        : 0u;
}

static uint16_t session_size(const S64LauncherCodeAssetNamespace* namespace)
{
    return namespace
        ? (uint16_t)(48u + (uint16_t)namespace->overlay_count * 16u)
        : 0u;
}

static uint16_t session_trailer(const S64LauncherCodeAssetNamespace* namespace)
{
    return namespace
        ? (uint16_t)(40u + (uint16_t)namespace->overlay_count * 16u)
        : 0u;
}

static uint8_t namespace_valid(const S64LauncherCodeAssetNamespace* namespace)
{
    return namespace != 0
        && namespace->overlay_count != 0u
        && namespace->overlay_count <= S64_CODE_OVERLAY_COUNT
        && namespace->manifest_filename != 0
        && namespace->session_magic != 0;
}

static uint8_t module_valid(
    const S64LauncherCodeAssetNamespace* namespace,
    const S64ModuleDescriptor* module
)
{
    return namespace_valid(namespace)
        && module != 0
        && module->id == namespace->module
        && s64_module_descriptor_loadable(module)
        && module->load_address == 0x0801u
        && module->byte_length - 2UL <= S64_CODE_OVERLAY_ADDRESS - 0x0801UL
        && module->startup_address >= module->load_address
        && (uint32_t)module->startup_address
            < module->load_address + module->byte_length - 2UL;
}

static uint8_t assets_valid(
    const S64LauncherCodeAssetNamespace* namespace,
    const S64CodeAssets* assets,
    const S64ReuRuntimeControl* control,
    const S64ModuleDescriptor* module,
    uint8_t require_ready
)
{
    uint8_t i;
    uint8_t j;
    uint8_t matches;

    if (!namespace_valid(namespace)
        || !assets
        || !s64_reu_runtime_valid(control)
        || !module_valid(namespace, module)
        || module->cache_state != S64_MODULE_CACHE_READY
        || assets->abi_version != S64_REU_RUNTIME_ABI_VERSION
        || assets->app_version != control->app_version
        || assets->app_version != module->app_version
        || assets->editor_length != module->byte_length
        || assets->editor_integrity != module->integrity) {
        return 0u;
    }

    for (i = 0u; i < namespace->overlay_count; ++i) {
        const S64CodeAsset* asset = &assets->assets[i];

        if (!asset->integrity
            || !asset->cache.length
            || asset->cache.length > S64_CODE_OVERLAY_CAPACITY
            || asset->cache.owner != S64_REU_OWNER_MODULE
            || asset->cache.flags != (uint8_t)(namespace->cache_flag + i)
            || (require_ready
                && asset->cache_state != S64_MODULE_CACHE_READY)) {
            return 0u;
        }

        matches = 0u;
        for (j = 0u; j < control->allocation_count; ++j) {
            const S64ReuAllocation* item = &control->allocations[j];
            if (item->owner == S64_REU_OWNER_MODULE
                && item->flags == asset->cache.flags) {
                if (item->address != asset->cache.address
                    || item->length != asset->cache.length) {
                    return 0u;
                }
                ++matches;
            }
        }
        if (matches != 1u) return 0u;
    }

    return 1u;
}

static const S64ReuAllocation* session_allocation(
    const S64LauncherCodeAssetNamespace* namespace,
    const S64ReuRuntimeControl* control
)
{
    const S64ReuAllocation* found = 0;
    uint16_t size;
    uint8_t i;

    if (!namespace_valid(namespace) || !s64_reu_runtime_valid(control)) return 0;
    size = session_size(namespace);

    for (i = 0u; i < control->allocation_count; ++i) {
        const S64ReuAllocation* item = &control->allocations[i];
        if (item->owner == S64_REU_OWNER_RUNTIME
            && item->flags == namespace->session_flag) {
            if (found || item->length != size || (item->address & 255UL)) return 0;
            found = item;
        }
    }

    return found;
}

static uint16_t session_checksum(
    const S64LauncherCodeAssetNamespace* namespace,
    const uint8_t* bytes
)
{
    uint16_t value = 0x64a5u;
    uint16_t size = session_size(namespace);
    uint16_t i;

    if (!namespace_valid(namespace) || !bytes || size < 2u) return 0u;
    for (i = 0u; i < size - 2u; ++i) {
        value = (uint16_t)((value << 5) | (value >> 11));
        value ^= bytes[i];
    }
    return value ? value : 1u;
}

static uint8_t verify_record(
    const S64ReuAllocation* allocation,
    const uint8_t* expected,
    uint16_t length
)
{
    uint8_t readback[S64_CODE_ASSET_SESSION_SIZE];

    if (!allocation || !expected || length > sizeof(readback)
        || allocation->length != length) return 0u;

    return s64_reu_load(
            (uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address,
            readback,
            length
        )
        && memcmp(expected, readback, length) == 0;
}

const S64LauncherCodeAssetNamespace* s64_dual_code_assets_namespace(
    S64ModuleId module
)
{
    if (module == S64_MODULE_EDITOR) return &editor_namespace;
    if (module == S64_MODULE_PUBLISHER) return &publisher_namespace;
    return 0;
}

uint8_t s64_dual_code_assets_parse(
    const S64LauncherCodeAssetNamespace* namespace,
    S64CodeAssets* assets,
    const uint8_t* wire,
    uint16_t length,
    const S64ModuleDescriptor* module
)
{
    S64CodeAssets parsed;
    uint16_t size;
    uint16_t crc_offset;
    uint8_t i;

    if (!namespace_valid(namespace)) return 0u;
    size = manifest_size(namespace);
    crc_offset = (uint16_t)(size - 4u);

    if (!assets || !wire || length != size
        || !module_valid(namespace, module)
        || wire[0] != 0x53u
        || wire[1] != 0x36u
        || wire[2] != 0x34u
        || wire[3] != namespace->magic_last
        || wire[4] != S64_CODE_ASSET_FORMAT_VERSION
        || wire[5] != namespace->overlay_count
        || get16(wire + 6u) != S64_REU_RUNTIME_ABI_VERSION
        || get16(wire + 8u) != module->app_version
        || get16(wire + 10u) != S64_CODE_OVERLAY_ADDRESS
        || get16(wire + 12u) != S64_CODE_OVERLAY_CAPACITY
        || get16(wire + 14u) != 0u
        || get32(wire + 16u) != module->byte_length
        || get32(wire + 20u) != module->integrity
        || get32(wire + crc_offset)
            != (s64_crc32_update(0xffffffffUL, wire, crc_offset)
                ^ 0xffffffffUL)) {
        return 0u;
    }

    memset(&parsed, 0, sizeof(parsed));
    parsed.abi_version = get16(wire + 6u);
    parsed.app_version = get16(wire + 8u);
    parsed.editor_length = get32(wire + 16u);
    parsed.editor_integrity = get32(wire + 20u);

    for (i = 0u; i < namespace->overlay_count; ++i) {
        parsed.assets[i].cache.length = get32(wire + 24u + i * 8u);
        parsed.assets[i].integrity = get32(wire + 28u + i * 8u);
        if (!parsed.assets[i].cache.length
            || parsed.assets[i].cache.length > S64_CODE_OVERLAY_CAPACITY
            || !parsed.assets[i].integrity) {
            return 0u;
        }
    }

    *assets = parsed;
    return 1u;
}

uint8_t s64_dual_code_assets_allocate(
    const S64LauncherCodeAssetNamespace* namespace,
    S64CodeAssets* assets,
    S64ReuRuntimeControl* control
)
{
    S64ReuRuntimeControl planned;
    S64CodeAssets planned_assets;
    uint8_t i;

    if (!namespace_valid(namespace)
        || !assets
        || !s64_reu_runtime_valid(control)
        || assets->app_version != control->app_version
        || !assets->app_version
        || assets->abi_version != S64_REU_RUNTIME_ABI_VERSION
        || !assets->editor_length
        || !assets->editor_integrity) {
        return 0u;
    }

    for (i = 0u; i < control->allocation_count; ++i) {
        const S64ReuAllocation* item = &control->allocations[i];
        if ((item->owner == S64_REU_OWNER_RUNTIME
                && item->flags == namespace->session_flag)
            || (item->owner == S64_REU_OWNER_MODULE
                && item->flags >= namespace->cache_flag
                && item->flags
                    < namespace->cache_flag + namespace->overlay_count)) {
            return 0u;
        }
    }

    planned = *control;
    planned_assets = *assets;

    for (i = 0u; i < namespace->overlay_count; ++i) {
        S64CodeAsset* asset = &planned_assets.assets[i];

        if (!asset->cache.length
            || asset->cache.length > S64_CODE_OVERLAY_CAPACITY
            || !asset->integrity
            || asset->cache_state != S64_MODULE_CACHE_EMPTY
            || asset->cache.owner != S64_REU_OWNER_NONE
            || !s64_reu_runtime_allocate(
                &planned,
                S64_REU_OWNER_MODULE,
                asset->cache.length,
                256u,
                &asset->cache
            )) {
            return 0u;
        }

        asset->cache.flags = (uint8_t)(namespace->cache_flag + i);
        planned.allocations[planned.allocation_count - 1u].flags =
            asset->cache.flags;
    }

    if (!s64_reu_runtime_allocate(
            &planned,
            S64_REU_OWNER_RUNTIME,
            session_size(namespace),
            256u,
            0
        )) {
        return 0u;
    }

    planned.allocations[planned.allocation_count - 1u].flags =
        namespace->session_flag;

    *control = planned;
    *assets = planned_assets;
    return 1u;
}

uint8_t s64_dual_code_assets_clear(
    const S64LauncherCodeAssetNamespace* namespace,
    const S64ReuRuntimeControl* control
)
{
    const S64ReuAllocation* allocation;
    uint8_t zero[S64_CODE_ASSET_SESSION_SIZE];
    uint16_t size;

    allocation = session_allocation(namespace, control);
    size = session_size(namespace);
    if (!allocation || !size || size > sizeof(zero)) return 0u;

    memset(zero, 0, size);
    return s64_reu_store(
            (uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address,
            zero,
            size
        )
        && verify_record(allocation, zero, size);
}

uint8_t s64_dual_code_assets_preload(
    const S64LauncherCodeAssetNamespace* namespace,
    S64Dos* dos,
    const S64ReuRuntimeControl* control,
    const S64ModuleDescriptor* module,
    S64CodeAssets* assets,
    S64SystemState* system
)
{
    S64DosResult result;
    S64DosResult closed;
    uint32_t size;
    uint8_t i;

    if (!namespace_valid(namespace) || !assets) return 0u;
    for (i = 0u; i < namespace->overlay_count; ++i)
        assets->assets[i].cache_state = S64_MODULE_CACHE_EMPTY;

    if (!dos
        || !system
        || !system->captured
        || system->restored
        || system->mode != S64_SYSTEM_MODE_IO_SAFE
        || !s64_dual_code_assets_clear(namespace, control)
        || !assets_valid(namespace, assets, control, module, 0u)
        || s64_dos_change_dir(dos, "/sd/") != S64_DOS_OK) {
        return 0u;
    }

    for (i = 0u; i < namespace->overlay_count; ++i) {
        S64CodeAsset* asset = &assets->assets[i];

        if (!namespace->filenames[i]
            || s64_dos_open_read(dos, namespace->filenames[i])
                != S64_DOS_OK) {
            return 0u;
        }

        result = s64_dos_file_size(dos, &size);
        if (result == S64_DOS_OK && size != asset->cache.length)
            result = S64_DOS_TARGET_ERROR;
        if (result == S64_DOS_OK)
            result = s64_dos_load_reu(
                dos,
                asset->cache.address,
                asset->cache.length
            );

        closed = s64_dos_close(dos);
        if (result != S64_DOS_OK || closed != S64_DOS_OK) return 0u;

        asset->cache_state = S64_MODULE_CACHE_LOADED_UNVERIFIED;
        if (!s64_reu_crc32_matches(
                asset->cache.address,
                asset->cache.length,
                asset->integrity,
                system
            )) {
            (void)s64_system_state_set_mode(system, S64_SYSTEM_MODE_IO_SAFE);
            return 0u;
        }
    }

    for (i = 0u; i < namespace->overlay_count; ++i)
        assets->assets[i].cache_state = S64_MODULE_CACHE_READY;

    return 1u;
}

uint8_t s64_dual_code_assets_store_verified(
    const S64LauncherCodeAssetNamespace* namespace,
    const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal,
    const S64ModuleDescriptor* module,
    const S64CodeAssets* assets
)
{
    const S64ReuAllocation* allocation;
    uint8_t record[S64_CODE_ASSET_SESSION_SIZE];
    uint8_t journal_wire[S64_SESSION_JOURNAL_WIRE_SIZE];
    uint16_t size;
    uint16_t trailer;
    uint8_t i;
    uint8_t offset;

    allocation = session_allocation(namespace, control);
    size = session_size(namespace);
    trailer = session_trailer(namespace);

    if (!allocation
        || size > sizeof(record)
        || !journal
        || journal->state != S64_SESSION_STATE_ACTIVE
        || !journal->sequence
        || journal->session_id != control->session_id
        || journal->package_version != control->app_version
        || !assets_valid(namespace, assets, control, module, 1u)
        || !s64_session_journal_encode(journal, journal_wire)
        || !s64_dual_code_assets_clear(namespace, control)) {
        return 0u;
    }

    memset(record, 0, size);
    memcpy(record, namespace->session_magic, 4u);
    record[4] = S64_CODE_ASSET_FORMAT_VERSION;
    record[5] = namespace->overlay_count;
    put16(record + 6u, S64_REU_RUNTIME_ABI_VERSION);
    memcpy(record + 8u, journal_wire, sizeof(journal_wire));
    put32(record + 32u, assets->editor_length);
    put32(record + 36u, assets->editor_integrity);

    for (i = 0u; i < namespace->overlay_count; ++i) {
        const S64CodeAsset* asset = &assets->assets[i];
        offset = (uint8_t)(40u + i * 16u);
        record[offset] = i + 1u;
        record[offset + 1u] = S64_MODULE_CACHE_READY;
        record[offset + 2u] = asset->cache.flags;
        put32(record + offset + 4u, asset->cache.address);
        put32(record + offset + 8u, asset->cache.length);
        put32(record + offset + 12u, asset->integrity);
    }

    put16(record + trailer, S64_CODE_OVERLAY_ADDRESS);
    put16(record + trailer + 2u, S64_CODE_OVERLAY_CAPACITY);
    put16(record + trailer + 6u, session_checksum(namespace, record));

    if (!s64_reu_store(
            (uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address,
            record,
            size
        )
        || !verify_record(allocation, record, size)) {
        (void)s64_dual_code_assets_clear(namespace, control);
        return 0u;
    }

    return 1u;
}
