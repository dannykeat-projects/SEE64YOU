#include "platform/module_preload.h"
#include "platform/reu.h"

/* Small nibble table avoids a 1 KiB table and eight bit steps per byte. */
static const uint32_t crc_table[16] = {
    0x00000000UL, 0x1db71064UL, 0x3b6e20c8UL, 0x26d930acUL,
    0x76dc4190UL, 0x6b6b51f4UL, 0x4db26158UL, 0x5005713cUL,
    0xedb88320UL, 0xf00f9344UL, 0xd6d6a3e8UL, 0xcb61b38cUL,
    0x9b64c2b0UL, 0x86d3d2d4UL, 0xa00ae278UL, 0xbdbdf21cUL
};
static uint8_t crc_scratch[256];

static uint8_t verify_module(const S64ModuleDescriptor* module)
{
    uint32_t address, remaining, bank_remaining, crc;
    uint16_t length, i;

    address = module->cache_address;
    remaining = module->byte_length;
    crc = 0xffffffffUL;
    while (remaining != 0UL) {
        length = remaining > sizeof(crc_scratch)
            ? sizeof(crc_scratch) : (uint16_t)remaining;
        bank_remaining = 65536UL - (uint16_t)address;
        if (bank_remaining < length) length = (uint16_t)bank_remaining;
        if (!s64_reu_load((uint8_t)(address >> 16), (uint16_t)address,
                crc_scratch, length)) return 0u;
        for (i = 0u; i < length; ++i) {
            crc ^= crc_scratch[i];
            crc = (crc >> 4) ^ crc_table[(uint8_t)(crc & 15UL)];
            crc = (crc >> 4) ^ crc_table[(uint8_t)(crc & 15UL)];
        }
        address += length;
        remaining -= length;
    }
    return (crc ^ 0xffffffffUL) == module->integrity;
}

uint8_t s64_module_cache_verify(S64ModuleManifest* manifest)
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
    if (!s64_module_manifest_loadable(manifest)) return 0u;
    for (i = 0u; i < S64_MODULE_COUNT; ++i) {
        module = &manifest->modules[i];
        if (module->cache_state != S64_MODULE_CACHE_LOADED_UNVERIFIED
            || module->cache_address >= 0x01000000UL
            || module->cache_capacity > 0x01000000UL - module->cache_address
            || !verify_module(module)) return 0u;
    }
    /* Commit readiness only after the whole package passes. */
    for (i = 0u; i < S64_MODULE_COUNT; ++i) {
        manifest->modules[i].cache_state = S64_MODULE_CACHE_READY;
    }
    return 1u;
}
