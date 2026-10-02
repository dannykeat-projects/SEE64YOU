#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/session_lifecycle.h"
#include "platform/turbo.h"

typedef struct {
    uint8_t turbo;
    uint8_t ignore_writes;
    uint8_t restore_ok;
    unsigned int reads;
    unsigned int writes;
    unsigned int saves;
    unsigned int active_saves;
    unsigned int clean_saves;
    unsigned int verifies;
    uint8_t clean_save_ok;
    S64SessionJournal saved;
} Mock;

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static uint8_t read_turbo(void* context)
{
    Mock* mock = (Mock*)context;
    ++mock->reads;
    return mock->turbo;
}

static void write_turbo(void* context, uint8_t value)
{
    Mock* mock = (Mock*)context;
    ++mock->writes;
    if (!mock->ignore_writes) mock->turbo = value;
}

static S64SessionJournalResult save_journal(
    void* context,
    S64SessionJournal* journal
)
{
    Mock* mock = (Mock*)context;
    ++mock->saves;
    if (journal->state == S64_SESSION_STATE_ACTIVE) {
        ++mock->active_saves;
        return S64_SESSION_JOURNAL_OK;
    }
    if (journal->state != S64_SESSION_STATE_CLEAN) {
        return S64_SESSION_JOURNAL_INVALID;
    }
    ++mock->clean_saves;
    if (!mock->clean_save_ok) return S64_SESSION_JOURNAL_IO_ERROR;
    mock->saved = *journal;
    return S64_SESSION_JOURNAL_OK;
}

static uint8_t restore_failing(void* context, const S64SystemSnapshot* snapshot)
{
    Mock* mock = (Mock*)context;
    (void)snapshot;
    return mock->restore_ok;
}

static uint8_t verify_noop(void* context, const S64SystemSnapshot* snapshot)
{
    Mock* mock = (Mock*)context;
    (void)snapshot;
    ++mock->verifies;
    return 1u;
}

static uint8_t apply_noop(void* context, S64SystemMode mode)
{
    (void)context;
    (void)mode;
    return 1u;
}

static void setup(
    Mock* mock,
    S64TurboBackend* turbo,
    S64SystemState* system,
    S64SessionLifecycle* lifecycle
)
{
    S64SystemBackend backend;
    S64SessionLifecycleStore store;

    s64_turbo_backend_init(turbo, read_turbo, write_turbo, mock);
    s64_turbo_backend_bind(&backend);
    s64_system_state_init(system, &backend, turbo);
    store.save = save_journal;
    store.context = mock;
    s64_session_lifecycle_init(lifecycle, system, &store);
}

static S64SessionJournal active_journal(uint8_t valid, uint8_t turbo)
{
    S64SessionJournal journal;
    memset(&journal, 0, sizeof(journal));
    journal.state = S64_SESSION_STATE_ACTIVE;
    journal.package_version = 0x0203u;
    journal.session_id = 0x12345678UL;
    journal.sequence = 17UL;
    journal.recovery_ref = 0x89abcdefUL;
    if (valid) {
        journal.restore_flags = S64_SESSION_JOURNAL_RESTORE_TURBO;
        journal.turbo_control = turbo;
    }
    return journal;
}

static void test_adopt_and_end(void)
{
    Mock mock;
    S64TurboBackend turbo;
    S64SystemState system;
    S64SessionLifecycle lifecycle;
    S64SessionJournal journal = active_journal(1u, 0x22u);

    memset(&mock, 0, sizeof(mock));
    mock.turbo = 0x22u;
    mock.clean_save_ok = 1u;
    setup(&mock, &turbo, &system, &lifecycle);

    require(s64_turbo_backend_adopt(&turbo, &(S64SystemSnapshot){0x22u, 1u}),
        "turbo adoption validates the original byte");
    require(mock.reads == 1u && mock.writes == 0u && mock.saves == 0u,
        "turbo adoption only reads and does not save");
    require(s64_session_lifecycle_adopt(&lifecycle, &journal)
        == S64_SESSION_LIFECYCLE_OK, "ACTIVE session is adopted");
    require(mock.writes == 0u && mock.saves == 0u && mock.active_saves == 0u,
        "session adoption performs no hardware writes or journal saves");
    require(lifecycle.journal.state == S64_SESSION_STATE_ACTIVE
        && lifecycle.journal.sequence == 17UL
        && lifecycle.journal.session_id == journal.session_id
        && lifecycle.journal.recovery_ref == journal.recovery_ref
        && lifecycle.journal.package_version == journal.package_version
        && lifecycle.journal.turbo_control == 0x22u,
        "adoption preserves ACTIVE metadata and original turbo snapshot");
    require(system.snapshot.turbo_control_valid
        && system.snapshot.turbo_control == 0x22u
        && system.mode == S64_SYSTEM_MODE_TRANSITION,
        "system state adopts the original snapshot at TRANSITION");

    require(s64_system_state_set_mode(&system, S64_SYSTEM_MODE_CPU_INTENSIVE)
        == S64_SYSTEM_OK, "CPU mode applies after adoption");
    require(mock.turbo == 0x2fu && mock.writes == 1u,
        "CPU mode temporarily accelerates while retaining control bits");
    require(s64_session_lifecycle_end(&lifecycle) == S64_SESSION_LIFECYCLE_OK,
        "adopted session restores and ends");
    require(mock.turbo == 0x22u && mock.writes == 2u && mock.reads == 3u,
        "end restores and verifies the exact original turbo byte");
    require(mock.saves == 1u && mock.active_saves == 0u
        && mock.clean_saves == 1u
        && mock.saved.state == S64_SESSION_STATE_CLEAN
        && mock.saved.sequence == journal.sequence
        && mock.saved.session_id == journal.session_id
        && mock.saved.recovery_ref == journal.recovery_ref
        && mock.saved.package_version == journal.package_version
        && mock.saved.turbo_control == journal.turbo_control,
        "only CLEAN is saved, preserving adopted journal metadata");
}

