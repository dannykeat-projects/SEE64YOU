#ifndef SEE64YOU_PLATFORM_JOURNAL_MIGRATION_H
#define SEE64YOU_PLATFORM_JOURNAL_MIGRATION_H

#include "platform/session_recovery.h"

/* Settings-owned staging service, not a startup hook. Resolve legacy ACTIVE
 * in its original directory, then seed a verified folder CLEAN if absent.
 * Never deletes root slots; backup/retirement is a separate acceptance step. */
S64SessionJournalResult s64_journal_migration_stage(
    S64Dos* dos,
    const S64SessionRecoveryBackend* backend
);

/* Separate retirement step. Both stores must be CLEAN; original root slot
 * bytes are retained as S64OLD0.DAT/S64OLD1.DAT in the installation folder.
 * Existing different backups block. Delete the newest CLEAN root slot last. */
S64SessionJournalResult s64_journal_migration_retire(S64Dos* dos);

#endif
