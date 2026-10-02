#include <conio.h>
#include <string.h>
#include "platform/reu.h"

/* Exercise the production startup/cleanup path, rather than a duplicate. */
#define main launcher_main
#include "launcher/main.c"
#undef main

/* Samples assume today's control/cache range [0x020000, 0x050180).
 * Every sentinel is backed up in C64 RAM and restored before returning. */
static const uint32_t addresses[] = {
    0x000000UL, 0x00efc0UL, 0x00f000UL, 0x00ffc0UL,
    0x010000UL, 0x01f000UL, 0x01fc00UL, 0x01ff00UL,
    0x0200c0UL, 0x050180UL, 0x060000UL, 0x070040UL,
    0x0ffff0UL, 0x1fffc0UL, 0x3fffc0UL, 0x7fffc0UL, 0xffffc0UL
};
#define SAMPLE_COUNT (sizeof(addresses) / sizeof(addresses[0]))
#define SAMPLE_SIZE 16u

static uint8_t originals[SAMPLE_COUNT][SAMPLE_SIZE];
static uint8_t sentinel[SAMPLE_SIZE], actual[SAMPLE_SIZE];
static S64SystemState outer;
static S64TurboBackend outer_turbo;
static S64SystemBackend outer_backend;
static uint8_t requested_turbo;
static S64ReuRuntimeControl gate_control;
static S64SessionJournal gate_journal;
static S64ModuleManifest gate_manifest;
static S64Dos gate_dos;
static S64LauncherRecoveryRuntime adoption_source, adoption_destination;
static unsigned adoption_dos_calls;

static S64UciResult adoption_execute(const S64UciCommandPart* parts,
    uint8_t count, S64UciReply* reply)
{
    ++adoption_dos_calls;
    return s64_uci_execute(parts, count, reply);
}

/* Simulates module replacement using independent initialized owners. Actual
 * overwrite-safe execution transfer is deliberately not part of this gate. */
static uint8_t adoption_gate(void)
{
    uint8_t passed = 0u, adopted = 0u;
    S64LauncherRecoveryRuntime* owner = &adoption_source;
    s64_launcher_recovery_runtime_init_hardware(&adoption_source);
    if (s64_launcher_recovery_runtime_run(&adoption_source)
            > S64_LAUNCHER_RECOVERY_READY_RESTORED) return 0u;
    if (s64_launcher_session_begin(&adoption_source, 1UL, 0x0201u, 0UL,
            S64_SYSTEM_MODE_INTERACTIVE) != S64_SESSION_LIFECYCLE_OK) goto cleanup;
    if (!s64_launcher_runtime_control_create(&adoption_source)
        || !s64_launcher_module_cache_plan(&adoption_source)
        || !s64_launcher_module_manifest_load(&adoption_source)
        || s64_system_state_set_mode(&adoption_source.system_state,
            S64_SYSTEM_MODE_IO_SAFE) != S64_SYSTEM_OK
        || !s64_launcher_module_session_clear(&adoption_source)
        || !s64_module_preload(&adoption_source.dos, &adoption_source.module_manifest)
        || !s64_module_cache_verify(&adoption_source.module_manifest,
            &adoption_source.system_state)
        || !s64_launcher_module_session_publish(&adoption_source)
        || s64_launcher_session_prepare_transition(&adoption_source)
            != S64_SYSTEM_OK) goto cleanup;
    s64_launcher_recovery_runtime_init_hardware(&adoption_destination);
    adoption_destination.dos.execute = adoption_execute;
    adoption_dos_calls = 0u;
    if (!s64_launcher_module_session_adopt(&adoption_destination, S64_MODULE_EDITOR,
            0x0201u, &adoption_source.lifecycle.journal)) goto cleanup;
    adopted = 1u;
    owner = &adoption_destination;
    if (adoption_dos_calls != 0u
        || owner->lifecycle.journal.sequence != adoption_source.lifecycle.journal.sequence
        || owner->system_state.snapshot.turbo_control != requested_turbo
        || s64_system_state_set_mode(&owner->system_state,
            S64_SYSTEM_MODE_CPU_INTENSIVE) != S64_SYSTEM_OK) goto cleanup;
    passed = 1u;
cleanup:
    if (owner->lifecycle.active_record_written
        && s64_launcher_session_end(owner) != S64_SESSION_LIFECYCLE_OK) passed = 0u;
    if (adopted && (owner->lifecycle.journal.state != S64_SESSION_STATE_CLEAN
        || !owner->system_state.restored
        || outer_turbo.read(outer_turbo.io_context) != requested_turbo)) passed = 0u;
    cputs(passed ? "ADOPTION PASS\r\n" : "ADOPTION FAIL\r\n");
    return passed;
}

