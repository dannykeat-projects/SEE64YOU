#include "platform/transfer_capsule_internal.h"
#include "platform/reu.h"

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

static const S64ModuleDescriptor* destination_module(
    const S64ReuRuntimeControl* control,
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
    return s64_transfer_capsule_module_valid(control, module, destination) ? module : 0;
}

uint8_t s64_transfer_capsule_store(
    const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal,
    const S64ModuleManifest* manifest,
    S64ModuleId destination
)
{
    const S64ReuAllocation* allocation = s64_transfer_capsule_allocation(control);
    const S64ModuleDescriptor* module =
        destination_module(control, manifest, destination);

    if (allocation == 0 || journal == 0 || module == 0
        || journal->state != S64_SESSION_STATE_ACTIVE
        || journal->sequence == 0UL
        || journal->session_id != control->session_id
        || journal->package_version != control->app_version
        || !s64_session_journal_encode(journal, journal_wire)) {
        return 0u;
    }

    memset(capsule_wire, 0, sizeof(capsule_wire));
    capsule_wire[0] = 'S';
    capsule_wire[1] = '6';
    capsule_wire[2] = '4';
    capsule_wire[3] = 'T';
    capsule_wire[4] = S64_TRANSFER_CAPSULE_FORMAT_VERSION;
    capsule_wire[5] = S64_REU_RUNTIME_ABI_VERSION;
    capsule_wire[6] = (uint8_t)destination;
    capsule_wire[7] = 0u;
    memcpy(capsule_wire + CAPSULE_JOURNAL_OFFSET, journal_wire,
        sizeof(journal_wire));

    capsule_wire[CAPSULE_MODULE_OFFSET] = module->id;
    capsule_wire[CAPSULE_MODULE_OFFSET + 1u] = module->cache_state;
    put16(capsule_wire + CAPSULE_MODULE_OFFSET + 2u, module->abi_version);
    put16(capsule_wire + CAPSULE_MODULE_OFFSET + 4u, module->app_version);
    put16(capsule_wire + CAPSULE_MODULE_OFFSET + 6u, module->load_address);
    put16(capsule_wire + CAPSULE_MODULE_OFFSET + 8u, module->startup_address);
    put32(capsule_wire + CAPSULE_MODULE_OFFSET + 10u, module->byte_length);
    put32(capsule_wire + CAPSULE_MODULE_OFFSET + 14u, module->integrity);
    put32(capsule_wire + CAPSULE_MODULE_OFFSET + 18u, module->cache_address);
    put32(capsule_wire + CAPSULE_MODULE_OFFSET + 22u, module->cache_capacity);
    put16(capsule_wire + CAPSULE_CHECKSUM_OFFSET, s64_transfer_capsule_checksum(capsule_wire));

    return s64_reu_store((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, capsule_wire, sizeof(capsule_wire))
        && s64_reu_load((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, capsule_readback, sizeof(capsule_readback))
        && memcmp(capsule_wire, capsule_readback, sizeof(capsule_wire)) == 0;
}
