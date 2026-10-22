#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/session_recovery.h"

typedef struct {
    S64SessionJournalResult load_result;
    S64SessionJournalResult save_result;
    S64SessionJournal journal;
    S64SessionJournal saved;
    unsigned int loads;
    unsigned int saves;
    unsigned int restores;
    unsigned int verifies;
    uint8_t restore_ok;
    uint8_t verify_ok;
    S64SystemSnapshot restored_snapshot;
} MockStore;

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static S64SessionJournalResult load_journal(
    void* context,
    S64SessionJournal* journal
)
{
    MockStore* store = (MockStore*)context;
    ++store->loads;

    if (store->load_result == S64_SESSION_JOURNAL_OK) {
        memcpy(journal, &store->journal, sizeof(*journal));
    }
    return store->load_result;
}

static S64SessionJournalResult save_journal(
    void* context,
    S64SessionJournal* journal
)
{
    MockStore* store = (MockStore*)context;
    ++store->saves;

    if (store->save_result == S64_SESSION_JOURNAL_OK) {
        memcpy(&store->saved, journal, sizeof(store->saved));
    }
    return store->save_result;
}

static uint8_t restore_snapshot(
    void* context,
    const S64SystemSnapshot* snapshot
)
{
    MockStore* store = (MockStore*)context;
    ++store->restores;
    memcpy(&store->restored_snapshot, snapshot, sizeof(*snapshot));
    return store->restore_ok;
}

static uint8_t verify_snapshot(
    void* context,
    const S64SystemSnapshot* snapshot
)
{
    MockStore* store = (MockStore*)context;
    ++store->verifies;
    require(snapshot->turbo_control_valid,
        "verify sees explicit restore-valid turbo");
    require(snapshot->turbo_control == store->restored_snapshot.turbo_control,
        "verify sees same snapshot as restore");
    return store->verify_ok;
}

static void init_recovery(
    S64SessionRecovery* recovery,
    MockStore* mock
)
{
    S64SessionRecoveryStore store;
    store.load = load_journal;
    store.save = save_journal;
    store.context = mock;
    s64_session_recovery_init(recovery, &store);
}

static S64SessionRecoveryBackend recovery_backend(MockStore* mock)
{
    S64SessionRecoveryBackend backend;
    backend.restore = restore_snapshot;
    backend.verify = verify_snapshot;
    backend.context = mock;
    return backend;
}

static MockStore ready_mock(void)
{
    MockStore mock;
    memset(&mock, 0, sizeof(mock));
    mock.load_result = S64_SESSION_JOURNAL_OK;
    mock.save_result = S64_SESSION_JOURNAL_OK;
    mock.restore_ok = 1u;
    mock.verify_ok = 1u;
    return mock;
}

static void prepare_active(
    S64SessionJournal* journal,
    uint8_t restorable
)
{
    S64SystemSnapshot snapshot;

    memset(&snapshot, 0, sizeof(snapshot));
    if (restorable) {
        snapshot.turbo_control = 0x25u;
        snapshot.turbo_control_valid = 1u;
    }

    require(
        s64_session_journal_prepare(
            journal,
            0x11223344UL,
            0x0201u,
            0x55667788UL,
            &snapshot
        ),
        "prepare active fixture"
    );
    journal->sequence = 7UL;
}

