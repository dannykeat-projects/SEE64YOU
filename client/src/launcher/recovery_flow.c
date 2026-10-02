#include "launcher/recovery_flow.h"

S64LauncherRecoveryResult s64_launcher_recovery_run(
    S64SessionRecovery* recovery,
    const S64SessionRecoveryBackend* backend
)
{
    S64SessionRecoveryDecision decision;
    S64SessionRecoveryResolveResult resolved;

    if (recovery == 0) return S64_LAUNCHER_RECOVERY_INVALID;

    decision = s64_session_recovery_inspect(recovery);
    switch (decision) {
        case S64_SESSION_RECOVERY_NONE:
            return S64_LAUNCHER_RECOVERY_READY_NONE;
        case S64_SESSION_RECOVERY_CLEAN:
            return S64_LAUNCHER_RECOVERY_READY_CLEAN;
        case S64_SESSION_RECOVERY_INVALID_JOURNAL:
            return S64_LAUNCHER_RECOVERY_BLOCKED_INVALID_JOURNAL;
        case S64_SESSION_RECOVERY_STORAGE_ERROR:
            return S64_LAUNCHER_RECOVERY_BLOCKED_STORAGE;
        case S64_SESSION_RECOVERY_INTERRUPTED:
            break;
        default:
            return S64_LAUNCHER_RECOVERY_INVALID;
    }

    resolved = s64_session_recovery_resolve(recovery, backend);
    switch (resolved) {
        case S64_SESSION_RECOVERY_RESOLVE_OK:
            return S64_LAUNCHER_RECOVERY_READY_RESTORED;
        case S64_SESSION_RECOVERY_RESOLVE_RESTORE_FAILED:
            return S64_LAUNCHER_RECOVERY_BLOCKED_RESTORE;
        case S64_SESSION_RECOVERY_RESOLVE_VERIFY_FAILED:
            return S64_LAUNCHER_RECOVERY_BLOCKED_VERIFY;
        case S64_SESSION_RECOVERY_RESOLVE_CLEAN_FAILED:
            return S64_LAUNCHER_RECOVERY_BLOCKED_CLEAN_WRITE;
        case S64_SESSION_RECOVERY_RESOLVE_INVALID:
        case S64_SESSION_RECOVERY_RESOLVE_NOT_INTERRUPTED:
        default:
            return S64_LAUNCHER_RECOVERY_INVALID;
    }
}

uint8_t s64_launcher_recovery_retryable(S64LauncherRecoveryResult result)
{
    return result == S64_LAUNCHER_RECOVERY_BLOCKED_STORAGE
        || result == S64_LAUNCHER_RECOVERY_BLOCKED_RESTORE
        || result == S64_LAUNCHER_RECOVERY_BLOCKED_VERIFY
        || result == S64_LAUNCHER_RECOVERY_BLOCKED_CLEAN_WRITE;
}

const char* s64_launcher_recovery_message(S64LauncherRecoveryResult result)
{
    switch (result) {
        case S64_LAUNCHER_RECOVERY_READY_NONE:
        case S64_LAUNCHER_RECOVERY_READY_CLEAN:
            return "";
        case S64_LAUNCHER_RECOVERY_READY_RESTORED:
            return "PREVIOUS SESSION RECOVERED";
        case S64_LAUNCHER_RECOVERY_BLOCKED_INVALID_JOURNAL:
            return "RECOVERY DATA INVALID - DIAGNOSTICS";
        case S64_LAUNCHER_RECOVERY_BLOCKED_STORAGE:
            return "RECOVERY STORAGE ERROR - RETRY";
        case S64_LAUNCHER_RECOVERY_BLOCKED_RESTORE:
            return "MACHINE STATE RESTORE FAILED - RETRY";
        case S64_LAUNCHER_RECOVERY_BLOCKED_VERIFY:
            return "MACHINE STATE VERIFY FAILED - RETRY";
        case S64_LAUNCHER_RECOVERY_BLOCKED_CLEAN_WRITE:
            return "RECOVERY SAVE FAILED - RETRY";
        case S64_LAUNCHER_RECOVERY_INVALID:
        default:
            return "RECOVERY INTERNAL ERROR - DIAGNOSTICS";
    }
}
