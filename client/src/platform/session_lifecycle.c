#include "platform/session_lifecycle.h"
#include "platform/session_lifecycle_internal.h"

#include <string.h>

void s64_session_lifecycle_init(
    S64SessionLifecycle* lifecycle,
    S64SystemState* system_state,
    const S64SessionLifecycleStore* store
)
{
    if (lifecycle == 0) return;

    memset(lifecycle, 0, sizeof(*lifecycle));
    lifecycle->system_state = system_state;
    lifecycle->system_result = S64_SYSTEM_OK;
    lifecycle->journal_result = S64_SESSION_JOURNAL_OK;
    if (store != 0) {
        lifecycle->store = *store;
    }
}

S64SessionLifecycleResult s64_session_lifecycle_save_clean(
    S64SessionLifecycle* lifecycle
)
{
    if (!s64_session_journal_mark_clean(&lifecycle->journal)) {
        lifecycle->journal_result = S64_SESSION_JOURNAL_INVALID;
        return S64_SESSION_LIFECYCLE_JOURNAL_CLEAN_FAILED;
    }

    lifecycle->journal_result = lifecycle->store.save(
        lifecycle->store.context,
        &lifecycle->journal
    );
    if (lifecycle->journal_result != S64_SESSION_JOURNAL_OK) {
        return S64_SESSION_LIFECYCLE_JOURNAL_CLEAN_FAILED;
    }

    lifecycle->active_record_written = 0u;
    lifecycle->clean_record_written = 1u;
    return S64_SESSION_LIFECYCLE_OK;
}

S64SessionLifecycleResult s64_session_lifecycle_adopt(
    S64SessionLifecycle* lifecycle, const S64SessionJournal* journal)
{
    S64SystemSnapshot snapshot;
    if (lifecycle == 0 || journal == 0 || lifecycle->system_state == 0
        || lifecycle->store.save == 0 || lifecycle->active_record_written
        || lifecycle->clean_record_written || journal->state != S64_SESSION_STATE_ACTIVE
        || journal->sequence == 0UL
        || !s64_session_journal_restore_snapshot(journal, &snapshot))
        return S64_SESSION_LIFECYCLE_INVALID;
    lifecycle->system_result = s64_system_state_adopt(lifecycle->system_state, &snapshot);
    if (lifecycle->system_result != S64_SYSTEM_OK) return S64_SESSION_LIFECYCLE_INVALID;
    lifecycle->journal = *journal;
    lifecycle->active_record_written = 1u;
    return S64_SESSION_LIFECYCLE_OK;
}

S64SessionLifecycleResult s64_session_lifecycle_end(
    S64SessionLifecycle* lifecycle
)
{
    if (lifecycle == 0
        || lifecycle->system_state == 0
        || lifecycle->store.save == 0
        || !lifecycle->active_record_written) {
        return S64_SESSION_LIFECYCLE_INVALID;
    }

    lifecycle->system_result = s64_system_state_restore(
        lifecycle->system_state
    );
    if (lifecycle->system_result != S64_SYSTEM_OK) {
        return S64_SESSION_LIFECYCLE_RESTORE_FAILED;
    }

    return s64_session_lifecycle_save_clean(lifecycle);
}
