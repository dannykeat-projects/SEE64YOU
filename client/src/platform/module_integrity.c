#include "platform/module_preload.h"
#include "platform/reu.h"
#include "platform/module_integrity.h"
#include "platform/crc32.h"

static uint8_t crc_scratch[256];

uint8_t s64_reu_crc32_matches(uint32_t address, uint32_t remaining,
    uint32_t expected, S64SystemState* system)
{
    uint32_t bank_remaining, crc;
    uint16_t length;

    if (!system || !system->captured || system->restored
        || system->mode != S64_SYSTEM_MODE_IO_SAFE || !remaining || !expected
        || address >= S64_REU_RUNTIME_HEAP_END
        || remaining > S64_REU_RUNTIME_HEAP_END - address) return 0u;
    crc = 0xffffffffUL;
    while (remaining != 0UL) {
        length = remaining > sizeof(crc_scratch)
            ? sizeof(crc_scratch) : (uint16_t)remaining;
        bank_remaining = 65536UL - (uint16_t)address;
        if (bank_remaining < length) length = (uint16_t)bank_remaining;
        if (!s64_reu_load((uint8_t)(address >> 16), (uint16_t)address,
                crc_scratch, length)) return 0u;
        if (s64_system_state_set_mode(system, S64_SYSTEM_MODE_CPU_INTENSIVE)
                != S64_SYSTEM_OK) return 0u;
        crc = s64_crc32_update(crc, crc_scratch, length);
        if (s64_system_state_set_mode(system, S64_SYSTEM_MODE_IO_SAFE)
                != S64_SYSTEM_OK) return 0u;
        address += length;
        remaining -= length;
    }
    return (crc ^ 0xffffffffUL) == expected;
}

uint8_t s64_module_cache_verify(S64ModuleManifest* manifest, S64SystemState* system)
{
    uint8_t i;
    S64ModuleDescriptor* module;
    if (manifest == 0) return 0u;
    /* Reverification revokes all previous readiness before any DMA/failure. */
    for (i = 0u; i < S64_MODULE_COUNT; ++i) {
        if (manifest->modules[i].cache_state == S64_MODULE_CACHE_READY) {
            manifest->modules[i].cache_state = S64_MODULE_CACHE_LOADED_UNVERIFIED;
        }
    }
    if (system == 0 || !system->captured || system->restored
        || system->mode != S64_SYSTEM_MODE_IO_SAFE
        || !s64_module_manifest_loadable(manifest)) return 0u;
    for (i = 0u; i < S64_MODULE_COUNT; ++i) {
        module = &manifest->modules[i];
        if (module->cache_state != S64_MODULE_CACHE_LOADED_UNVERIFIED
            || module->cache_address >= 0x01000000UL
            || module->cache_capacity > 0x01000000UL - module->cache_address
            || !s64_reu_crc32_matches(module->cache_address, module->byte_length,
                module->integrity, system)) {
            /* Best effort on failure; the session owner still performs verified
             * restoration before recording CLEAN. */
            s64_system_state_set_mode(system, S64_SYSTEM_MODE_IO_SAFE);
            return 0u;
        }
    }
    /* Commit readiness only after the whole package passes. */
    for (i = 0u; i < S64_MODULE_COUNT; ++i) {
        manifest->modules[i].cache_state = S64_MODULE_CACHE_READY;
    }
    return 1u;
}
