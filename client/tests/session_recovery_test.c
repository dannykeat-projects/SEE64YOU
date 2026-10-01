#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/session_recovery.h"

typedef struct {
    S64SessionJournalResult result;
    S64SessionJournal journal;
    unsigned int loads;
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

    if (store->result == S64_SESSION_JOURNAL_OK) {
        memcpy(journal, &store->journal, sizeof(*journal));
    }
    return store->result;
}

static void init_recovery(
    S64SessionRecovery* recovery,
    MockStore* mock
)
{
    S64SessionRecoveryStore store;
    store.load = load_journal;
    store.context = mock;
    s64_session_recovery_init(recovery, &store);
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
    mock.result = S64_SESSION_JOURNAL_NOT_FOUND;
    init_recovery(&recovery, &mock);
    require(
        s64_session_recovery_inspect(&recovery)
            == S64_SESSION_RECOVERY_NONE,
        "missing journal means no prior managed session"
    );
    require(!recovery.restore_available, "missing journal has no restore state");

    memset(&mock, 0, sizeof(mock));
    mock.result = S64_SESSION_JOURNAL_OK;
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

    memset(&mock, 0, sizeof(mock));
    mock.result = S64_SESSION_JOURNAL_OK;
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

    memset(&mock, 0, sizeof(mock));
    mock.result = S64_SESSION_JOURNAL_OK;
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

    memset(&mock, 0, sizeof(mock));
    mock.result = S64_SESSION_JOURNAL_INVALID;
    init_recovery(&recovery, &mock);
    require(
        s64_session_recovery_inspect(&recovery)
            == S64_SESSION_RECOVERY_INVALID_JOURNAL,
        "invalid journal is surfaced separately"
    );
    require(!recovery.restore_available,
        "invalid journal never exposes restoration");

    memset(&mock, 0, sizeof(mock));
    mock.result = S64_SESSION_JOURNAL_IO_ERROR;
    init_recovery(&recovery, &mock);
    require(
        s64_session_recovery_inspect(&recovery)
            == S64_SESSION_RECOVERY_STORAGE_ERROR,
        "storage failure is distinct from absent journal"
    );
    require(recovery.journal_result == S64_SESSION_JOURNAL_IO_ERROR,
        "exact storage failure remains available to launcher");

    memset(&mock, 0, sizeof(mock));
    mock.result = S64_SESSION_JOURNAL_VERIFY_ERROR;
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

    puts("session recovery tests passed");
    return 0;
}
