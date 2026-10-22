#include "platform/session_lifecycle.h"
#include "platform/session_lifecycle_internal.h"

static uint8_t mode_valid(S64SystemMode mode)
{
    return mode == S64_SYSTEM_MODE_INTERACTIVE
        || mode == S64_SYSTEM_MODE_CPU_INTENSIVE
        || mode == S64_SYSTEM_MODE_IO_SAFE
        || mode == S64_SYSTEM_MODE_TRANSITION;
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

    result = s64_session_lifecycle_save_clean(lifecycle);
    return result == S64_SESSION_LIFECYCLE_OK
        ? S64_SESSION_LIFECYCLE_APPLY_FAILED
        : result;
}
