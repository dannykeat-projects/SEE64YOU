#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "platform/module_session.h"
#include "platform/transfer_capsule.h"

static uint8_t capsule_wire[S64_TRANSFER_CAPSULE_WIRE_SIZE];
static uint8_t fail_store, fail_load, corrupt_read;

uint8_t s64_reu_store(uint8_t bank, uint16_t address,
    const void* source, uint16_t length)
{
    assert(bank == 5u && address == 0x0200u
        && length == sizeof(capsule_wire));
    if (fail_store) return 0u;
    memcpy(capsule_wire, source, length);
    return 1u;
}

uint8_t s64_reu_load(uint8_t bank, uint16_t address,
    void* destination, uint16_t length)
{
    assert(bank == 5u && address == 0x0200u
        && length == sizeof(capsule_wire));
    if (fail_load) return 0u;
    memcpy(destination, capsule_wire, length);
    if (corrupt_read) ((uint8_t*)destination)[0] ^= 1u;
    return 1u;
}

static void fixture(S64ReuRuntimeControl* control,
    S64ModuleManifest* manifest, S64SessionJournal* journal)
{
    unsigned i;
    S64SystemSnapshot snapshot;

    memset(&snapshot, 0, sizeof(snapshot));
    snapshot.turbo_control_valid = 1u;
    snapshot.turbo_control = 0x22u;

    s64_reu_runtime_init(control, 0x0201u, 0x11223344UL);
    s64_module_manifest_init(manifest, control->app_version);
    assert(s64_module_manifest_allocate(manifest, control));
    assert(s64_module_session_reserve(control));
    assert(s64_transfer_capsule_reserve(control));
    assert(!s64_transfer_capsule_reserve(control));
    assert(control->allocation_count == 5u);
    assert(control->allocations[4].address == 0x050200UL);
    assert(control->next_free == 0x050240UL);

    for (i = 0u; i < S64_MODULE_COUNT; ++i) {
        manifest->modules[i].cache_state = S64_MODULE_CACHE_READY;
        manifest->modules[i].load_address = 0x0801u;
        manifest->modules[i].startup_address = 0x080du;
        manifest->modules[i].byte_length = 4096UL + i;
        manifest->modules[i].integrity = 0x12340000UL + i;
    }

    assert(s64_session_journal_prepare(journal, control->session_id,
        control->app_version, 0x3344UL, &snapshot));
    journal->sequence = 9UL;
    memset(capsule_wire, 0, sizeof(capsule_wire));
    fail_store = fail_load = corrupt_read = 0u;
}

int main(void)
{
    S64ReuRuntimeControl control;
    S64ModuleManifest manifest;
    S64SessionJournal journal;
    S64TransferCapsule capsule, untouched;
    uint8_t original[S64_TRANSFER_CAPSULE_WIRE_SIZE];
    unsigned i;

    fixture(&control, &manifest, &journal);
    memset(&capsule, 0xa5, sizeof(capsule));
    untouched = capsule;

    assert(s64_transfer_capsule_store(&control, &journal, &manifest,
        (S64ModuleId)0, S64_MODULE_EDITOR));
    assert(s64_transfer_capsule_load(&control, &capsule));
    assert(capsule.source == (S64ModuleId)0);
    assert(capsule.destination == S64_MODULE_EDITOR);
    assert(capsule.journal.sequence == journal.sequence);
    assert(capsule.journal.session_id == journal.session_id);
    assert(capsule.module.id == S64_MODULE_EDITOR);
    assert(capsule.module.cache_state == S64_MODULE_CACHE_READY);
    assert(capsule.module.cache_address == manifest.modules[0].cache_address);
    assert(capsule.module.byte_length == manifest.modules[0].byte_length);
    assert(capsule.module.startup_address == manifest.modules[0].startup_address);

    memcpy(original, capsule_wire, sizeof(original));
    for (i = 0u; i < sizeof(capsule_wire); ++i) {
        memcpy(capsule_wire, original, sizeof(original));
        capsule_wire[i] ^= 1u;
        capsule = untouched;
        assert(!s64_transfer_capsule_load(&control, &capsule));
        assert(memcmp(&capsule, &untouched, sizeof(capsule)) == 0);
    }
    memcpy(capsule_wire, original, sizeof(original));

    journal.state = S64_SESSION_STATE_CLEAN;
    assert(!s64_transfer_capsule_store(&control, &journal, &manifest,
        (S64ModuleId)0, S64_MODULE_EDITOR));
    journal.state = S64_SESSION_STATE_ACTIVE;
    ++journal.session_id;
    assert(!s64_transfer_capsule_store(&control, &journal, &manifest,
        (S64ModuleId)0, S64_MODULE_EDITOR));
    --journal.session_id;

    manifest.modules[0].cache_state = S64_MODULE_CACHE_LOADED_UNVERIFIED;
    assert(!s64_transfer_capsule_store(&control, &journal, &manifest,
        (S64ModuleId)0, S64_MODULE_EDITOR));
    manifest.modules[0].cache_state = S64_MODULE_CACHE_READY;

    manifest.modules[0].startup_address = 0x7000u;
    assert(!s64_transfer_capsule_store(&control, &journal, &manifest,
        (S64ModuleId)0, S64_MODULE_EDITOR));
    manifest.modules[0].startup_address = 0x080du;

    manifest.modules[0].byte_length = 65536UL;
    assert(!s64_transfer_capsule_store(&control, &journal, &manifest,
        (S64ModuleId)0, S64_MODULE_EDITOR));
    manifest.modules[0].byte_length = 4096UL;

    manifest.modules[0].cache_address += 256UL;
    assert(!s64_transfer_capsule_store(&control, &journal, &manifest,
        (S64ModuleId)0, S64_MODULE_EDITOR));
    manifest.modules[0].cache_address -= 256UL;

    assert(!s64_transfer_capsule_store(&control, &journal, &manifest,
        (S64ModuleId)0, (S64ModuleId)0));
    assert(!s64_transfer_capsule_store(&control, &journal, &manifest,
        (S64ModuleId)0, S64_MODULE_PUBLISHER));
    assert(!s64_transfer_capsule_store(&control, &journal, &manifest,
        S64_MODULE_EDITOR, S64_MODULE_EDITOR));
    assert(s64_transfer_capsule_store(&control, &journal, &manifest,
        S64_MODULE_EDITOR, S64_MODULE_PUBLISHER));
    assert(s64_transfer_capsule_load(&control, &capsule));
    assert(capsule.source == S64_MODULE_EDITOR
        && capsule.destination == S64_MODULE_PUBLISHER);
    fail_store = 1u;
    assert(!s64_transfer_capsule_store(&control, &journal, &manifest,
        (S64ModuleId)0, S64_MODULE_EDITOR));
    fail_store = 0u;
    fail_load = 1u;
    assert(!s64_transfer_capsule_store(&control, &journal, &manifest,
        (S64ModuleId)0, S64_MODULE_EDITOR));
    fail_load = 0u;
    corrupt_read = 1u;
    assert(!s64_transfer_capsule_store(&control, &journal, &manifest,
        (S64ModuleId)0, S64_MODULE_EDITOR));
    corrupt_read = 0u;

    assert(s64_transfer_capsule_clear(&control));
    capsule = untouched;
    assert(!s64_transfer_capsule_load(&control, &capsule));
    assert(memcmp(&capsule, &untouched, sizeof(capsule)) == 0);

    puts("transfer capsule tests passed");
    return 0;
}
