#include <stdint.h>

#include "platform/journal_migration.h"
#include "platform/package_version.h"
#include "platform/turbo.h"
#include "platform/uci.h"

/* Disposable, backed-up journal fixture only. Never run on user journals.
 * Default seeds root ACTIVE with the actual entry turbo byte. SEED=0 instead
 * stages the supplied files, for collision/refusal checks. RETIRE=1 instead
 * retires supplied CLEAN fixtures, keeping verified S64OLD backups. $C040:
 * $A5 = success, $5A = refusal, $C041 = journal result; $C042/$C043 = turbo.
 * Returns to BASIC. The external harness owns backup and exact SD restoration. */
#ifndef S64_MIGRATION_PROBE_SEED
#define S64_MIGRATION_PROBE_SEED 1
#endif
#ifndef S64_MIGRATION_PROBE_RETIRE
#define S64_MIGRATION_PROBE_RETIRE 0
#endif
#define REPORT ((volatile uint8_t*)0xC040u)

int main(void)
{
    S64Dos dos;
    S64TurboBackend turbo;
    S64SystemBackend system;
    S64SessionRecoveryBackend backend;
    S64SessionJournalResult result;
#if S64_MIGRATION_PROBE_SEED
    S64SessionJournal journal;
    S64SystemSnapshot snapshot;
#endif

    REPORT[0] = 0u;
    s64_dos_init(&dos, s64_uci_execute, S64_DOS_TARGET_PRIMARY);
    s64_turbo_backend_init_hardware(&turbo);
    s64_turbo_backend_bind_adopt(&system);
    backend.restore = system.restore;
    backend.verify = system.verify;
    backend.context = &turbo;
    REPORT[2] = turbo.read(turbo.io_context);
#if S64_MIGRATION_PROBE_SEED
    snapshot.turbo_control = REPORT[2];
    snapshot.turbo_control_valid = 1u;
    if (!s64_session_journal_prepare(&journal, 0x643a1101UL,
            S64_PACKAGE_VERSION, 0UL, &snapshot)) return 1;
    result = s64_session_journal_store_save(&dos, &journal);
    if (result != S64_SESSION_JOURNAL_OK) {
        REPORT[1] = (uint8_t)result;
        REPORT[3] = turbo.read(turbo.io_context);
        REPORT[0] = 0x5Au;
        return 1;
    }
#endif
#if S64_MIGRATION_PROBE_RETIRE
    result = s64_journal_migration_retire(&dos);
#else
    result = s64_journal_migration_stage(&dos, &backend);
#endif
    REPORT[1] = (uint8_t)result;
    REPORT[3] = turbo.read(turbo.io_context);
    REPORT[0] = result == S64_SESSION_JOURNAL_OK ? 0xA5u : 0x5Au;
    return result == S64_SESSION_JOURNAL_OK ? 0 : 1;
}
