#ifndef SEE64YOU_PLATFORM_SESSION_RECOVERY_H
#define SEE64YOU_PLATFORM_SESSION_RECOVERY_H

#include <stdint.h>

#include "platform/session_journal.h"
#include "platform/system_state.h"

typedef S64SessionJournalResult (*S64SessionRecoveryLoadFn)(
    void* context,
    S64SessionJournal* journal
);

typedef S64SessionJournalResult (*S64SessionRecoverySaveFn)(
    void* context,
    S64SessionJournal* journal
);

typedef struct {
    S64SessionRecoveryLoadFn load;
    S64SessionRecoverySaveFn save;
    void* context;
} S64SessionRecoveryStore;

typedef uint8_t (*S64SessionRecoveryRestoreFn)(
    void* context,
    const S64SystemSnapshot* snapshot
);

typedef uint8_t (*S64SessionRecoveryVerifyFn)(
    void* context,
    const S64SystemSnapshot* snapshot
);

typedef struct {
    S64SessionRecoveryRestoreFn restore;
    S64SessionRecoveryVerifyFn verify;
    void* context;
} S64SessionRecoveryBackend;

typedef enum {
    S64_SESSION_RECOVERY_NONE = 0,
    S64_SESSION_RECOVERY_CLEAN,
    S64_SESSION_RECOVERY_INTERRUPTED,
    S64_SESSION_RECOVERY_INVALID_JOURNAL,
    S64_SESSION_RECOVERY_STORAGE_ERROR
} S64SessionRecoveryDecision;

typedef enum {
    S64_SESSION_RECOVERY_RESOLVE_OK = 0,
    S64_SESSION_RECOVERY_RESOLVE_INVALID,
    S64_SESSION_RECOVERY_RESOLVE_NOT_INTERRUPTED,
    S64_SESSION_RECOVERY_RESOLVE_RESTORE_FAILED,
    S64_SESSION_RECOVERY_RESOLVE_VERIFY_FAILED,
    S64_SESSION_RECOVERY_RESOLVE_CLEAN_FAILED
} S64SessionRecoveryResolveResult;

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

S64SessionRecoveryResolveResult s64_session_recovery_resolve(
    S64SessionRecovery* recovery,
    const S64SessionRecoveryBackend* backend
);

#endif
