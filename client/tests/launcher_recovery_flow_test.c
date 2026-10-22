#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "launcher/recovery_flow.h"

typedef struct {
    S64SessionJournalResult load_result;
    S64SessionJournalResult save_result;
    S64SessionJournal journal;
    unsigned int restores;
    unsigned int verifies;
    unsigned int saves;
    uint8_t restore_ok;
    uint8_t verify_ok;
} Mock;

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static S64SessionJournalResult load_journal(void* context, S64SessionJournal* journal)
{
    Mock* mock = (Mock*)context;
    if (mock->load_result == S64_SESSION_JOURNAL_OK) {
        memcpy(journal, &mock->journal, sizeof(*journal));
    }
    return mock->load_result;
}

static S64SessionJournalResult save_journal(void* context, S64SessionJournal* journal)
{
    Mock* mock = (Mock*)context;
    ++mock->saves;
    if (mock->save_result == S64_SESSION_JOURNAL_OK) {
        memcpy(&mock->journal, journal, sizeof(*journal));
    }
    return mock->save_result;
}

static uint8_t restore_state(void* context, const S64SystemSnapshot* snapshot)
{
    Mock* mock = (Mock*)context;
    ++mock->restores;
    require(snapshot->turbo_control_valid, "restore receives valid state");
    require(snapshot->turbo_control == 0x25u, "restore receives exact state");
    return mock->restore_ok;
}

static uint8_t verify_state(void* context, const S64SystemSnapshot* snapshot)
{
    Mock* mock = (Mock*)context;
    ++mock->verifies;
    require(snapshot->turbo_control == 0x25u, "verify receives exact state");
    return mock->verify_ok;
}

static Mock ready_mock(void)
{
    Mock mock;
    S64SystemSnapshot snapshot;
    memset(&mock, 0, sizeof(mock));
    memset(&snapshot, 0, sizeof(snapshot));
    mock.load_result = S64_SESSION_JOURNAL_OK;
    mock.save_result = S64_SESSION_JOURNAL_OK;
    mock.restore_ok = 1u;
    mock.verify_ok = 1u;
    snapshot.turbo_control = 0x25u;
    snapshot.turbo_control_valid = 1u;
    require(s64_session_journal_prepare(
        &mock.journal, 0x01020304UL, 0x0201u, 0UL, &snapshot
    ), "prepare interrupted session");
    mock.journal.sequence = 9UL;
    return mock;
}

static void init(
    S64SessionRecovery* recovery,
    S64SessionRecoveryBackend* backend,
    Mock* mock
)
{
    S64SessionRecoveryStore store;
    store.load = load_journal;
    store.save = save_journal;
    store.context = mock;
    s64_session_recovery_init(recovery, &store);
    backend->restore = restore_state;
    backend->verify = verify_state;
    backend->context = mock;
}

int main(void)
{
    S64SessionRecovery recovery;
    S64SessionRecoveryBackend backend;
    S64LauncherRecoveryResult result;
    Mock mock;

    memset(&mock, 0, sizeof(mock));
    mock.load_result = S64_SESSION_JOURNAL_NOT_FOUND;
    mock.save_result = S64_SESSION_JOURNAL_OK;
    mock.restore_ok = 1u;
    mock.verify_ok = 1u;
    init(&recovery, &backend, &mock);
    result = s64_launcher_recovery_run(&recovery, &backend);
    require(result == S64_LAUNCHER_RECOVERY_READY_NONE, "missing journal proceeds silently");
    require(strcmp(s64_launcher_recovery_message(result), "") == 0, "normal startup has no recovery message");

    mock = ready_mock();
    require(s64_session_journal_mark_clean(&mock.journal), "clean fixture");
    init(&recovery, &backend, &mock);
    result = s64_launcher_recovery_run(&recovery, &backend);
    require(result == S64_LAUNCHER_RECOVERY_READY_CLEAN, "clean prior session proceeds silently");
    require(mock.restores == 0u && mock.saves == 0u, "clean session requires no recovery writes");

    mock = ready_mock();
    init(&recovery, &backend, &mock);
    result = s64_launcher_recovery_run(&recovery, &backend);
    require(result == S64_LAUNCHER_RECOVERY_READY_RESTORED, "interrupted session is automatically restored");
    require(mock.restores == 1u && mock.verifies == 1u && mock.saves == 1u, "startup restores verifies then persists CLEAN");
    require(mock.journal.state == S64_SESSION_STATE_CLEAN, "automatic recovery closes prior session");
    require(strcmp(s64_launcher_recovery_message(result), "PREVIOUS SESSION RECOVERED") == 0, "recovered status is concise");

    mock = ready_mock();
    mock.restore_ok = 0u;
    init(&recovery, &backend, &mock);
    result = s64_launcher_recovery_run(&recovery, &backend);
    require(result == S64_LAUNCHER_RECOVERY_BLOCKED_RESTORE, "restore failure blocks launcher");
    require(s64_launcher_recovery_retryable(result), "restore failure offers retry");
    require(mock.saves == 0u && recovery.journal.state == S64_SESSION_STATE_ACTIVE, "restore failure never cleans journal");

    mock = ready_mock();
    mock.verify_ok = 0u;
    init(&recovery, &backend, &mock);
    result = s64_launcher_recovery_run(&recovery, &backend);
    require(result == S64_LAUNCHER_RECOVERY_BLOCKED_VERIFY, "verification failure blocks launcher");
    require(s64_launcher_recovery_retryable(result), "verification failure offers retry");
    require(mock.saves == 0u && recovery.journal.state == S64_SESSION_STATE_ACTIVE, "verification failure remains ACTIVE");

    mock = ready_mock();
    mock.save_result = S64_SESSION_JOURNAL_IO_ERROR;
    init(&recovery, &backend, &mock);
    result = s64_launcher_recovery_run(&recovery, &backend);
    require(result == S64_LAUNCHER_RECOVERY_BLOCKED_CLEAN_WRITE, "CLEAN write failure blocks launcher");
    require(s64_launcher_recovery_retryable(result), "CLEAN write failure offers retry");
    require(recovery.journal.state == S64_SESSION_STATE_ACTIVE, "failed CLEAN write preserves ACTIVE state");

    memset(&mock, 0, sizeof(mock));
    mock.load_result = S64_SESSION_JOURNAL_INVALID;
    init(&recovery, &backend, &mock);
    result = s64_launcher_recovery_run(&recovery, &backend);
    require(result == S64_LAUNCHER_RECOVERY_BLOCKED_INVALID_JOURNAL, "invalid journal requires diagnostics");
    require(!s64_launcher_recovery_retryable(result), "invalid data is not blindly retried");

    memset(&mock, 0, sizeof(mock));
    mock.load_result = S64_SESSION_JOURNAL_IO_ERROR;
    init(&recovery, &backend, &mock);
    result = s64_launcher_recovery_run(&recovery, &backend);
    require(result == S64_LAUNCHER_RECOVERY_BLOCKED_STORAGE, "storage failure blocks startup recovery");
    require(s64_launcher_recovery_retryable(result), "storage failure offers retry");

    require(s64_launcher_recovery_run(0, 0) == S64_LAUNCHER_RECOVERY_INVALID, "null recovery is rejected");
    puts("launcher recovery flow tests passed");
    return 0;
}
