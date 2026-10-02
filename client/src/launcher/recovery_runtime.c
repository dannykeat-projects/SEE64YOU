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
    s64_system_state_init(
        &runtime->system_state,
        &runtime->system_backend,
        &runtime->turbo
    );
    runtime->lifecycle_store.save = save_journal;
    runtime->lifecycle_store.context = runtime;
    s64_session_lifecycle_init(
        &runtime->lifecycle,
        &runtime->system_state,
        &runtime->lifecycle_store
    );

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
#ifdef __CC65__
    s64_dos_init(
        &runtime->dos,
        s64_uci_execute,
        S64_DOS_TARGET_PRIMARY
    );
#else
    /*
     * The hardware initializer is never executed by host tests. Keep its DOS
     * execute pointer null off-target so the host object does not acquire an
     * unnecessary link-time dependency on the memory-mapped UCI transport.
     */
    s64_dos_init(
        &runtime->dos,
        0,
        S64_DOS_TARGET_PRIMARY
    );
#endif
    s64_turbo_backend_init_hardware(&runtime->turbo);
    s64_turbo_backend_bind(&runtime->system_backend);
    s64_system_state_init(
        &runtime->system_state,
        &runtime->system_backend,
        &runtime->turbo
    );
    runtime->lifecycle_store.save = save_journal;
    runtime->lifecycle_store.context = runtime;
    s64_session_lifecycle_init(
        &runtime->lifecycle,
        &runtime->system_state,
        &runtime->lifecycle_store
    );

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


S64SessionLifecycleResult s64_launcher_session_begin(
    S64LauncherRecoveryRuntime* runtime,
    uint32_t session_id,
    uint16_t package_version,
    uint32_t recovery_ref,
    S64SystemMode initial_mode
)
{
    if (runtime == 0) return S64_SESSION_LIFECYCLE_INVALID;
    return s64_session_lifecycle_begin(
        &runtime->lifecycle,
        session_id,
        package_version,
        recovery_ref,
        initial_mode
    );
}

S64SessionLifecycleResult s64_launcher_session_end(
    S64LauncherRecoveryRuntime* runtime
)
{
    if (runtime == 0) return S64_SESSION_LIFECYCLE_INVALID;
    return s64_session_lifecycle_end(&runtime->lifecycle);
}


S64SystemResult s64_launcher_session_prepare_transition(
    S64LauncherRecoveryRuntime* runtime
)
{
    if (runtime == 0
        || !runtime->lifecycle.active_record_written
        || runtime->lifecycle.clean_record_written) {
        return S64_SYSTEM_INVALID;
    }
    return s64_system_state_set_mode(
        &runtime->system_state,
        S64_SYSTEM_MODE_TRANSITION
    );
}

S64SystemResult s64_launcher_session_resume_interactive(
    S64LauncherRecoveryRuntime* runtime
)
{
    if (runtime == 0
        || !runtime->lifecycle.active_record_written
        || runtime->lifecycle.clean_record_written) {
        return S64_SYSTEM_INVALID;
    }
    return s64_system_state_set_mode(
        &runtime->system_state,
        S64_SYSTEM_MODE_INTERACTIVE
    );
}


uint8_t s64_launcher_runtime_control_create(
    S64LauncherRecoveryRuntime* runtime
)
{
    if (runtime == 0
        || !runtime->lifecycle.active_record_written
        || runtime->lifecycle.clean_record_written
        || runtime->lifecycle.journal.session_id == 0UL) {
        return 0u;
    }

    s64_reu_runtime_init(
        &runtime->reu_control,
        runtime->lifecycle.journal.package_version,
        runtime->lifecycle.journal.session_id
    );
    return s64_reu_runtime_store_verified(&runtime->reu_control);
}
