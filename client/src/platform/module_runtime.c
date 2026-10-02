#include "platform/module_runtime.h"

#include <string.h>

#include "platform/module_session.h"
#include "platform/session_journal.h"
#include "platform/transfer_capsule.h"

static S64SessionJournalResult save_journal(
    void* context,
    S64SessionJournal* journal
)
{
    S64ModuleRuntime* runtime = (S64ModuleRuntime*)context;

    if (runtime == 0 || runtime->dos == 0) {
        return S64_SESSION_JOURNAL_IO_ERROR;
    }
    return s64_session_journal_store_save(runtime->dos, journal);
}

static uint8_t descriptor_equal(
    const S64ModuleDescriptor* left,
    const S64ModuleDescriptor* right
)
{
    return left != 0 && right != 0
        && left->id == right->id
        && left->cache_state == right->cache_state
        && left->abi_version == right->abi_version
        && left->app_version == right->app_version
        && left->load_address == right->load_address
        && left->startup_address == right->startup_address
        && left->byte_length == right->byte_length
        && left->integrity == right->integrity
        && left->cache_address == right->cache_address
        && left->cache_capacity == right->cache_capacity;
}

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
}

void s64_module_runtime_init_hardware(
    S64ModuleRuntime* runtime,
    S64Dos* dos
)
{
    if (runtime == 0) return;

    memset(runtime, 0, sizeof(*runtime));
    runtime->dos = dos;
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
}

uint8_t s64_module_runtime_adopt(
    S64ModuleRuntime* runtime,
    S64ModuleId destination,
    uint16_t package_version
)
{
    S64TransferCapsule capsule;
    S64SystemSnapshot snapshot;
    const S64ModuleDescriptor* shared;

    if (runtime == 0 || package_version == 0u
        || destination < S64_MODULE_EDITOR
        || destination > S64_MODULE_SETTINGS
        || runtime->system_state.captured
        || runtime->lifecycle.active_record_written
        || runtime->lifecycle.clean_record_written
        || !s64_reu_runtime_load(&runtime->reu_control)
        || runtime->reu_control.app_version != package_version
        || !s64_transfer_capsule_load(&runtime->reu_control, &capsule)
        || capsule.destination != destination
        || capsule.journal.package_version != package_version
        || !s64_module_session_load(
            &runtime->reu_control,
            &capsule.journal,
            &runtime->module_manifest
        )) {
        return 0u;
    }

    shared = &runtime->module_manifest.modules[(uint8_t)destination - 1u];
    if (!descriptor_equal(&capsule.module, shared)
        || !s64_session_journal_restore_snapshot(
            &capsule.journal,
            &snapshot
        )
        || !s64_turbo_backend_adopt(&runtime->turbo, &snapshot)
        || s64_session_lifecycle_adopt(
            &runtime->lifecycle,
            &capsule.journal
        ) != S64_SESSION_LIFECYCLE_OK) {
        return 0u;
    }

    runtime->current_module = (uint8_t)destination;
    runtime->shared_invalidated = 0u;
    return 1u;
}

S64SystemResult s64_module_runtime_prepare_transition(
    S64ModuleRuntime* runtime
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

S64SystemResult s64_module_runtime_resume_interactive(
    S64ModuleRuntime* runtime
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

S64SessionLifecycleResult s64_module_runtime_end(
    S64ModuleRuntime* runtime
)
{
    if (runtime == 0) return S64_SESSION_LIFECYCLE_INVALID;

    if (!runtime->shared_invalidated) {
        if (s64_system_state_set_mode(
                &runtime->system_state,
                S64_SYSTEM_MODE_IO_SAFE
            ) != S64_SYSTEM_OK
            || !s64_transfer_capsule_clear(&runtime->reu_control)
            || !s64_module_session_clear(&runtime->reu_control)) {
            return S64_SESSION_LIFECYCLE_RESTORE_FAILED;
        }
        runtime->shared_invalidated = 1u;
    }

    return s64_session_lifecycle_end(&runtime->lifecycle);
}
