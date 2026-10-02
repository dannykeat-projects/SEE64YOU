#include "platform/session_lifecycle.h"

#include <string.h>

static uint8_t mode_valid(S64SystemMode mode)
{
    return mode == S64_SYSTEM_MODE_INTERACTIVE
        || mode == S64_SYSTEM_MODE_CPU_INTENSIVE
        || mode == S64_SYSTEM_MODE_IO_SAFE
        || mode == S64_SYSTEM_MODE_TRANSITION;
}

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

static S64SessionLifecycleResult save_active(
    S64SessionLifecycle* lifecycle
)
{
    lifecycle->journal_result = lifecycle->store.save(
        lifecycle->store.context,
        &lifecycle->journal
    );
    if (lifecycle->journal_result != S64_SESSION_JOURNAL_OK) {
        return S64_SESSION_LIFECYCLE_JOURNAL_ACTIVE_FAILED;
    }

    lifecycle->active_record_written = 1u;
    lifecycle->clean_record_written = 0u;
    return S64_SESSION_LIFECYCLE_OK;
}

static S64SessionLifecycleResult save_clean(
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

S64SessionLifecycleResult s64_session_lifecycle_begin(
    S64SessionLifecycle* lifecycle,
    uint32_t session_id,
    uint16_t package_version,
    uint32_t recovery_ref,
    S64SystemMode initial_mode
)
{
    S64SessionLifecycleResult result;

    if (lifecycle == 0
        || lifecycle->system_state == 0
        || lifecycle->store.save == 0
        || session_id == 0UL
        || !mode_valid(initial_mode)
        || lifecycle->active_record_written) {
        return S64_SESSION_LIFECYCLE_INVALID;
    }

    lifecycle->clean_record_written = 0u;
    lifecycle->system_result = s64_system_state_capture(
        lifecycle->system_state
    );
    if (lifecycle->system_result != S64_SYSTEM_OK) {
        return S64_SESSION_LIFECYCLE_CAPTURE_FAILED;
    }

    if (!s64_session_journal_prepare(
            &lifecycle->journal,
            session_id,
            package_version,
            recovery_ref,
            &lifecycle->system_state->snapshot
        )) {
        lifecycle->system_result = s64_system_state_restore(
            lifecycle->system_state
        );
        return S64_SESSION_LIFECYCLE_JOURNAL_PREPARE_FAILED;
    }

    result = save_active(lifecycle);
    if (result != S64_SESSION_LIFECYCLE_OK) {
        lifecycle->system_result = s64_system_state_restore(
            lifecycle->system_state
        );
        return result;
    }

    lifecycle->system_result = s64_system_state_set_mode(
        lifecycle->system_state,
        initial_mode
    );
    if (lifecycle->system_result == S64_SYSTEM_OK) {
        return S64_SESSION_LIFECYCLE_OK;
    }

    /*
     * A failed apply must preserve the backend's previously captured state.
     * Restore still runs so a backend that partially modified state can
     * report/repair it if the manager recorded a modification.
     */
    lifecycle->system_result = s64_system_state_restore(
        lifecycle->system_state
    );
    if (lifecycle->system_result != S64_SYSTEM_OK) {
        return S64_SESSION_LIFECYCLE_RESTORE_FAILED;
    }

    result = save_clean(lifecycle);
    return result == S64_SESSION_LIFECYCLE_OK
        ? S64_SESSION_LIFECYCLE_APPLY_FAILED
        : result;
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

    return save_clean(lifecycle);
}
