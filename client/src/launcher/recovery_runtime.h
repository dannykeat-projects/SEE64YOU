#ifndef SEE64YOU_LAUNCHER_RECOVERY_RUNTIME_H
#define SEE64YOU_LAUNCHER_RECOVERY_RUNTIME_H

#include "launcher/recovery_flow.h"
#include "platform/dos.h"
#include "platform/system_state.h"
#include "platform/turbo.h"

typedef struct {
    S64Dos dos;
    S64TurboBackend turbo;
    S64SystemBackend system_backend;
    S64SessionRecoveryStore store;
    S64SessionRecoveryBackend recovery_backend;
    S64SessionRecovery recovery;
} S64LauncherRecoveryRuntime;

void s64_launcher_recovery_runtime_init(
    S64LauncherRecoveryRuntime* runtime,
    S64UciExecuteFn execute,
    S64TurboReadFn read,
    S64TurboWriteFn write,
    void* turbo_context
);

void s64_launcher_recovery_runtime_init_hardware(
    S64LauncherRecoveryRuntime* runtime
);

S64LauncherRecoveryResult s64_launcher_recovery_runtime_run(
    S64LauncherRecoveryRuntime* runtime
);

#endif