/* Diagnostic-only preset; production modes keep their existing mapping. */
static uint8_t test_mode(void* context, S64SystemMode mode)
{
    S64TurboBackend* turbo = (S64TurboBackend*)context;
    (void)mode;
    requested_turbo = (turbo->captured_control & 0xf0u) | 2u;
    if (!turbo->controllable) return 0u;
    turbo->write(turbo->io_context, requested_turbo);
    if (turbo->read(turbo->io_context) == requested_turbo) return 1u;
    turbo->write(turbo->io_context, turbo->captured_control);
    return 0u;
}

int main(void)
{
    uint8_t i, j, prepared = 0u, passed = 1u;
    uint32_t address;
    int result;

    s64_turbo_backend_init_hardware(&outer_turbo);
    s64_turbo_backend_bind(&outer_backend);
    outer_backend.apply_mode = test_mode;
    s64_system_state_init(&outer, &outer_backend, &outer_turbo);
    if (s64_system_state_capture(&outer) != S64_SYSTEM_OK) return 1;

    for (i = 0u; i < SAMPLE_COUNT; ++i) {
        address = addresses[i];
        if (!s64_reu_load((uint8_t)(address >> 16), (uint16_t)address,
                originals[i], SAMPLE_SIZE)) {
            passed = 0u;
            break;
        }
        ++prepared;
        for (j = 0u; j < SAMPLE_SIZE; ++j)
            sentinel[j] = (uint8_t)(0xa5u ^ i ^ (j * 13u));
        if (!s64_reu_store((uint8_t)(address >> 16), (uint16_t)address,
                sentinel, SAMPLE_SIZE)) {
            passed = 0u;
            break;
        }
    }
    if (passed && s64_system_state_set_mode(&outer,
            S64_SYSTEM_MODE_CPU_INTENSIVE) == S64_SYSTEM_OK) {
        result = launcher_main();
        if (result != 0
            || outer_turbo.read(outer_turbo.io_context) != requested_turbo)
            passed = 0u;
        /* The persisted descriptors must not authorize a closed session. */
        s64_dos_init(&gate_dos, s64_uci_execute, S64_DOS_TARGET_PRIMARY);
        if (!s64_reu_runtime_load(&gate_control)
            || s64_session_journal_store_load(&gate_dos, &gate_journal)
                != S64_SESSION_JOURNAL_OK
            || gate_journal.state != S64_SESSION_STATE_CLEAN
            || s64_module_session_load(&gate_control, &gate_journal, &gate_manifest))
            passed = 0u;
        /* Simulate a leftover ACTIVE capsule from immediately before CLEAN.
         * This must fail because exit physically revoked the shared record. */
        gate_journal.state = S64_SESSION_STATE_ACTIVE;
        if (gate_journal.sequence != 0UL) --gate_journal.sequence;
        if (s64_module_session_load(&gate_control, &gate_journal, &gate_manifest))
            passed = 0u;
        if (passed && !adoption_gate()) passed = 0u;
        for (i = 0u; i < SAMPLE_COUNT; ++i) {
            address = addresses[i];
            if (!s64_reu_load((uint8_t)(address >> 16), (uint16_t)address,
                    actual, SAMPLE_SIZE)) passed = 0u;
            for (j = 0u; j < SAMPLE_SIZE; ++j) {
                if (actual[j] != (uint8_t)(0xa5u ^ i ^ (j * 13u)))
                    passed = 0u;
            }
        }
    } else passed = 0u;

    for (i = 0u; i < prepared; ++i) {
        address = addresses[i];
        if (!s64_reu_store((uint8_t)(address >> 16), (uint16_t)address,
                originals[i], SAMPLE_SIZE)
            || !s64_reu_load((uint8_t)(address >> 16), (uint16_t)address,
                actual, SAMPLE_SIZE)
            || memcmp(actual, originals[i], SAMPLE_SIZE) != 0) passed = 0u;
    }
    if (s64_system_state_restore(&outer) != S64_SYSTEM_OK) passed = 0u;
    cputs(passed ? "CACHE GATE PASS\r\n" : "CACHE GATE FAIL\r\n");
    cprintf("START %u INNER %u FINAL %u\r\n", outer.snapshot.turbo_control,
        requested_turbo, outer_turbo.read(outer_turbo.io_context));
    return passed ? 0 : 1;
}
