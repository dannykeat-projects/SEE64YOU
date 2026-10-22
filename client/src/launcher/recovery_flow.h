#ifndef SEE64YOU_LAUNCHER_RECOVERY_FLOW_H
#define SEE64YOU_LAUNCHER_RECOVERY_FLOW_H

#include <stdint.h>
#include "platform/session_recovery.h"

typedef enum {
    S64_LAUNCHER_RECOVERY_READY_NONE = 0,
    S64_LAUNCHER_RECOVERY_READY_CLEAN,
    S64_LAUNCHER_RECOVERY_READY_RESTORED,
    S64_LAUNCHER_RECOVERY_BLOCKED_INVALID_JOURNAL,
    S64_LAUNCHER_RECOVERY_BLOCKED_STORAGE,
    S64_LAUNCHER_RECOVERY_BLOCKED_RESTORE,
    S64_LAUNCHER_RECOVERY_BLOCKED_VERIFY,
    S64_LAUNCHER_RECOVERY_BLOCKED_CLEAN_WRITE,
    S64_LAUNCHER_RECOVERY_INVALID
} S64LauncherRecoveryResult;

S64LauncherRecoveryResult s64_launcher_recovery_run(
    S64SessionRecovery* recovery,
    const S64SessionRecoveryBackend* backend
);
uint8_t s64_launcher_recovery_retryable(S64LauncherRecoveryResult result);
const char* s64_launcher_recovery_message(S64LauncherRecoveryResult result);

#endif
