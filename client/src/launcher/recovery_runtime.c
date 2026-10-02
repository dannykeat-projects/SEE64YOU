#include "launcher/recovery_runtime.h"

#include <string.h>

#include "platform/session_journal.h"
#include "platform/uci.h"

static S64SessionJournalResult load_journal(
    void* context,
    S64SessionJournal* journal
)
{
    S64LauncherRecoveryRuntime* runtime =
        (S64LauncherRecoveryRuntime*)context;
    return s64_session_journal_store_load(&runtime->dos, journal);
}

static S64SessionJournalResult save_journal(
    void* context,
    S64SessionJournal* journal
)
{
    S64LauncherRecoveryRuntime* runtime =
        (S64LauncherRecoveryRuntime*)context;
    return s64_session_journal_store_save(&runtime->dos, journal);
}

static uint8_t restore_snapshot(
    void* context,
    const S64SystemSnapshot* snapshot
)
{
    S64LauncherRecoveryRuntime* runtime =
        (S64LauncherRecoveryRuntime*)context;

    if (runtime->system_backend.restore == 0) return 0u;
    return runtime->system_backend.restore(&runtime->turbo, snapshot);
}

static uint8_t verify_snapshot(
    void* context,
    const S64SystemSnapshot* snapshot
)
{
    S64LauncherRecoveryRuntime* runtime =
        (S64LauncherRecoveryRuntime*)context;

    if (runtime->system_backend.verify == 0) return 1u;
    return runtime->system_backend.verify(&runtime->turbo, snapshot);
}

void s64_launcher_recovery_runtime_init(
    S64LauncherRecoveryRuntime* runtime,
    S64UciExecuteFn execute,
    S64TurboReadFn read,
    S64TurboWriteFn write,
    void* turbo_context
)
{
    if (runtime == 0) return;

    memset(runtime, 0, sizeof(*runtime));
    s64_dos_init(&runtime->dos, execute, S64_DOS_TARGET_PRIMARY);
    s64_turbo_backend_init(
        &runtime->turbo,
        read,
        write,
        turbo_context
    );
    s64_turbo_backend_bind(&runtime->system_backend);

    runtime->store.load = load_journal;
    runtime->store.save = save_journal;
    runtime->store.context = runtime;

    runtime->recovery_backend.restore = restore_snapshot;
    runtime->recovery_backend.verify = verify_snapshot;
    runtime->recovery_backend.context = runtime;

    s64_session_recovery_init(&runtime->recovery, &runtime->store);
}

void s64_launcher_recovery_runtime_init_hardware(
    S64LauncherRecoveryRuntime* runtime
)
{
    if (runtime == 0) return;

    memset(runtime, 0, sizeof(*runtime));
    s64_dos_init(
        &runtime->dos,
        s64_uci_execute,
        S64_DOS_TARGET_PRIMARY
    );
    s64_turbo_backend_init_hardware(&runtime->turbo);
    s64_turbo_backend_bind(&runtime->system_backend);

    runtime->store.load = load_journal;
    runtime->store.save = save_journal;
    runtime->store.context = runtime;

    runtime->recovery_backend.restore = restore_snapshot;
    runtime->recovery_backend.verify = verify_snapshot;
    runtime->recovery_backend.context = runtime;

    s64_session_recovery_init(&runtime->recovery, &runtime->store);
}

S64LauncherRecoveryResult s64_launcher_recovery_runtime_run(
    S64LauncherRecoveryRuntime* runtime
)
{
    if (runtime == 0) return S64_LAUNCHER_RECOVERY_INVALID;
    return s64_launcher_recovery_run(
        &runtime->recovery,
        &runtime->recovery_backend
    );
}
