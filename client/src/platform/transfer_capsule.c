#include "platform/transfer_capsule.h"
#include "platform/reu.h"

#include <string.h>

#define CAPSULE_JOURNAL_OFFSET 8u
#define CAPSULE_MODULE_OFFSET 32u
#define CAPSULE_CHECKSUM_OFFSET 62u

static uint8_t wire[S64_TRANSFER_CAPSULE_WIRE_SIZE];
static uint8_t readback[S64_TRANSFER_CAPSULE_WIRE_SIZE];
static uint8_t journal_wire[S64_SESSION_JOURNAL_WIRE_SIZE];

static void put16(uint8_t* out, uint16_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
}

static void put32(uint8_t* out, uint32_t value)
{
    put16(out, (uint16_t)value);
    put16(out + 2u, (uint16_t)(value >> 16));
}

static uint16_t get16(const uint8_t* in)
{
    return (uint16_t)(in[0] | ((uint16_t)in[1] << 8));
}

static uint32_t get32(const uint8_t* in)
{
    return (uint32_t)get16(in) | ((uint32_t)get16(in + 2u) << 16);
}

static uint16_t checksum(const uint8_t* bytes)
{
    uint16_t value = 0x6454u;
    uint8_t i;

    for (i = 0u; i < CAPSULE_CHECKSUM_OFFSET; ++i) {
        value = (uint16_t)((value << 3) | (value >> 13));
        value ^= bytes[i];
    }
    return value == 0u ? 1u : value;
}

static const S64ReuAllocation* capsule_allocation(
    const S64ReuRuntimeControl* control
)
{
    const S64ReuAllocation* found = 0;
    uint8_t i;

    if (!s64_reu_runtime_valid(control)) return 0;
    for (i = 0u; i < control->allocation_count; ++i) {
        const S64ReuAllocation* allocation = &control->allocations[i];
        if (allocation->owner == S64_REU_OWNER_RUNTIME
            && allocation->flags == S64_TRANSFER_CAPSULE_ALLOCATION_FLAG) {
            if (found != 0
                || allocation->length != S64_TRANSFER_CAPSULE_WIRE_SIZE
                || (allocation->address & 255UL) != 0UL) {
                return 0;
            }
            found = allocation;
        }
    }
    return found;
}

static const S64ModuleDescriptor* destination_module(
    const S64ModuleManifest* manifest,
    S64ModuleId destination
)
{
    const S64ModuleDescriptor* module;

    if (manifest == 0
        || destination < S64_MODULE_EDITOR
        || destination > S64_MODULE_SETTINGS) {
        return 0;
    }
    module = &manifest->modules[(uint8_t)destination - 1u];
    return module->id == (uint8_t)destination
        && module->cache_state == S64_MODULE_CACHE_READY
        && s64_module_descriptor_loadable(module)
        ? module : 0;
}

uint8_t s64_transfer_capsule_reserve(S64ReuRuntimeControl* control)
{
    S64ReuAllocation allocation;
    uint8_t i;

    if (!s64_reu_runtime_valid(control)) return 0u;
    for (i = 0u; i < control->allocation_count; ++i) {
        if (control->allocations[i].owner == S64_REU_OWNER_RUNTIME
            && control->allocations[i].flags
                == S64_TRANSFER_CAPSULE_ALLOCATION_FLAG) {
            return 0u;
        }
    }
    if (!s64_reu_runtime_allocate(control, S64_REU_OWNER_RUNTIME,
            S64_TRANSFER_CAPSULE_WIRE_SIZE, 256u, &allocation)) {
        return 0u;
    }
    control->allocations[control->allocation_count - 1u].flags =
        S64_TRANSFER_CAPSULE_ALLOCATION_FLAG;
    return 1u;
}

