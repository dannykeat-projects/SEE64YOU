#include "platform/module_transition.h"
#include <string.h>

static uint8_t owns_active(const S64ModuleRuntime* runtime)
{
    return runtime != 0 && runtime->system_state.captured && !runtime->system_state.restored
        && runtime->lifecycle.active_record_written && !runtime->lifecycle.clean_record_written
        && !runtime->shared_invalidated
        && runtime->current_module >= S64_MODULE_EDITOR
        && runtime->current_module <= S64_MODULE_SETTINGS
        && s64_reu_runtime_valid(&runtime->reu_control)
        && runtime->lifecycle.journal.state == S64_SESSION_STATE_ACTIVE
        && runtime->lifecycle.journal.sequence
        && runtime->lifecycle.journal.session_id == runtime->reu_control.session_id
        && runtime->lifecycle.journal.package_version == runtime->reu_control.app_version;
}

S64ModuleTransitionResult s64_module_transition_cancel(
    S64ModuleRuntime* runtime, S64ModuleTransferPlan* plan)
{
    if (plan) memset(plan, 0, sizeof(*plan));
    if (!plan || !owns_active(runtime)) return S64_MODULE_TRANSITION_INVALID;
    if (s64_system_state_set_mode(&runtime->system_state, S64_SYSTEM_MODE_IO_SAFE) != S64_SYSTEM_OK
        || !s64_transfer_capsule_clear(&runtime->reu_control)
        || s64_module_runtime_resume_interactive(runtime) != S64_SYSTEM_OK)
        return S64_MODULE_TRANSITION_RECOVERY_REQUIRED;
    return S64_MODULE_TRANSITION_OK;
}

S64ModuleTransitionResult s64_module_transition_prepare(
    S64ModuleRuntime* runtime, S64ModuleId destination, S64ModuleTransferPlan* plan)
{
    S64TransferCapsule capsule;
    S64ModuleTransferPlan candidate;
    const S64ModuleDescriptor* expected_module;
    uint8_t expected[S64_SESSION_JOURNAL_WIRE_SIZE];
    uint8_t actual[S64_SESSION_JOURNAL_WIRE_SIZE];
    if (plan) memset(plan, 0, sizeof(*plan));
    if (!plan || !owns_active(runtime)
        || runtime->system_state.mode != S64_SYSTEM_MODE_INTERACTIVE
        || destination < S64_MODULE_EDITOR || destination > S64_MODULE_SETTINGS
        || destination == runtime->current_module) return S64_MODULE_TRANSITION_INVALID;
    expected_module = &runtime->module_manifest.modules[(uint8_t)destination - 1u];
    if (s64_system_state_set_mode(&runtime->system_state, S64_SYSTEM_MODE_IO_SAFE) != S64_SYSTEM_OK)
        return S64_MODULE_TRANSITION_RECOVERY_REQUIRED;
    if (!s64_transfer_capsule_clear(&runtime->reu_control)
        || !s64_transfer_capsule_store(&runtime->reu_control,
            &runtime->lifecycle.journal, &runtime->module_manifest,
            (S64ModuleId)runtime->current_module, destination)
        || !s64_transfer_capsule_load(&runtime->reu_control, &capsule)
        || capsule.source != (S64ModuleId)runtime->current_module
        || capsule.destination != destination
        || capsule.module.id != expected_module->id
        || capsule.module.cache_state != expected_module->cache_state
        || capsule.module.abi_version != expected_module->abi_version
        || capsule.module.app_version != expected_module->app_version
        || capsule.module.load_address != expected_module->load_address
        || capsule.module.startup_address != expected_module->startup_address
        || capsule.module.byte_length != expected_module->byte_length
        || capsule.module.integrity != expected_module->integrity
        || capsule.module.cache_address != expected_module->cache_address
        || capsule.module.cache_capacity != expected_module->cache_capacity
        || !s64_session_journal_encode(&runtime->lifecycle.journal, expected)
        || !s64_session_journal_encode(&capsule.journal, actual)
        || memcmp(expected, actual, sizeof(expected))
        || !s64_module_transfer_plan(&capsule, &candidate)
        || s64_module_runtime_prepare_transition(runtime) != S64_SYSTEM_OK) {
        /* A partial store/readback must not leave an executable candidate. */
        return s64_module_transition_cancel(runtime, plan) == S64_MODULE_TRANSITION_OK
            ? S64_MODULE_TRANSITION_FAILED : S64_MODULE_TRANSITION_RECOVERY_REQUIRED;
    }
    *plan = candidate;
    return S64_MODULE_TRANSITION_OK;
}
