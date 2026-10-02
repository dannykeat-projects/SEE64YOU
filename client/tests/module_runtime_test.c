#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "platform/module_runtime.h"
#include "platform/module_session.h"
#include "platform/transfer_capsule.h"

static uint8_t control_wire[S64_REU_RUNTIME_CONTROL_CAPACITY];
static uint8_t session_wire[S64_MODULE_SESSION_WIRE_SIZE];
static uint8_t capsule_wire[S64_TRANSFER_CAPSULE_WIRE_SIZE];

typedef struct {
    uint8_t value;
    unsigned reads;
    unsigned writes;
} FakeRegister;

uint8_t s64_reu_store(
    uint8_t bank,
    uint16_t address,
    const void* source,
    uint16_t length
)
{
    if (bank == S64_REU_RUNTIME_CONTROL_BANK
        && address == S64_REU_RUNTIME_CONTROL_BASE
        && length <= sizeof(control_wire)) {
        memcpy(control_wire, source, length);
        return 1u;
    }
    if (bank == 5u && address == 0x0100u
        && length == sizeof(session_wire)) {
        memcpy(session_wire, source, length);
        return 1u;
    }
    if (bank == 5u && address == 0x0200u
        && length == sizeof(capsule_wire)) {
        memcpy(capsule_wire, source, length);
        return 1u;
    }
    return 0u;
}

uint8_t s64_reu_load(
    uint8_t bank,
    uint16_t address,
    void* destination,
    uint16_t length
)
{
    if (bank == S64_REU_RUNTIME_CONTROL_BANK
        && address == S64_REU_RUNTIME_CONTROL_BASE
        && length <= sizeof(control_wire)) {
        memcpy(destination, control_wire, length);
        return 1u;
    }
    if (bank == 5u && address == 0x0100u
        && length == sizeof(session_wire)) {
        memcpy(destination, session_wire, length);
        return 1u;
    }
    if (bank == 5u && address == 0x0200u
        && length == sizeof(capsule_wire)) {
        memcpy(destination, capsule_wire, length);
        return 1u;
    }
    return 0u;
}

static uint8_t fake_read(void* context)
{
    FakeRegister* reg = (FakeRegister*)context;
    ++reg->reads;
    return reg->value;
}

static void fake_write(void* context, uint8_t value)
{
    FakeRegister* reg = (FakeRegister*)context;
    ++reg->writes;
    reg->value = value;
}

static void prepare_shared(
    S64ReuRuntimeControl* control,
    S64ModuleManifest* manifest,
    S64SessionJournal* journal
)
{
    S64SystemSnapshot snapshot;
    unsigned i;

    memset(&snapshot, 0, sizeof(snapshot));
    snapshot.turbo_control_valid = 1u;
    snapshot.turbo_control = 0x25u;

    s64_reu_runtime_init(control, 0x0201u, 0x11223344UL);
    s64_module_manifest_init(manifest, control->app_version);
    assert(s64_module_manifest_allocate(manifest, control));
    assert(s64_module_session_reserve(control));
    assert(s64_transfer_capsule_reserve(control));

    for (i = 0u; i < S64_MODULE_COUNT; ++i) {
        manifest->modules[i].cache_state = S64_MODULE_CACHE_READY;
        manifest->modules[i].load_address = 0x0801u;
        manifest->modules[i].startup_address = 0x080du;
        manifest->modules[i].byte_length = 0x3002UL;
        manifest->modules[i].integrity = 0x12345670UL + i;
    }

    assert(s64_session_journal_prepare(
        journal,
        control->session_id,
        control->app_version,
        0x3344UL,
        &snapshot
    ));
    journal->sequence = 7UL;

    assert(s64_reu_runtime_store_verified(control));
    assert(s64_module_session_store_verified(control, journal, manifest));
    assert(s64_transfer_capsule_store(
        control,
        journal,
        manifest,
        S64_MODULE_EDITOR
    ));
}

int main(void)
{
    S64ReuRuntimeControl control;
    S64ModuleManifest manifest;
    S64SessionJournal journal;
    S64ModuleRuntime runtime;
    S64Dos dos;
    FakeRegister reg;
    unsigned reads_before, writes_before;

    memset(control_wire, 0, sizeof(control_wire));
    memset(session_wire, 0, sizeof(session_wire));
    memset(capsule_wire, 0, sizeof(capsule_wire));
    prepare_shared(&control, &manifest, &journal);

    memset(&dos, 0, sizeof(dos));
    memset(&reg, 0, sizeof(reg));
    reg.value = 0x25u;

    s64_module_runtime_init(
        &runtime,
        &dos,
        fake_read,
        fake_write,
        &reg
    );

    assert(!s64_module_runtime_adopt(
        &runtime,
        S64_MODULE_EDITOR,
        0x0201u,
        0u
    ));
    assert(!runtime.lifecycle.active_record_written);
    assert(!runtime.system_state.captured);

    reads_before = reg.reads;
    writes_before = reg.writes;
    assert(s64_module_runtime_adopt(
        &runtime,
        S64_MODULE_EDITOR,
        0x0201u,
        1u
    ));
    assert(runtime.current_module == S64_MODULE_EDITOR);
    assert(runtime.lifecycle.active_record_written);
    assert(runtime.lifecycle.journal.sequence == journal.sequence);
    assert(runtime.system_state.captured);
    assert(runtime.system_state.mode == S64_SYSTEM_MODE_TRANSITION);
    assert(runtime.system_state.snapshot.turbo_control == 0x25u);
    assert(reg.reads == reads_before + 1u);
    assert(reg.writes == writes_before);

    assert(s64_module_runtime_resume_interactive(&runtime) == S64_SYSTEM_OK);
    assert(runtime.system_state.mode == S64_SYSTEM_MODE_INTERACTIVE);

    assert(!s64_module_runtime_adopt(
        &runtime,
        S64_MODULE_EDITOR,
        0x0201u,
        1u
    ));

    puts("module runtime tests passed");
    return 0;
}