uint8_t s64_transfer_capsule_clear(const S64ReuRuntimeControl* control)
{
    const S64ReuAllocation* allocation = capsule_allocation(control);

    if (allocation == 0) return 0u;
    memset(wire, 0, sizeof(wire));
    return s64_reu_store((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, wire, sizeof(wire))
        && s64_reu_load((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, readback, sizeof(readback))
        && memcmp(wire, readback, sizeof(wire)) == 0;
}

uint8_t s64_transfer_capsule_store(
    const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal,
    const S64ModuleManifest* manifest,
    S64ModuleId destination
)
{
    const S64ReuAllocation* allocation = capsule_allocation(control);
    const S64ModuleDescriptor* module =
        destination_module(manifest, destination);

    if (allocation == 0 || journal == 0 || module == 0
        || journal->state != S64_SESSION_STATE_ACTIVE
        || journal->sequence == 0UL
        || journal->session_id != control->session_id
        || journal->package_version != control->app_version
        || !s64_session_journal_encode(journal, journal_wire)) {
        return 0u;
    }

    memset(wire, 0, sizeof(wire));
    wire[0] = 'S';
    wire[1] = '6';
    wire[2] = '4';
    wire[3] = 'T';
    wire[4] = S64_TRANSFER_CAPSULE_FORMAT_VERSION;
    wire[5] = S64_REU_RUNTIME_ABI_VERSION;
    wire[6] = (uint8_t)destination;
    wire[7] = 0u;
    memcpy(wire + CAPSULE_JOURNAL_OFFSET, journal_wire,
        sizeof(journal_wire));

    wire[CAPSULE_MODULE_OFFSET] = module->id;
    wire[CAPSULE_MODULE_OFFSET + 1u] = module->cache_state;
    put16(wire + CAPSULE_MODULE_OFFSET + 2u, module->abi_version);
    put16(wire + CAPSULE_MODULE_OFFSET + 4u, module->app_version);
    put16(wire + CAPSULE_MODULE_OFFSET + 6u, module->load_address);
    put16(wire + CAPSULE_MODULE_OFFSET + 8u, module->startup_address);
    put32(wire + CAPSULE_MODULE_OFFSET + 10u, module->byte_length);
    put32(wire + CAPSULE_MODULE_OFFSET + 14u, module->integrity);
    put32(wire + CAPSULE_MODULE_OFFSET + 18u, module->cache_address);
    put32(wire + CAPSULE_MODULE_OFFSET + 22u, module->cache_capacity);
    put16(wire + CAPSULE_CHECKSUM_OFFSET, checksum(wire));

    return s64_reu_store((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, wire, sizeof(wire))
        && s64_reu_load((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, readback, sizeof(readback))
        && memcmp(wire, readback, sizeof(wire)) == 0;
}

uint8_t s64_transfer_capsule_load(
    const S64ReuRuntimeControl* control,
    S64TransferCapsule* capsule
)
{
    const S64ReuAllocation* allocation = capsule_allocation(control);
    S64TransferCapsule decoded;
    uint8_t i;

    if (allocation == 0 || capsule == 0
        || !s64_reu_load((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, wire, sizeof(wire))) {
        return 0u;
    }

    if (wire[0] != 'S' || wire[1] != '6' || wire[2] != '4'
        || wire[3] != 'T'
        || wire[4] != S64_TRANSFER_CAPSULE_FORMAT_VERSION
        || wire[5] != S64_REU_RUNTIME_ABI_VERSION
        || wire[6] < S64_MODULE_EDITOR || wire[6] > S64_MODULE_SETTINGS
        || wire[7] != 0u
        || get16(wire + CAPSULE_CHECKSUM_OFFSET) != checksum(wire)) {
        return 0u;
    }

    for (i = CAPSULE_MODULE_OFFSET + 26u;
            i < CAPSULE_CHECKSUM_OFFSET; ++i) {
        if (wire[i] != 0u) return 0u;
    }

    memset(&decoded, 0, sizeof(decoded));
    if (!s64_session_journal_decode(
            &decoded.journal,
            wire + CAPSULE_JOURNAL_OFFSET
        )
        || decoded.journal.state != S64_SESSION_STATE_ACTIVE
        || decoded.journal.sequence == 0UL
        || decoded.journal.session_id != control->session_id
        || decoded.journal.package_version != control->app_version) {
        return 0u;
    }

    decoded.destination = (S64ModuleId)wire[6];
    decoded.module.id = wire[CAPSULE_MODULE_OFFSET];
    decoded.module.cache_state = wire[CAPSULE_MODULE_OFFSET + 1u];
    decoded.module.abi_version = get16(wire + CAPSULE_MODULE_OFFSET + 2u);
    decoded.module.app_version = get16(wire + CAPSULE_MODULE_OFFSET + 4u);
    decoded.module.load_address = get16(wire + CAPSULE_MODULE_OFFSET + 6u);
    decoded.module.startup_address = get16(wire + CAPSULE_MODULE_OFFSET + 8u);
    decoded.module.byte_length = get32(wire + CAPSULE_MODULE_OFFSET + 10u);
    decoded.module.integrity = get32(wire + CAPSULE_MODULE_OFFSET + 14u);
    decoded.module.cache_address = get32(wire + CAPSULE_MODULE_OFFSET + 18u);
    decoded.module.cache_capacity = get32(wire + CAPSULE_MODULE_OFFSET + 22u);

    if (decoded.module.id != (uint8_t)decoded.destination
        || decoded.module.cache_state != S64_MODULE_CACHE_READY
        || decoded.module.app_version != control->app_version
        || !s64_module_descriptor_loadable(&decoded.module)) {
        return 0u;
    }

    *capsule = decoded;
    return 1u;
}
