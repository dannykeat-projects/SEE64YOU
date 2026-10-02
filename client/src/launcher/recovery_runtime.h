#ifndef SEE64YOU_LAUNCHER_RECOVERY_RUNTIME_H
#define SEE64YOU_LAUNCHER_RECOVERY_RUNTIME_H

#include "launcher/recovery_flow.h"
#include "platform/dos.h"
#include "platform/module_manifest.h"
#include "platform/module_session.h"
#include "platform/reu_runtime.h"
#include "platform/session_lifecycle.h"
#include "platform/system_state.h"
#include "platform/turbo.h"

typedef struct {
    S64Dos dos;
    S64TurboBackend turbo;
    S64SystemBackend system_backend;
    S64SystemState system_state;
    S64SessionLifecycleStore lifecycle_store;
    S64SessionLifecycle lifecycle;
    S64SessionRecoveryStore store;
    S64SessionRecoveryBackend recovery_backend;
    S64SessionRecovery recovery;
    S64ReuRuntimeControl reu_control;
    S64ModuleManifest module_manifest;
    uint8_t module_session_invalidated;
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

S64SessionLifecycleResult s64_launcher_session_begin(
    S64LauncherRecoveryRuntime* runtime,
    uint32_t session_id,
    uint16_t package_version,
    uint32_t recovery_ref,
    S64SystemMode initial_mode
);

uint8_t s64_launcher_runtime_control_create(
    S64LauncherRecoveryRuntime* runtime
);
uint8_t s64_launcher_module_cache_plan(
    S64LauncherRecoveryRuntime* runtime
);
uint8_t s64_launcher_module_session_clear(S64LauncherRecoveryRuntime* runtime);
uint8_t s64_launcher_module_session_publish(S64LauncherRecoveryRuntime* runtime);
uint8_t s64_launcher_module_manifest_load(
    S64LauncherRecoveryRuntime* runtime
);

S64SystemResult s64_launcher_session_prepare_transition(
    S64LauncherRecoveryRuntime* runtime
);

S64SystemResult s64_launcher_session_resume_interactive(
    S64LauncherRecoveryRuntime* runtime
);

S64SessionLifecycleResult s64_launcher_session_end(
    S64LauncherRecoveryRuntime* runtime
);

#endif
