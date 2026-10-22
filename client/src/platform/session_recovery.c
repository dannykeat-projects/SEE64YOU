#include "platform/session_recovery.h"

#include <string.h>

void s64_session_recovery_init(
    S64SessionRecovery* recovery,
    const S64SessionRecoveryStore* store
)
{
    if (recovery == 0) return;

    memset(recovery, 0, sizeof(*recovery));
    recovery->journal_result = S64_SESSION_JOURNAL_NOT_FOUND;
    recovery->decision = S64_SESSION_RECOVERY_NONE;
    if (store != 0) {
        recovery->store = *store;
    }
}

S64SessionRecoveryDecision s64_session_recovery_inspect(
    S64SessionRecovery* recovery
)
{
    if (recovery == 0 || recovery->store.load == 0) {
        return S64_SESSION_RECOVERY_STORAGE_ERROR;
    }

    memset(&recovery->journal, 0, sizeof(recovery->journal));
    memset(&recovery->restore_snapshot, 0, sizeof(recovery->restore_snapshot));
    recovery->restore_available = 0u;

    recovery->journal_result = recovery->store.load(
        recovery->store.context,
        &recovery->journal
    );

    if (recovery->journal_result == S64_SESSION_JOURNAL_NOT_FOUND) {
        recovery->decision = S64_SESSION_RECOVERY_NONE;
        return recovery->decision;
    }

    if (recovery->journal_result == S64_SESSION_JOURNAL_INVALID) {
        recovery->decision = S64_SESSION_RECOVERY_INVALID_JOURNAL;
        return recovery->decision;
    }

    if (recovery->journal_result != S64_SESSION_JOURNAL_OK) {
        recovery->decision = S64_SESSION_RECOVERY_STORAGE_ERROR;
        return recovery->decision;
    }

    if (recovery->journal.state == S64_SESSION_STATE_CLEAN) {
        recovery->decision = S64_SESSION_RECOVERY_CLEAN;
        return recovery->decision;
    }

    if (!s64_session_journal_is_unclean(&recovery->journal)
        || !s64_session_journal_restore_snapshot(
            &recovery->journal,
            &recovery->restore_snapshot
        )) {
        recovery->decision = S64_SESSION_RECOVERY_INVALID_JOURNAL;
        return recovery->decision;
    }

    recovery->restore_available =
        recovery->restore_snapshot.turbo_control_valid ? 1u : 0u;
    recovery->decision = S64_SESSION_RECOVERY_INTERRUPTED;
    return recovery->decision;
}

S64SessionRecoveryResolveResult s64_session_recovery_resolve(
    S64SessionRecovery* recovery,
    const S64SessionRecoveryBackend* backend
)
{
    S64SessionJournal pending;

    if (recovery == 0 || recovery->store.save == 0) {
        return S64_SESSION_RECOVERY_RESOLVE_INVALID;
    }
    if (recovery->decision != S64_SESSION_RECOVERY_INTERRUPTED) {
        return S64_SESSION_RECOVERY_RESOLVE_NOT_INTERRUPTED;
    }
    if (!s64_session_journal_is_unclean(&recovery->journal)) {
        return S64_SESSION_RECOVERY_RESOLVE_INVALID;
    }

    if (recovery->restore_available) {
        if (backend == 0 || backend->restore == 0) {
            return S64_SESSION_RECOVERY_RESOLVE_INVALID;
        }
        if (!backend->restore(
                backend->context,
                &recovery->restore_snapshot
            )) {
            return S64_SESSION_RECOVERY_RESOLVE_RESTORE_FAILED;
        }
        if (backend->verify != 0
            && !backend->verify(
                backend->context,
                &recovery->restore_snapshot
            )) {
            return S64_SESSION_RECOVERY_RESOLVE_VERIFY_FAILED;
        }
    }

    memcpy(&pending, &recovery->journal, sizeof(pending));
    if (!s64_session_journal_mark_clean(&pending)) {
        recovery->journal_result = S64_SESSION_JOURNAL_INVALID;
        return S64_SESSION_RECOVERY_RESOLVE_CLEAN_FAILED;
    }

    recovery->journal_result = recovery->store.save(
        recovery->store.context,
        &pending
    );
    if (recovery->journal_result != S64_SESSION_JOURNAL_OK) {
        return S64_SESSION_RECOVERY_RESOLVE_CLEAN_FAILED;
    }

    memcpy(&recovery->journal, &pending, sizeof(recovery->journal));
    memset(&recovery->restore_snapshot, 0, sizeof(recovery->restore_snapshot));
    recovery->restore_available = 0u;
    recovery->decision = S64_SESSION_RECOVERY_CLEAN;
    return S64_SESSION_RECOVERY_RESOLVE_OK;
}
