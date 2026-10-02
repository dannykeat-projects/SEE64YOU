#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "platform/module_preload.h"
#include "platform/reu.h"

static S64ModuleManifest manifest;
static uint8_t cache[S64_MODULE_COUNT][65536];
static unsigned reads, fail_read;

uint8_t s64_reu_load(uint8_t bank, uint16_t address, void* destination, uint16_t length)
{
    uint32_t linear = ((uint32_t)bank << 16) | address;
    unsigned i;
    ++reads;
    assert(length > 0 && length <= 256);
    assert((uint32_t)address + length <= 65536UL);
    if (reads == fail_read) return 0;
    for (i = 0; i < S64_MODULE_COUNT; ++i) {
        const S64ModuleDescriptor* m = &manifest.modules[i];
        if (linear >= m->cache_address
            && linear - m->cache_address + length <= m->byte_length) {
            memcpy(destination, cache[i] + linear - m->cache_address, length);
            return 1;
        }
    }
    assert(0); /* No read may escape a module's exact file extent. */
    return 0;
}
uint8_t s64_reu_runtime_valid(const S64ReuRuntimeControl* control)
{ (void)control; return 0; }
uint8_t s64_reu_runtime_allocate(S64ReuRuntimeControl* c, S64ReuOwner owner,
    uint32_t size, uint16_t alignment, S64ReuAllocation* allocation)
{ (void)c; (void)owner; (void)size; (void)alignment; (void)allocation; return 0; }

static void fixture(void)
{
    unsigned i, j;
    s64_module_manifest_init(&manifest, 0x201);
    for (i = 0; i < S64_MODULE_COUNT; ++i) {
        manifest.modules[i].load_address = 0x801;
        manifest.modules[i].startup_address = 0x80d;
        manifest.modules[i].cache_state = S64_MODULE_CACHE_LOADED_UNVERIFIED;
        for (j = 0; j < 65536; ++j) cache[i][j] = (uint8_t)j;
    }
    memcpy(cache[0], "123456789", 9);
    manifest.modules[0].cache_address = 0x03fff9UL;
    manifest.modules[0].byte_length = 9;
    manifest.modules[0].integrity = 0xcbf43926UL;
    manifest.modules[1].cache_address = 0x050123UL;
    manifest.modules[1].byte_length = 513;
    manifest.modules[1].integrity = 0x6b785ab1UL;
    manifest.modules[2].cache_address = 0xffff00UL;
    manifest.modules[2].cache_capacity = 256;
    manifest.modules[2].byte_length = 256;
    manifest.modules[2].integrity = 0x29058c73UL;
    reads = fail_read = 0;
}
static void unverified(void)
{
    unsigned i;
    for (i = 0; i < S64_MODULE_COUNT; ++i)
        assert(manifest.modules[i].cache_state != S64_MODULE_CACHE_READY);
}
int main(void)
{
    unsigned i;
    fixture();
    assert(s64_module_cache_verify(&manifest));
    assert(reads == 6); /* Includes the split of the first file across banks. */
    for (i = 0; i < S64_MODULE_COUNT; ++i)
        assert(manifest.modules[i].cache_state == S64_MODULE_CACHE_READY);
    cache[0][0] ^= 1; /* Header bytes are part of integrity, too. */
    assert(!s64_module_cache_verify(&manifest));
    unverified();
    fixture(); cache[2][255] ^= 1;
    assert(!s64_module_cache_verify(&manifest)); unverified();
    for (i = 1; i <= 6; ++i) {
        fixture(); fail_read = i;
        assert(!s64_module_cache_verify(&manifest)); unverified();
    }
    fixture(); manifest.modules[1].cache_state = S64_MODULE_CACHE_EMPTY;
    assert(!s64_module_cache_verify(&manifest)); unverified();
    fixture(); manifest.modules[0].cache_address = 0x1000000UL;
    assert(!s64_module_cache_verify(&manifest)); assert(reads == 0); unverified();
    fixture(); manifest.modules[0].cache_address = 0xfffffcUL;
    assert(!s64_module_cache_verify(&manifest)); assert(reads == 0); unverified();
    fixture(); manifest.modules[1].cache_address = 0x050000UL;
    manifest.modules[1].byte_length = 65536UL;
    manifest.modules[1].integrity = 0xb11de6a1UL;
    assert(s64_module_cache_verify(&manifest));
    assert(!s64_module_cache_verify(0));
    puts("module integrity tests passed");
    return 0;
}