static void test_rejections_and_mismatch(void)
{
    Mock mock;
    S64TurboBackend turbo;
    S64SystemState system;
    S64SessionLifecycle lifecycle;
    S64SessionJournal journal = active_journal(1u, 0x24u);
    S64SystemSnapshot snapshot = {0x24u, 1u};

    memset(&mock, 0, sizeof(mock));
    mock.turbo = 0x25u;
    mock.clean_save_ok = 1u;
    setup(&mock, &turbo, &system, &lifecycle);
    require(!s64_turbo_backend_adopt(&turbo, &snapshot),
        "turbo mismatch rejects adoption");
    require(mock.reads == 1u && mock.writes == 0u && mock.saves == 0u,
        "mismatch rejection is read-only");

    mock.turbo = 0x24u;
    setup(&mock, &turbo, &system, &lifecycle);
    require(s64_session_lifecycle_adopt(&lifecycle, &journal)
        == S64_SESSION_LIFECYCLE_OK, "valid fixture adopts before double check");
    require(s64_session_lifecycle_adopt(&lifecycle, &journal)
        == S64_SESSION_LIFECYCLE_INVALID, "double session adoption rejects");
    require(s64_system_state_adopt(&system, &snapshot) == S64_SYSTEM_INVALID,
        "double system-state adoption rejects");

    setup(&mock, &turbo, &system, &lifecycle);
    journal.state = S64_SESSION_STATE_CLEAN;
    require(s64_session_lifecycle_adopt(&lifecycle, &journal)
        == S64_SESSION_LIFECYCLE_INVALID, "CLEAN journal adoption rejects");
    journal.state = S64_SESSION_STATE_ACTIVE;
    journal.sequence = 0UL;
    require(s64_session_lifecycle_adopt(&lifecycle, &journal)
        == S64_SESSION_LIFECYCLE_INVALID, "zero-sequence adoption rejects");
    require(s64_session_lifecycle_adopt(0, &journal)
        == S64_SESSION_LIFECYCLE_INVALID, "null lifecycle adoption rejects");
    require(s64_session_lifecycle_adopt(&lifecycle, 0)
        == S64_SESSION_LIFECYCLE_INVALID, "null journal adoption rejects");
    require(s64_system_state_adopt(0, &snapshot) == S64_SYSTEM_INVALID,
        "null system state adoption rejects");
    require(s64_system_state_adopt(&system, 0) == S64_SYSTEM_INVALID,
        "null system snapshot adoption rejects");
    snapshot.turbo_control_valid = 2u;
    require(s64_system_state_adopt(&system, &snapshot) == S64_SYSTEM_INVALID,
        "invalid system snapshot flags reject adoption");
    require(s64_turbo_backend_adopt(0, &snapshot) == 0u,
        "null turbo backend adoption rejects");
    require(s64_turbo_backend_adopt(&turbo, 0) == 0u,
        "null turbo snapshot adoption rejects");
    require(s64_turbo_backend_adopt(&turbo, &snapshot) == 0u,
        "invalid turbo snapshot flags reject adoption");
}

static void test_unavailable_turbo(void)
{
    Mock mock;
    S64TurboBackend turbo;
    S64SystemState system;
    S64SessionLifecycle lifecycle;
    S64SessionJournal journal = active_journal(0u, 0u);
    S64SystemSnapshot snapshot = {0u, 0u};

    memset(&mock, 0, sizeof(mock));
    mock.turbo = 0xffu;
    mock.clean_save_ok = 1u;
    setup(&mock, &turbo, &system, &lifecycle);
    require(s64_turbo_backend_adopt(&turbo, &snapshot),
        "unavailable turbo is accepted without claiming ownership");
    require(mock.reads == 0u && mock.writes == 0u,
        "unavailable adoption does not probe or write turbo");
    require(s64_session_lifecycle_adopt(&lifecycle, &journal)
        == S64_SESSION_LIFECYCLE_OK, "session without turbo snapshot adopts");
    require(s64_system_state_set_mode(&system, S64_SYSTEM_MODE_CPU_INTENSIVE)
        == S64_SYSTEM_OK, "unavailable turbo permits unmanaged CPU mode");
    require(mock.reads == 0u && mock.writes == 0u && mock.saves == 0u,
        "unavailable CPU mode does not probe or write turbo");
    require(s64_session_lifecycle_end(&lifecycle) == S64_SESSION_LIFECYCLE_OK,
        "unavailable turbo session can end");
    require(mock.reads == 0u && mock.writes == 0u && mock.saves == 1u
        && mock.saved.state == S64_SESSION_STATE_CLEAN,
        "unavailable setting remains unowned while CLEAN is saved");
}

