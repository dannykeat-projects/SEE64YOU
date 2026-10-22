#ifndef SEE64YOU_PLATFORM_MODULE_RUNTIME_H
#define SEE64YOU_PLATFORM_MODULE_RUNTIME_H

#include <stdint.h>

#include "platform/dos.h"
#include "platform/module_manifest.h"
#include "platform/reu_runtime.h"
#include "platform/session_lifecycle.h"
#include "platform/system_state.h"
#include "platform/turbo.h"

typedef struct {
    S64Dos* dos;
    S64TurboBackend turbo;
    S64SystemBackend system_backend;
    S64SystemState system_state;
    S64SessionLifecycleStore lifecycle_store;
    S64SessionLifecycle lifecycle;
    S64ReuRuntimeControl reu_control;
    S64ModuleManifest module_manifest;
    uint8_t current_module;
    uint8_t shared_invalidated;
} S64ModuleRuntime;

void s64_module_runtime_init(
    S64ModuleRuntime* runtime,
    S64Dos* dos,
    S64TurboReadFn read,
    S64TurboWriteFn write,
    void* turbo_context
);

void s64_module_runtime_init_hardware(
    S64ModuleRuntime* runtime,
    S64Dos* dos
);

uint8_t s64_module_runtime_adopt(
    S64ModuleRuntime* runtime,
    S64ModuleId destination,
    uint16_t package_version,
    uint8_t trusted_arrival
);

S64SystemResult s64_module_runtime_prepare_transition(
    S64ModuleRuntime* runtime
);

S64SystemResult s64_module_runtime_resume_interactive(
    S64ModuleRuntime* runtime
);

S64SessionLifecycleResult s64_module_runtime_end(
    S64ModuleRuntime* runtime
);

#endif
