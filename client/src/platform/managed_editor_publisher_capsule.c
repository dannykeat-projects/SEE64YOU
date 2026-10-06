#include "platform/module_runtime.h"
#include "platform/transfer_capsule_internal.h"
#include "platform/reu.h"

#include <string.h>

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

uint8_t s64_managed_editor_publisher_capsule_store(
    S64ModuleRuntime* runtime,
    uint8_t* scratch
)
{
    const S64ReuAllocation* allocation;
    const S64ModuleDescriptor* module;
    uint8_t* readback;

    if (!runtime || !scratch
        || runtime->current_module != S64_MODULE_EDITOR
        || runtime->system_state.mode != S64_SYSTEM_MODE_IO_SAFE
        || !runtime->lifecycle.active_record_written
        || runtime->lifecycle.clean_record_written
        || runtime->shared_invalidated) {
        return 0u;
    }

    allocation = s64_transfer_capsule_allocation(&runtime->reu_control);
    module = &runtime->module_manifest.modules[S64_MODULE_PUBLISHER - 1u];
    if (!allocation
        || !s64_transfer_capsule_module_valid(
            &runtime->reu_control, module, S64_MODULE_PUBLISHER)
        || runtime->lifecycle.journal.state != S64_SESSION_STATE_ACTIVE
        || !runtime->lifecycle.journal.sequence
        || runtime->lifecycle.journal.session_id != runtime->reu_control.session_id
        || runtime->lifecycle.journal.package_version
            != runtime->reu_control.app_version) {
        return 0u;
    }

    memset(scratch, 0, S64_TRANSFER_CAPSULE_WIRE_SIZE);
    scratch[0] = 'S';
    scratch[1] = '6';
    scratch[2] = '4';
    scratch[3] = 'T';
    scratch[4] = S64_TRANSFER_CAPSULE_FORMAT_VERSION;
    scratch[5] = S64_REU_RUNTIME_ABI_VERSION;
    scratch[6] = S64_MODULE_PUBLISHER;
    if (!s64_session_journal_encode(
            &runtime->lifecycle.journal,
            scratch + CAPSULE_JOURNAL_OFFSET
        )) {
        return 0u;
    }

    scratch[CAPSULE_MODULE_OFFSET] = module->id;
    scratch[CAPSULE_MODULE_OFFSET + 1u] = module->cache_state;
    put16(scratch + CAPSULE_MODULE_OFFSET + 2u, module->abi_version);
    put16(scratch + CAPSULE_MODULE_OFFSET + 4u, module->app_version);
    put16(scratch + CAPSULE_MODULE_OFFSET + 6u, module->load_address);
    put16(scratch + CAPSULE_MODULE_OFFSET + 8u, module->startup_address);
    put32(scratch + CAPSULE_MODULE_OFFSET + 10u, module->byte_length);
    put32(scratch + CAPSULE_MODULE_OFFSET + 14u, module->integrity);
    put32(scratch + CAPSULE_MODULE_OFFSET + 18u, module->cache_address);
    put32(scratch + CAPSULE_MODULE_OFFSET + 22u, module->cache_capacity);
    put16(scratch + CAPSULE_CHECKSUM_OFFSET,
        s64_transfer_capsule_checksum(scratch));

    readback = scratch + S64_TRANSFER_CAPSULE_WIRE_SIZE;
    return s64_reu_store(
            (uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address,
            scratch,
            S64_TRANSFER_CAPSULE_WIRE_SIZE
        )
        && s64_reu_load(
            (uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address,
            readback,
            S64_TRANSFER_CAPSULE_WIRE_SIZE
        )
        && memcmp(scratch, readback, S64_TRANSFER_CAPSULE_WIRE_SIZE) == 0;
}