int main(void)
{
    S64SessionRecovery recovery;
    MockStore mock;

    memset(&mock, 0, sizeof(mock));
    mock.load_result = S64_SESSION_JOURNAL_NOT_FOUND;
    init_recovery(&recovery, &mock);
    require(
        s64_session_recovery_inspect(&recovery)
            == S64_SESSION_RECOVERY_NONE,
        "missing journal means no prior managed session"
    );
    require(!recovery.restore_available, "missing journal has no restore state");

    mock = ready_mock();
    prepare_active(&mock.journal, 1u);
    require(s64_session_journal_mark_clean(&mock.journal),
        "mark clean fixture");
    init_recovery(&recovery, &mock);
    require(
        s64_session_recovery_inspect(&recovery)
            == S64_SESSION_RECOVERY_CLEAN,
        "clean journal requires no interrupted-session recovery"
    );
    require(!recovery.restore_available,
        "clean journal never exposes restore action");
    require(recovery.journal.session_id == 0x11223344UL
        && recovery.journal.package_version == 0x0201u,
        "clean metadata remains available to launcher diagnostics");

    mock = ready_mock();
    prepare_active(&mock.journal, 1u);
    init_recovery(&recovery, &mock);
    require(
        s64_session_recovery_inspect(&recovery)
            == S64_SESSION_RECOVERY_INTERRUPTED,
        "active journal is interrupted session"
    );
    require(recovery.restore_available,
        "active journal exposes explicitly captured restore state");
    require(recovery.restore_snapshot.turbo_control_valid
        && recovery.restore_snapshot.turbo_control == 0x25u,
        "recovery snapshot contains exact captured turbo byte");
    require(recovery.journal.sequence == 7UL
        && recovery.journal.recovery_ref == 0x55667788UL,
        "launcher receives recovery metadata from active record");

    {
        S64SessionRecoveryBackend backend = recovery_backend(&mock);
        require(
            s64_session_recovery_resolve(&recovery, &backend)
                == S64_SESSION_RECOVERY_RESOLVE_OK,
            "interrupted state restores and resolves cleanly"
        );
    }
    require(mock.restores == 1u && mock.verifies == 1u,
        "restore is verified before cleanup");
    require(mock.restored_snapshot.turbo_control_valid
        && mock.restored_snapshot.turbo_control == 0x25u,
        "restore receives exact captured turbo");
    require(mock.saves == 1u
        && mock.saved.state == S64_SESSION_STATE_CLEAN,
        "successful recovery persists CLEAN supersession");
    require(mock.saved.sequence == 7UL,
        "CLEAN supersession advances through journal store sequence handling");
    require(recovery.decision == S64_SESSION_RECOVERY_CLEAN
        && !recovery.restore_available
        && !recovery.restore_snapshot.turbo_control_valid,
        "resolved recovery no longer exposes stale restore data");

    mock = ready_mock();
    prepare_active(&mock.journal, 0u);
    init_recovery(&recovery, &mock);
    require(
        s64_session_recovery_inspect(&recovery)
            == S64_SESSION_RECOVERY_INTERRUPTED,
        "active session is still interrupted without restorable globals"
    );
    require(!recovery.restore_available,
        "uncaptured machine state is not invented for recovery");
    require(!recovery.restore_snapshot.turbo_control_valid,
        "restore snapshot valid bits remain clear");
    require(
        s64_session_recovery_resolve(&recovery, 0)
            == S64_SESSION_RECOVERY_RESOLVE_OK,
        "interrupted session with no captured globals still closes cleanly"
    );
    require(mock.restores == 0u && mock.verifies == 0u,
        "no hardware restoration runs without restore-valid state");
    require(mock.saves == 1u
        && mock.saved.state == S64_SESSION_STATE_CLEAN,
        "no-state recovery still supersedes ACTIVE journal");

    mock = ready_mock();
    prepare_active(&mock.journal, 1u);
    init_recovery(&recovery, &mock);
    require(
        s64_session_recovery_inspect(&recovery)
            == S64_SESSION_RECOVERY_INTERRUPTED,
        "restore-failure fixture is interrupted"
    );
    {
        S64SessionRecoveryBackend backend = recovery_backend(&mock);
        mock.restore_ok = 0u;
        require(
            s64_session_recovery_resolve(&recovery, &backend)
                == S64_SESSION_RECOVERY_RESOLVE_RESTORE_FAILED,
            "failed restoration blocks CLEAN record"
        );
    }
    require(mock.saves == 0u && mock.verifies == 0u,
        "failed restoration neither verifies nor cleans");
    require(recovery.decision == S64_SESSION_RECOVERY_INTERRUPTED
        && recovery.journal.state == S64_SESSION_STATE_ACTIVE,
        "failed restoration remains retryable and ACTIVE");

    mock = ready_mock();
    prepare_active(&mock.journal, 1u);
    init_recovery(&recovery, &mock);
    require(
        s64_session_recovery_inspect(&recovery)
            == S64_SESSION_RECOVERY_INTERRUPTED,
        "verify-failure fixture is interrupted"
    );
    {
        S64SessionRecoveryBackend backend = recovery_backend(&mock);
        mock.verify_ok = 0u;
        require(
            s64_session_recovery_resolve(&recovery, &backend)
                == S64_SESSION_RECOVERY_RESOLVE_VERIFY_FAILED,
            "failed verification blocks CLEAN record"
        );
    }
    require(mock.restores == 1u && mock.verifies == 1u && mock.saves == 0u,
        "verification failure preserves durable ACTIVE state");
    require(recovery.decision == S64_SESSION_RECOVERY_INTERRUPTED
        && recovery.journal.state == S64_SESSION_STATE_ACTIVE,
        "verification failure remains retryable");

    mock = ready_mock();
    prepare_active(&mock.journal, 1u);
    init_recovery(&recovery, &mock);
    require(
        s64_session_recovery_inspect(&recovery)
            == S64_SESSION_RECOVERY_INTERRUPTED,
        "clean-write-failure fixture is interrupted"
    );
    {
        S64SessionRecoveryBackend backend = recovery_backend(&mock);
        mock.save_result = S64_SESSION_JOURNAL_IO_ERROR;
        require(
            s64_session_recovery_resolve(&recovery, &backend)
                == S64_SESSION_RECOVERY_RESOLVE_CLEAN_FAILED,
            "failed CLEAN persistence is reported"
        );
        require(recovery.journal_result == S64_SESSION_JOURNAL_IO_ERROR,
            "exact CLEAN persistence failure remains available");
        require(recovery.decision == S64_SESSION_RECOVERY_INTERRUPTED
            && recovery.journal.state == S64_SESSION_STATE_ACTIVE,
            "failed CLEAN write does not mutate inspected ACTIVE state");

        mock.save_result = S64_SESSION_JOURNAL_OK;
        require(
            s64_session_recovery_resolve(&recovery, &backend)
                == S64_SESSION_RECOVERY_RESOLVE_OK,
            "CLEAN persistence failure can be retried"
        );
    }
    require(mock.restores == 2u && mock.verifies == 2u && mock.saves == 2u,
        "retry safely replays idempotent restore/verify before CLEAN");
    require(recovery.decision == S64_SESSION_RECOVERY_CLEAN,
        "retry resolves recovery only after durable CLEAN write");

    mock = ready_mock();
    prepare_active(&mock.journal, 1u);
    init_recovery(&recovery, &mock);
    require(
        s64_session_recovery_inspect(&recovery)
            == S64_SESSION_RECOVERY_INTERRUPTED,
        "missing-backend fixture is interrupted"
    );
    require(
        s64_session_recovery_resolve(&recovery, 0)
            == S64_SESSION_RECOVERY_RESOLVE_INVALID,
        "restorable state requires a restore backend"
    );
    require(mock.saves == 0u,
        "missing backend cannot accidentally clean interrupted state");

    memset(&mock, 0, sizeof(mock));
    mock.load_result = S64_SESSION_JOURNAL_INVALID;
    init_recovery(&recovery, &mock);
    require(
        s64_session_recovery_inspect(&recovery)
            == S64_SESSION_RECOVERY_INVALID_JOURNAL,
        "invalid journal is surfaced separately"
    );
    require(!recovery.restore_available,
        "invalid journal never exposes restoration");
    require(
        s64_session_recovery_resolve(&recovery, 0)
            == S64_SESSION_RECOVERY_RESOLVE_NOT_INTERRUPTED,
        "invalid journal cannot be resolved as interrupted state"
    );

    memset(&mock, 0, sizeof(mock));
    mock.load_result = S64_SESSION_JOURNAL_IO_ERROR;
    init_recovery(&recovery, &mock);
    require(
        s64_session_recovery_inspect(&recovery)
            == S64_SESSION_RECOVERY_STORAGE_ERROR,
        "storage failure is distinct from absent journal"
    );
    require(recovery.journal_result == S64_SESSION_JOURNAL_IO_ERROR,
        "exact storage failure remains available to launcher");

    memset(&mock, 0, sizeof(mock));
    mock.load_result = S64_SESSION_JOURNAL_VERIFY_ERROR;
    init_recovery(&recovery, &mock);
    require(
        s64_session_recovery_inspect(&recovery)
            == S64_SESSION_RECOVERY_STORAGE_ERROR,
        "unexpected persistence verification error fails closed"
    );

    s64_session_recovery_init(&recovery, 0);
    require(
        s64_session_recovery_inspect(&recovery)
            == S64_SESSION_RECOVERY_STORAGE_ERROR,
        "missing load backend fails closed"
    );
    require(
        s64_session_recovery_resolve(&recovery, 0)
            == S64_SESSION_RECOVERY_RESOLVE_INVALID,
        "missing save backend cannot resolve recovery"
    );

    puts("session recovery tests passed");
    return 0;
}
