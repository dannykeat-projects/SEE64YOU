#ifndef SEE64YOU_PLATFORM_SESSION_RECOVERY_H
#define SEE64YOU_PLATFORM_SESSION_RECOVERY_H

#include <stdint.h>

#include "platform/session_journal.h"
#include "platform/system_state.h"

typedef S64SessionJournalResult (*S64SessionRecoveryLoadFn)(
    void* context,
    S64SessionJournal* journal
);

typedef struct {
    S64SessionRecoveryLoadFn load;
    void* context;
} S64SessionRecoveryStore;

typedef enum {
    S64_SESSION_RECOVERY_NONE = 0,
    S64_SESSION_RECOVERY_CLEAN,
    S64_SESSION_RECOVERY_INTERRUPTED,
    S64_SESSION_RECOVERY_INVALID_JOURNAL,
    S64_SESSION_RECOVERY_STORAGE_ERROR
} S64SessionRecoveryDecision;

typedef struct {
    S64SessionRecoveryStore store;
    S64SessionJournal journal;
    S64SystemSnapshot restore_snapshot;
    S64SessionJournalResult journal_result;
    S64SessionRecoveryDecision decision;
    uint8_t restore_available;
} S64SessionRecovery;

void s64_session_recovery_init(
    S64SessionRecovery* recovery,
    const S64SessionRecoveryStore* store
);

S64SessionRecoveryDecision s64_session_recovery_inspect(
    S64SessionRecovery* recovery
);

#endif
