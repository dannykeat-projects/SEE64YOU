#include "platform/module_session_internal.h"
#include "platform/reu.h"
#include <string.h>

#ifdef S64_PERSISTENT_RUNTIME_FIT
#include "platform/persistent_fit_scratch.h"
#define decoded (*(S64ModuleManifest*)S64_PERSISTENT_FIT_SCRATCH_BASE)
typedef char S64PersistentFitModuleDecodeFits[
    S64_PERSISTENT_FIT_SCRATCH_CAPACITY - (int)sizeof(S64ModuleManifest)
];
#else
static S64ModuleManifest decoded;
#endif

static uint16_t get16(const uint8_t* in)
{ return (uint16_t)(in[0] | ((uint16_t)in[1] << 8)); }
static uint32_t get32(const uint8_t* in)
{ return (uint32_t)get16(in) | ((uint32_t)get16(in + 2) << 16); }

uint8_t s64_module_session_load(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, S64ModuleManifest* manifest)
{
    const S64ReuAllocation* allocation = s64_module_session_allocation(control);
    uint8_t i, offset;
    S64ModuleDescriptor* module;
    if (allocation == 0 || manifest == 0
        || !s64_module_session_active_session(control, journal)
        || !s64_reu_load((uint8_t)(allocation->address >> 16),
            (uint16_t)allocation->address, s64_module_session_wire,
            sizeof(s64_module_session_wire))) return 0u;
    if (s64_module_session_wire[0] != 0x53u
        || s64_module_session_wire[1] != 0x36u
        || s64_module_session_wire[2] != 0x34u
        || s64_module_session_wire[3] != 0x4du
        || s64_module_session_wire[4] != 1u
        || s64_module_session_wire[5] != S64_REU_RUNTIME_ABI_VERSION
        || s64_module_session_wire[6] != S64_MODULE_COUNT
        || s64_module_session_wire[7] != 0u
        || get16(s64_module_session_wire + S64_MODULE_SESSION_CHECKSUM_OFFSET)
            != s64_module_session_checksum(s64_module_session_wire)
        || memcmp(s64_module_session_wire + S64_MODULE_SESSION_JOURNAL_OFFSET,
            s64_module_session_journal_wire,
            sizeof(s64_module_session_journal_wire)) != 0) return 0u;
    for (i = S64_MODULE_SESSION_MODULE_OFFSET
            + S64_MODULE_COUNT * S64_MODULE_SESSION_MODULE_SIZE;
            i < S64_MODULE_SESSION_CHECKSUM_OFFSET; ++i) {
        if (s64_module_session_wire[i] != 0u) return 0u;
    }
    s64_module_manifest_init(&decoded, control->app_version);
    for (i = 0u; i < S64_MODULE_COUNT; ++i) {
        module = &decoded.modules[i];
        offset = S64_MODULE_SESSION_MODULE_OFFSET
            + i * S64_MODULE_SESSION_MODULE_SIZE;
        module->id = s64_module_session_wire[offset];
        module->cache_state = s64_module_session_wire[offset + 1u];
        module->abi_version = get16(s64_module_session_wire + offset + 2u);
        module->app_version = get16(s64_module_session_wire + offset + 4u);
        module->load_address = get16(s64_module_session_wire + offset + 6u);
        module->startup_address = get16(s64_module_session_wire + offset + 8u);
        module->byte_length = get32(s64_module_session_wire + offset + 10u);
        module->integrity = get32(s64_module_session_wire + offset + 14u);
        module->cache_address = get32(s64_module_session_wire + offset + 18u);
        module->cache_capacity = get32(s64_module_session_wire + offset + 22u);
    }
    if (!s64_module_session_package_ready(control, &decoded)) return 0u;
    *manifest = decoded;
    return 1u;
}