static void test_retry_paths(void)
{
    Mock mock;
    S64TurboBackend turbo;
    S64SystemState system;
    S64SessionLifecycle lifecycle;
    S64SessionJournal journal = active_journal(1u, 0x31u);

    memset(&mock, 0, sizeof(mock));
    mock.turbo = 0x31u;
    mock.clean_save_ok = 1u;
    setup(&mock, &turbo, &system, &lifecycle);
    require(s64_turbo_backend_adopt(&turbo,
        &(S64SystemSnapshot){0x31u, 1u}), "retry fixture turbo adopts");
    require(s64_session_lifecycle_adopt(&lifecycle, &journal)
        == S64_SESSION_LIFECYCLE_OK, "retry fixture session adopts");
    require(s64_system_state_set_mode(&system, S64_SYSTEM_MODE_CPU_INTENSIVE)
        == S64_SYSTEM_OK, "retry fixture enters CPU mode");
    mock.ignore_writes = 1u;
    require(s64_session_lifecycle_end(&lifecycle)
        == S64_SESSION_LIFECYCLE_RESTORE_FAILED,
        "unrestorable turbo fails verification and blocks CLEAN");
    require(mock.clean_saves == 0u && lifecycle.active_record_written,
        "failed restore verification keeps session recoverable");
    mock.ignore_writes = 0u;
    require(s64_session_lifecycle_end(&lifecycle) == S64_SESSION_LIFECYCLE_OK,
        "failed restore verification can be retried");
    require(mock.turbo == 0x31u && mock.clean_saves == 1u,
        "restore retry verifies original before CLEAN");

    memset(&mock, 0, sizeof(mock));
    mock.turbo = 0x31u;
    mock.clean_save_ok = 0u;
    setup(&mock, &turbo, &system, &lifecycle);
    require(s64_turbo_backend_adopt(&turbo,
        &(S64SystemSnapshot){0x31u, 1u}), "CLEAN retry fixture turbo adopts");
    require(s64_session_lifecycle_adopt(&lifecycle, &journal)
        == S64_SESSION_LIFECYCLE_OK, "CLEAN retry fixture session adopts");
    require(s64_system_state_set_mode(&system, S64_SYSTEM_MODE_CPU_INTENSIVE)
        == S64_SYSTEM_OK, "CLEAN retry fixture enters CPU mode");
    require(s64_session_lifecycle_end(&lifecycle)
        == S64_SESSION_LIFECYCLE_JOURNAL_CLEAN_FAILED,
        "failed CLEAN save remains an error");
    require(mock.turbo == 0x31u && lifecycle.active_record_written
        && !lifecycle.clean_record_written,
        "failed CLEAN save keeps durable ACTIVE recoverable");
    mock.clean_save_ok = 1u;
    require(s64_session_lifecycle_end(&lifecycle) == S64_SESSION_LIFECYCLE_OK,
        "failed CLEAN save can be retried");
    require(mock.clean_saves == 2u && mock.active_saves == 0u
        && mock.saved.state == S64_SESSION_STATE_CLEAN,
        "CLEAN retry never rewrites ACTIVE");

    {
        S64SystemBackend backend;
        S64SessionLifecycleStore store;
        S64SystemState failed_system;
        S64SessionLifecycle failed_lifecycle;

        backend.capture = 0;
        backend.apply_mode = apply_noop;
        backend.restore = restore_failing;
        backend.verify = verify_noop;
        s64_system_state_init(&failed_system, &backend, &mock);
        store.save = save_journal;
        store.context = &mock;
        s64_session_lifecycle_init(&failed_lifecycle, &failed_system, &store);
        mock.restore_ok = 0u;
        require(s64_session_lifecycle_adopt(&failed_lifecycle, &journal)
            == S64_SESSION_LIFECYCLE_OK,
            "restore-failure fixture adopts ACTIVE session");
        require(s64_session_lifecycle_end(&failed_lifecycle)
            == S64_SESSION_LIFECYCLE_RESTORE_FAILED,
            "backend restore failure blocks CLEAN");
        require(mock.verifies == 0u && mock.clean_saves == 2u
            && failed_lifecycle.active_record_written,
            "restore failure remains ACTIVE and skips verification");
        mock.restore_ok = 1u;
        require(s64_session_lifecycle_end(&failed_lifecycle)
            == S64_SESSION_LIFECYCLE_OK,
            "backend restore failure can be retried");
    }
}

int main(void)
{
    test_adopt_and_end();
    test_rejections_and_mismatch();
    test_unavailable_turbo();
    test_retry_paths();
    puts("session adoption tests passed");
    return 0;
}
