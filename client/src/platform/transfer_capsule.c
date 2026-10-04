#include "platform/transfer_capsule_internal.h"
#include "platform/reu.h"

uint8_t capsule_wire[S64_TRANSFER_CAPSULE_WIRE_SIZE];
uint8_t capsule_readback[S64_TRANSFER_CAPSULE_WIRE_SIZE];

uint16_t capsule_checksum(const uint8_t* bytes)
{
    uint16_t value = 0x6454u;
    uint8_t i;

    for (i = 0u; i < CAPSULE_CHECKSUM_OFFSET; ++i) {
        value = (uint16_t)((value << 3) | (value >> 13));
        value ^= bytes[i];
    }
    return value == 0u ? 1u : value;
}

const S64ReuAllocation* capsule_allocation(
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

const char* capsule_filename(S64ModuleId destination)
{
    static const char* const filenames[S64_MODULE_COUNT] = {
        "S64EDIT.PRG", "S64PUB.PRG", "S64CFG.PRG"
    };

    return destination >= S64_MODULE_EDITOR
        && destination <= S64_MODULE_SETTINGS
        ? filenames[(uint8_t)destination - 1u] : 0;
}

uint8_t capsule_module_valid(
    const S64ReuRuntimeControl* control,
    const S64ModuleDescriptor* module,
    S64ModuleId destination
)
{
    const char* filename = capsule_filename(destination);
    uint32_t payload_length;
    uint8_t matches = 0u;
    uint8_t i;

    if (control == 0 || module == 0 || filename == 0
        || module->id != (uint8_t)destination
        || strcmp(module->filename, filename) != 0
        || module->cache_state != S64_MODULE_CACHE_READY
        || module->app_version != control->app_version
        || !s64_module_descriptor_loadable(module)) {
        return 0u;
    }

    payload_length = module->byte_length - 2UL;
    if (payload_length > 65536UL - module->load_address
        || module->startup_address < module->load_address
        || (uint32_t)module->startup_address
            >= (uint32_t)module->load_address + payload_length) {
        return 0u;
    }

    for (i = 0u; i < control->allocation_count; ++i) {
        const S64ReuAllocation* allocation = &control->allocations[i];
        if (allocation->owner == S64_REU_OWNER_MODULE
            && allocation->address == module->cache_address
            && allocation->length == module->cache_capacity) {
            ++matches;
        }
    }
    return matches == 1u;
}


static uint16_t get16(const uint8_t* in)
{
    return (uint16_t)(in[0] | ((uint16_t)in[1] << 8));
}

static uint32_t get32(const uint8_t* in)
{
    return (uint32_t)get16(in) | ((uint32_t)get16(in + 2u) << 16);
}

uint8_t s64_transfer_capsule_clear(const S64ReuRuntimeControl* control)
{
    const S64ReuAllocation* allocation = capsule_allocation(control);

    if (allocation == 0) return 0u;
    memset(capsule_wire, 0, sizeof(capsule_wire));
    return s64_reu_store((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, capsule_wire, sizeof(capsule_wire))
        && s64_reu_load((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, capsule_readback, sizeof(capsule_readback))
        && memcmp(capsule_wire, capsule_readback, sizeof(capsule_wire)) == 0;
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
            (uint16_t)allocation->address, capsule_wire, sizeof(capsule_wire))) {
        return 0u;
    }

    if (capsule_wire[0] != 'S' || capsule_wire[1] != '6' || capsule_wire[2] != '4'
        || capsule_wire[3] != 'T'
        || capsule_wire[4] != S64_TRANSFER_CAPSULE_FORMAT_VERSION
        || capsule_wire[5] != S64_REU_RUNTIME_ABI_VERSION
        || capsule_wire[6] < S64_MODULE_EDITOR || capsule_wire[6] > S64_MODULE_SETTINGS
        || capsule_wire[7] != 0u
        || get16(capsule_wire + CAPSULE_CHECKSUM_OFFSET) != capsule_checksum(capsule_wire)) {
        return 0u;
    }

    for (i = CAPSULE_MODULE_OFFSET + 26u;
            i < CAPSULE_CHECKSUM_OFFSET; ++i) {
        if (capsule_wire[i] != 0u) return 0u;
    }

    memset(&decoded, 0, sizeof(decoded));
    if (!s64_session_journal_decode(
            &decoded.journal,
            capsule_wire + CAPSULE_JOURNAL_OFFSET
        )
        || decoded.journal.state != S64_SESSION_STATE_ACTIVE
        || decoded.journal.sequence == 0UL
        || decoded.journal.session_id != control->session_id
        || decoded.journal.package_version != control->app_version) {
        return 0u;
    }

    decoded.destination = (S64ModuleId)capsule_wire[6];
    decoded.module.id = capsule_wire[CAPSULE_MODULE_OFFSET];
    decoded.module.cache_state = capsule_wire[CAPSULE_MODULE_OFFSET + 1u];
    decoded.module.abi_version = get16(capsule_wire + CAPSULE_MODULE_OFFSET + 2u);
    decoded.module.app_version = get16(capsule_wire + CAPSULE_MODULE_OFFSET + 4u);
    decoded.module.load_address = get16(capsule_wire + CAPSULE_MODULE_OFFSET + 6u);
    decoded.module.startup_address = get16(capsule_wire + CAPSULE_MODULE_OFFSET + 8u);
    decoded.module.byte_length = get32(capsule_wire + CAPSULE_MODULE_OFFSET + 10u);
    decoded.module.integrity = get32(capsule_wire + CAPSULE_MODULE_OFFSET + 14u);
    decoded.module.cache_address = get32(capsule_wire + CAPSULE_MODULE_OFFSET + 18u);
    decoded.module.cache_capacity = get32(capsule_wire + CAPSULE_MODULE_OFFSET + 22u);
    strncpy(decoded.module.filename, capsule_filename(decoded.destination),
        sizeof(decoded.module.filename) - 1u);

    if (!capsule_module_valid(
            control,
            &decoded.module,
            decoded.destination
        )) {
        return 0u;
    }

    *capsule = decoded;
    return 1u;
}
