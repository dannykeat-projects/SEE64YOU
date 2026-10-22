#include "platform/session_journal.h"
#include <string.h>

uint8_t s64_session_journal_prepare(
    S64SessionJournal* journal,
    uint32_t session_id,
    uint16_t package_version,
    uint32_t recovery_ref,
    const S64SystemSnapshot* snapshot
)
{
    if (journal == 0 || session_id == 0UL) return 0u;

    memset(journal, 0, sizeof(*journal));
    journal->state = S64_SESSION_STATE_ACTIVE;
    journal->package_version = package_version;
    journal->session_id = session_id;
    journal->recovery_ref = recovery_ref;

    if (snapshot != 0 && snapshot->turbo_control_valid) {
        journal->restore_flags |= S64_SESSION_JOURNAL_RESTORE_TURBO;
        journal->turbo_control = snapshot->turbo_control;
    }
    return 1u;
}
