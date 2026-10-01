#ifndef SEE64YOU_PLATFORM_SESSION_LIFECYCLE_H
#define SEE64YOU_PLATFORM_SESSION_LIFECYCLE_H

#include <stdint.h>

#include "platform/session_journal.h"
#include "platform/system_state.h"

typedef S64SessionJournalResult (*S64SessionLifecycleSaveFn)(
    void* context,
    S64SessionJournal* journal
);

typedef struct {
    S64SessionLifecycleSaveFn save;
    void* context;
} S64SessionLifecycleStore;

typedef enum {
    S64_SESSION_LIFECYCLE_OK = 0,
    S64_SESSION_LIFECYCLE_INVALID,
    S64_SESSION_LIFECYCLE_CAPTURE_FAILED,
    S64_SESSION_LIFECYCLE_JOURNAL_PREPARE_FAILED,
    S64_SESSION_LIFECYCLE_JOURNAL_ACTIVE_FAILED,
    S64_SESSION_LIFECYCLE_APPLY_FAILED,
    S64_SESSION_LIFECYCLE_RESTORE_FAILED,
    S64_SESSION_LIFECYCLE_JOURNAL_CLEAN_FAILED
} S64SessionLifecycleResult;

typedef struct {
    S64SystemState* system_state;
    S64SessionLifecycleStore store;
    S64SessionJournal journal;
    S64SystemResult system_result;
    S64SessionJournalResult journal_result;
    uint8_t active_record_written;
    uint8_t clean_record_written;
} S64SessionLifecycle;

void s64_session_lifecycle_init(
    S64SessionLifecycle* lifecycle,
    S64SystemState* system_state,
    const S64SessionLifecycleStore* store
);

S64SessionLifecycleResult s64_session_lifecycle_begin(
    S64SessionLifecycle* lifecycle,
    uint32_t session_id,
    uint16_t package_version,
    uint32_t recovery_ref,
    S64SystemMode initial_mode
);

S64SessionLifecycleResult s64_session_lifecycle_end(
    S64SessionLifecycle* lifecycle
);

#endif
