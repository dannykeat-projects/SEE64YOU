#include "platform/module_runtime_internal.h"
#include <string.h>

void s64_module_runtime_init(
    S64ModuleRuntime* runtime,
    S64Dos* dos,
    S64TurboReadFn read,
    S64TurboWriteFn write,
    void* turbo_context
)
{
    if (runtime == 0) return;

    memset(runtime, 0, sizeof(*runtime));
    runtime->dos = dos;
    s64_turbo_backend_init(&runtime->turbo, read, write, turbo_context);
    s64_turbo_backend_bind_adopt(&runtime->system_backend);
    s64_system_state_init(
        &runtime->system_state,
        &runtime->system_backend,
        &runtime->turbo
    );
    runtime->lifecycle_store.save = module_runtime_save_journal;
    runtime->lifecycle_store.context = runtime;
    s64_session_lifecycle_init(
        &runtime->lifecycle,
        &runtime->system_state,
        &runtime->lifecycle_store
    );
}
