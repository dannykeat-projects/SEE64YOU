#include "platform/module_session_internal.h"
#include "platform/reu.h"
#include <string.h>

/* Shared scratch keeps this path off the cc65 software stack. Non-reentrant.
 * Managed overlays reuse the reserved $C800 synchronous workspace instead. */
#ifndef S64_MANAGED_OVERLAY
uint8_t s64_module_session_wire[S64_MODULE_SESSION_WIRE_SIZE];
uint8_t s64_module_session_readback[S64_MODULE_SESSION_WIRE_SIZE];
uint8_t s64_module_session_journal_wire[S64_SESSION_JOURNAL_WIRE_SIZE];
#endif

uint16_t s64_module_session_checksum(const uint8_t* bytes)
{
    uint16_t value = 0x64a5u;
    uint8_t i;
    for (i = 0u; i < S64_MODULE_SESSION_CHECKSUM_OFFSET; ++i) {
        value = (uint16_t)((value << 5) | (value >> 11));
        value ^= bytes[i];
    }
    return value == 0u ? 1u : value;
}

const S64ReuAllocation* s64_module_session_allocation(const S64ReuRuntimeControl* control)
{
    const S64ReuAllocation* found = 0;
    uint8_t i;
    if (!s64_reu_runtime_valid(control)) return 0;
    for (i = 0u; i < control->allocation_count; ++i) {
        const S64ReuAllocation* allocation = &control->allocations[i];
        if (allocation->owner == S64_REU_OWNER_RUNTIME
            && allocation->flags == S64_MODULE_SESSION_ALLOCATION_FLAG) {
            if (found != 0 || allocation->length != S64_MODULE_SESSION_WIRE_SIZE
                || (allocation->address & 255UL) != 0UL) return 0;
            found = allocation;
        }
    }
    return found;
}

uint8_t s64_module_session_clear(const S64ReuRuntimeControl* control)
{
    const S64ReuAllocation* allocation = s64_module_session_allocation(control);
    if (allocation == 0) return 0u;
    memset(s64_module_session_wire, 0, sizeof(s64_module_session_wire));
    return s64_reu_store((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, s64_module_session_wire,
            sizeof(s64_module_session_wire))
        && s64_reu_load((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, s64_module_session_readback,
            sizeof(s64_module_session_readback))
        && memcmp(s64_module_session_wire, s64_module_session_readback,
            sizeof(s64_module_session_wire)) == 0;
}

uint8_t s64_module_session_active_session(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal)
{
    return journal != 0 && journal->state == S64_SESSION_STATE_ACTIVE
        && journal->sequence != 0UL && journal->session_id == control->session_id
        && journal->package_version == control->app_version
        && s64_session_journal_encode(journal, s64_module_session_journal_wire);
}

uint8_t s64_module_session_package_ready(const S64ReuRuntimeControl* control,
    const S64ModuleManifest* manifest)
{
    static const char* const filenames[S64_MODULE_COUNT] = {
        "S64EDIT.PRG", "S64PUB.PRG", "S64CFG.PRG"
    };
    uint8_t i, j, matches;
    const S64ModuleDescriptor* module;
    if (manifest == 0 || !s64_module_manifest_loadable(manifest)) return 0u;
    for (i = 0u; i < S64_MODULE_COUNT; ++i) {
        module = &manifest->modules[i];
        if (module->id != i + 1u || strcmp(module->filename, filenames[i]) != 0
            || module->app_version != control->app_version
            || module->cache_state != S64_MODULE_CACHE_READY) return 0u;
        matches = 0u;
        for (j = 0u; j < control->allocation_count; ++j) {
            const S64ReuAllocation* allocation = &control->allocations[j];
            if (allocation->owner == S64_REU_OWNER_MODULE
                && allocation->address == module->cache_address
                && allocation->length == module->cache_capacity) ++matches;
        }
        if (matches != 1u) return 0u;
        for (j = 0u; j < i; ++j) {
            if (manifest->modules[j].cache_address == module->cache_address)
                return 0u;
        }
    }
    return 1u;
}
