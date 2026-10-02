#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "platform/module_session.h"

static S64ReuRuntimeControl control;
static S64ModuleManifest manifest, output, untouched;
static S64SessionJournal journal;
static uint8_t cache_record[S64_MODULE_SESSION_WIRE_SIZE];
static uint8_t fail_store, fail_load, corrupt_read;
static unsigned stores, loads;

uint8_t s64_reu_store(uint8_t bank, uint16_t address, const void* source, uint16_t size)
{
    assert(bank == 5 && address == 0x100 && size == sizeof(cache_record));
    ++stores;
    if (fail_store) return 0;
    memcpy(cache_record, source, size);
    return 1;
}
uint8_t s64_reu_load(uint8_t bank, uint16_t address, void* destination, uint16_t size)
{
    assert(bank == 5 && address == 0x100 && size == sizeof(cache_record));
    ++loads;
    if (fail_load) return 0;
    memcpy(destination, cache_record, size);
    if (corrupt_read) ((uint8_t*)destination)[0] ^= 1;
    return 1;
}

static void fixture(void)
{
    unsigned i;
    S64SystemSnapshot snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    snapshot.turbo_control_valid = 1;
    snapshot.turbo_control = 0x82;
    s64_reu_runtime_init(&control, 0x201, 0x11223344UL);
    s64_module_manifest_init(&manifest, 0x201);
    assert(s64_module_manifest_allocate(&manifest, &control));
    assert(s64_module_session_reserve(&control));
    assert(!s64_module_session_reserve(&control));
    assert(control.allocation_count == 4);
    assert(control.next_free == 0x050180UL);
    for (i = 0; i < S64_MODULE_COUNT; ++i) {
        manifest.modules[i].cache_state = S64_MODULE_CACHE_READY;
        manifest.modules[i].load_address = 0x801;
        manifest.modules[i].startup_address = 0x80d;
        manifest.modules[i].byte_length = 256;
        manifest.modules[i].integrity = 0x12345678UL + i;
    }
    assert(s64_session_journal_prepare(&journal, control.session_id,
        control.app_version, 0x2345, &snapshot));
    journal.sequence = 7;
    memset(cache_record, 0, sizeof(cache_record));
    memset(&output, 0xa5, sizeof(output));
    untouched = output;
    stores = loads = fail_store = fail_load = corrupt_read = 0;
}
static void rejected(void)
{
    assert(!s64_module_session_load(&control, &journal, &output));
    assert(memcmp(&output, &untouched, sizeof(output)) == 0);
}
/* Rechecksum deliberate semantic mutations to test more than corruption detection. */
static void rechecksum(void)
{
    unsigned i;
    uint16_t c = 0x64a5;
    for (i = 0; i < 126; ++i) {
        c = (uint16_t)((c << 5) | (c >> 11));
        c ^= cache_record[i];
    }
    if (!c) c = 1;
    cache_record[126] = (uint8_t)c;
    cache_record[127] = (uint8_t)(c >> 8);
}
int main(void)
{
    unsigned i;
    uint8_t original[128];
    fixture(); rejected();
    assert(s64_module_session_store_verified(&control, &journal, &manifest));
    assert(stores == 1 && loads == 2);
    assert(cache_record[0] == 0x53 && cache_record[4] == 1);
    assert(s64_module_session_load(&control, &journal, &output));
    for (i = 0; i < S64_MODULE_COUNT; ++i) {
        assert(output.modules[i].id == manifest.modules[i].id);
        assert(!strcmp(output.modules[i].filename, manifest.modules[i].filename));
        assert(output.modules[i].cache_state == S64_MODULE_CACHE_READY);
        assert(output.modules[i].abi_version == manifest.modules[i].abi_version);
        assert(output.modules[i].app_version == manifest.modules[i].app_version);
        assert(output.modules[i].load_address == manifest.modules[i].load_address);
        assert(output.modules[i].startup_address == manifest.modules[i].startup_address);
        assert(output.modules[i].byte_length == manifest.modules[i].byte_length);
        assert(output.modules[i].integrity == manifest.modules[i].integrity);
        assert(output.modules[i].cache_address == manifest.modules[i].cache_address);
        assert(output.modules[i].cache_capacity == manifest.modules[i].cache_capacity);
    }
    memcpy(original, cache_record, sizeof(original));
    output = untouched;
    for (i = 0; i < sizeof(cache_record); ++i) {
        memcpy(cache_record, original, sizeof(original));
        cache_record[i] ^= 1;
        rejected();
    }
    memcpy(cache_record, original, sizeof(original));
    ++journal.sequence; rejected(); --journal.sequence;
    ++journal.session_id; rejected(); --journal.session_id;
    ++journal.package_version; rejected(); --journal.package_version;
    ++journal.recovery_ref; rejected(); --journal.recovery_ref;
    journal.turbo_control ^= 1; rejected(); journal.turbo_control ^= 1;
    journal.restore_flags = 0; rejected(); journal.restore_flags = 1;
    journal.state = S64_SESSION_STATE_CLEAN; rejected(); journal.state = S64_SESSION_STATE_ACTIVE;
    ++control.session_id; rejected(); --control.session_id;
    ++control.app_version; rejected(); --control.app_version;
    for (i = 0; i < S64_MODULE_COUNT; ++i) {
        manifest.modules[i].cache_state = S64_MODULE_CACHE_LOADED_UNVERIFIED;
        assert(!s64_module_session_store_verified(&control, &journal, &manifest));
        manifest.modules[i].cache_state = S64_MODULE_CACHE_READY;
    }
    manifest.modules[0].cache_address += 256;
    assert(!s64_module_session_store_verified(&control, &journal, &manifest));
    manifest.modules[0].cache_address -= 256;
    strcpy(manifest.modules[0].filename, "WRONG.PRG");
    assert(!s64_module_session_store_verified(&control, &journal, &manifest));
    strcpy(manifest.modules[0].filename, "S64EDIT.PRG");
    /* Valid checksum cannot grant readiness or retarget an allocation. */
    cache_record[33] = S64_MODULE_CACHE_LOADED_UNVERIFIED; rechecksum(); rejected();
    memcpy(cache_record, original, sizeof(original));
    cache_record[50] ^= 1; rechecksum(); rejected();
    memcpy(cache_record, original, sizeof(original));
    cache_record[58] = S64_MODULE_EDITOR; rechecksum(); rejected();
    memcpy(cache_record, original, sizeof(original));
    cache_record[110] = 1; rechecksum(); rejected();
    memcpy(cache_record, original, sizeof(original));
    fail_store = 1;
    assert(!s64_module_session_store_verified(&control, &journal, &manifest));
    fail_store = 0; fail_load = 1;
    assert(!s64_module_session_store_verified(&control, &journal, &manifest));
    rejected(); fail_load = 0; corrupt_read = 1;
    assert(!s64_module_session_store_verified(&control, &journal, &manifest));
    rejected(); corrupt_read = 0;
    assert(s64_module_session_clear(&control)); rejected();
    control.allocations[3].length = 127;
    assert(!s64_module_session_reserve(&control));
    assert(!s64_module_session_clear(&control)); rejected();
    assert(!s64_module_session_load(0, &journal, &output));
    assert(!s64_module_session_load(&control, 0, &output));
    puts("module session tests passed");
    return 0;
}
