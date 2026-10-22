#include "platform/module_session_internal.h"
#include "platform/reu.h"
#include <string.h>

static void put16(uint8_t* out, uint16_t value)
{ out[0] = (uint8_t)value; out[1] = (uint8_t)(value >> 8); }
static void put32(uint8_t* out, uint32_t value)
{
    put16(out, (uint16_t)value);
    put16(out + 2, (uint16_t)(value >> 16));
}

uint8_t s64_module_session_reserve(S64ReuRuntimeControl* control)
{
    S64ReuAllocation allocation;
    uint8_t i;
    if (!s64_reu_runtime_valid(control)) return 0u;
    for (i = 0u; i < control->allocation_count; ++i) {
        if (control->allocations[i].owner == S64_REU_OWNER_RUNTIME
            && control->allocations[i].flags == S64_MODULE_SESSION_ALLOCATION_FLAG)
            return 0u;
    }
    if (!s64_reu_runtime_allocate(control, S64_REU_OWNER_RUNTIME,
            S64_MODULE_SESSION_WIRE_SIZE, 256u, &allocation)) return 0u;
    control->allocations[control->allocation_count - 1u].flags =
        S64_MODULE_SESSION_ALLOCATION_FLAG;
    return 1u;
}

uint8_t s64_module_session_store_verified(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const S64ModuleManifest* manifest)
{
    const S64ReuAllocation* allocation = s64_module_session_allocation(control);
    uint8_t i, offset;
    const S64ModuleDescriptor* module;
    if (allocation == 0 || !s64_module_session_active_session(control, journal)
        || !s64_module_session_package_ready(control, manifest)) return 0u;
    memset(s64_module_session_wire, 0, sizeof(s64_module_session_wire));
    s64_module_session_wire[0] = 0x53u;
    s64_module_session_wire[1] = 0x36u;
    s64_module_session_wire[2] = 0x34u;
    s64_module_session_wire[3] = 0x4du;
    s64_module_session_wire[4] = 1u; /* Record format, independent of runtime/module ABI. */
    s64_module_session_wire[5] = S64_REU_RUNTIME_ABI_VERSION;
    s64_module_session_wire[6] = S64_MODULE_COUNT;
    memcpy(s64_module_session_wire + S64_MODULE_SESSION_JOURNAL_OFFSET,
        s64_module_session_journal_wire, sizeof(s64_module_session_journal_wire));
    for (i = 0u; i < S64_MODULE_COUNT; ++i) {
        module = &manifest->modules[i];
        offset = S64_MODULE_SESSION_MODULE_OFFSET + i * S64_MODULE_SESSION_MODULE_SIZE;
        s64_module_session_wire[offset] = module->id;
        s64_module_session_wire[offset + 1u] = module->cache_state;
        put16(s64_module_session_wire + offset + 2u, module->abi_version);
        put16(s64_module_session_wire + offset + 4u, module->app_version);
        put16(s64_module_session_wire + offset + 6u, module->load_address);
        put16(s64_module_session_wire + offset + 8u, module->startup_address);
        put32(s64_module_session_wire + offset + 10u, module->byte_length);
        put32(s64_module_session_wire + offset + 14u, module->integrity);
        put32(s64_module_session_wire + offset + 18u, module->cache_address);
        put32(s64_module_session_wire + offset + 22u, module->cache_capacity);
    }
    put16(s64_module_session_wire + S64_MODULE_SESSION_CHECKSUM_OFFSET,
        s64_module_session_checksum(s64_module_session_wire));
    return s64_reu_store((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, s64_module_session_wire,
            sizeof(s64_module_session_wire))
        && s64_reu_load((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, s64_module_session_readback,
            sizeof(s64_module_session_readback))
        && memcmp(s64_module_session_wire, s64_module_session_readback,
            sizeof(s64_module_session_wire)) == 0;
}
