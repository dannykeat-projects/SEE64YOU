#include "platform/module_session.h"
#include "platform/reu.h"
#include <string.h>

#define JOURNAL_OFFSET 8u
#define MODULE_OFFSET 32u
#define MODULE_SIZE 26u
#define CHECKSUM_OFFSET (S64_MODULE_SESSION_WIRE_SIZE - 2u)

/* Shared scratch keeps this path off the cc65 software stack. Non-reentrant. */
static uint8_t wire[S64_MODULE_SESSION_WIRE_SIZE];
static uint8_t readback[S64_MODULE_SESSION_WIRE_SIZE];
static uint8_t journal_wire[S64_SESSION_JOURNAL_WIRE_SIZE];
static S64ModuleManifest decoded;

static void put16(uint8_t* out, uint16_t value)
{ out[0] = (uint8_t)value; out[1] = (uint8_t)(value >> 8); }
static void put32(uint8_t* out, uint32_t value)
{
    put16(out, (uint16_t)value);
    put16(out + 2, (uint16_t)(value >> 16));
}
static uint16_t get16(const uint8_t* in)
{ return (uint16_t)(in[0] | ((uint16_t)in[1] << 8)); }
static uint32_t get32(const uint8_t* in)
{ return (uint32_t)get16(in) | ((uint32_t)get16(in + 2) << 16); }
static uint16_t checksum(const uint8_t* bytes)
{
    uint16_t value = 0x64a5u;
    uint8_t i;
    for (i = 0u; i < CHECKSUM_OFFSET; ++i) {
        value = (uint16_t)((value << 5) | (value >> 11));
        value ^= bytes[i];
    }
    return value == 0u ? 1u : value;
}

static const S64ReuAllocation* session_allocation(const S64ReuRuntimeControl* control)
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
    const S64ReuAllocation* allocation = session_allocation(control);
    if (allocation == 0) return 0u;
    memset(wire, 0, sizeof(wire));
    return s64_reu_store((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, wire, sizeof(wire))
        && s64_reu_load((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, readback, sizeof(readback))
        && memcmp(wire, readback, sizeof(wire)) == 0;
}

static uint8_t active_session(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal)
{
    return journal != 0 && journal->state == S64_SESSION_STATE_ACTIVE
        && journal->sequence != 0UL && journal->session_id == control->session_id
        && journal->package_version == control->app_version
        && s64_session_journal_encode(journal, journal_wire);
}

static uint8_t package_ready(const S64ReuRuntimeControl* control,
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

uint8_t s64_module_session_store_verified(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const S64ModuleManifest* manifest)
{
    const S64ReuAllocation* allocation = session_allocation(control);
    uint8_t i, offset;
    const S64ModuleDescriptor* module;
    if (allocation == 0 || !active_session(control, journal)
        || !package_ready(control, manifest)) return 0u;
    memset(wire, 0, sizeof(wire));
    wire[0] = 0x53u; wire[1] = 0x36u; wire[2] = 0x34u; wire[3] = 0x4du;
    wire[4] = 1u; /* Record format, independent of runtime/module ABI. */
    wire[5] = S64_REU_RUNTIME_ABI_VERSION;
    wire[6] = S64_MODULE_COUNT;
    memcpy(wire + JOURNAL_OFFSET, journal_wire, sizeof(journal_wire));
    for (i = 0u; i < S64_MODULE_COUNT; ++i) {
        module = &manifest->modules[i];
        offset = MODULE_OFFSET + i * MODULE_SIZE;
        wire[offset] = module->id;
        wire[offset + 1u] = module->cache_state;
        put16(wire + offset + 2u, module->abi_version);
        put16(wire + offset + 4u, module->app_version);
        put16(wire + offset + 6u, module->load_address);
        put16(wire + offset + 8u, module->startup_address);
        put32(wire + offset + 10u, module->byte_length);
        put32(wire + offset + 14u, module->integrity);
        put32(wire + offset + 18u, module->cache_address);
        put32(wire + offset + 22u, module->cache_capacity);
    }
    put16(wire + CHECKSUM_OFFSET, checksum(wire));
    return s64_reu_store((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, wire, sizeof(wire))
        && s64_reu_load((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, readback, sizeof(readback))
        && memcmp(wire, readback, sizeof(wire)) == 0;
}

uint8_t s64_module_session_load(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, S64ModuleManifest* manifest)
{
    const S64ReuAllocation* allocation = session_allocation(control);
    uint8_t i, offset;
    S64ModuleDescriptor* module;
    if (allocation == 0 || manifest == 0 || !active_session(control, journal)
        || !s64_reu_load((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, wire, sizeof(wire))) return 0u;
    if (wire[0] != 0x53u || wire[1] != 0x36u || wire[2] != 0x34u
        || wire[3] != 0x4du || wire[4] != 1u
        || wire[5] != S64_REU_RUNTIME_ABI_VERSION || wire[6] != S64_MODULE_COUNT
        || wire[7] != 0u
        || get16(wire + CHECKSUM_OFFSET) != checksum(wire)
        || memcmp(wire + JOURNAL_OFFSET, journal_wire, sizeof(journal_wire)) != 0)
        return 0u;
    for (i = MODULE_OFFSET + S64_MODULE_COUNT * MODULE_SIZE;
            i < CHECKSUM_OFFSET; ++i) {
        if (wire[i] != 0u) return 0u;
    }
    s64_module_manifest_init(&decoded, control->app_version);
    for (i = 0u; i < S64_MODULE_COUNT; ++i) {
        module = &decoded.modules[i];
        offset = MODULE_OFFSET + i * MODULE_SIZE;
        module->id = wire[offset];
        module->cache_state = wire[offset + 1u];
        module->abi_version = get16(wire + offset + 2u);
        module->app_version = get16(wire + offset + 4u);
        module->load_address = get16(wire + offset + 6u);
        module->startup_address = get16(wire + offset + 8u);
        module->byte_length = get32(wire + offset + 10u);
        module->integrity = get32(wire + offset + 14u);
        module->cache_address = get32(wire + offset + 18u);
        module->cache_capacity = get32(wire + offset + 22u);
    }
    if (!package_ready(control, &decoded)) return 0u;
    *manifest = decoded;
    return 1u;
}
