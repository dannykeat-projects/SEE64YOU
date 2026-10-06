#include "launcher/code_assets_runtime.h"
#include "platform/code_assets_internal.h"
#include <string.h>

static uint8_t wire[S64_CODE_ASSET_MANIFEST_SIZE];
static uint16_t received;
static uint16_t expected_length;
static S64CodeAssets editor_assets;
static S64CodeAssets publisher_assets;

static uint8_t manifest_sink(const uint8_t* data, uint16_t length, void* context)
{
    (void)context;
    if (!data || length > expected_length - received) return 0u;
    memcpy(wire + received, data, length);
    received += length;
    return 1u;
}

static uint8_t load_manifest(S64LauncherRecoveryRuntime* runtime,
    const S64CodeAssetNamespace* namespace, S64CodeAssets* assets,
    const S64ModuleDescriptor* module)
{
    S64DosResult result, closed;
    uint32_t size;
    uint16_t read;
    if (!runtime || !namespace || !assets || !module
        || !namespace->manifest_filename || !namespace->overlay_count
        || namespace->overlay_count > S64_CODE_OVERLAY_COUNT) return 0u;
    expected_length = (uint16_t)(28u + (uint16_t)namespace->overlay_count * 8u);
    if (expected_length > sizeof(wire)
        || s64_dos_change_dir(&runtime->dos, "/sd/") != S64_DOS_OK
        || s64_dos_open_read(&runtime->dos, namespace->manifest_filename) != S64_DOS_OK)
        return 0u;
    received = 0u;
    read = 0u;
    result = s64_dos_file_size(&runtime->dos, &size);
    if (result == S64_DOS_OK && size != expected_length) result = S64_DOS_TARGET_ERROR;
    if (result == S64_DOS_OK)
        result = s64_dos_read(&runtime->dos, expected_length, manifest_sink, 0, &read);
    closed = s64_dos_close(&runtime->dos);
    return result == S64_DOS_OK && closed == S64_DOS_OK
        && read == expected_length && received == read
        && s64_code_assets_parse_for(namespace, assets, wire, expected_length, module);
}

static uint8_t session_reserved(const S64ReuRuntimeControl* control,
    const S64CodeAssetNamespace* namespace)
{
    uint8_t i;
    if (!control || !namespace) return 0u;
    for (i = 0u; i < control->allocation_count; ++i) {
        const S64ReuAllocation* item = &control->allocations[i];
        if (item->owner == S64_REU_OWNER_RUNTIME
            && item->flags == namespace->session_flag) return 1u;
    }
    return 0u;
}

static uint8_t clear_ready_sessions(S64LauncherRecoveryRuntime* runtime,
    const S64CodeAssetNamespace* editor_namespace,
    const S64CodeAssetNamespace* publisher_namespace)
{
    uint8_t ok = 1u;
    if (session_reserved(&runtime->reu_control, editor_namespace)
        && !s64_code_assets_clear_for(editor_namespace, &runtime->reu_control)) ok = 0u;
    if (session_reserved(&runtime->reu_control, publisher_namespace)
        && !s64_code_assets_clear_for(publisher_namespace, &runtime->reu_control)) ok = 0u;
    return ok;
}

uint8_t s64_launcher_code_assets_prepare(S64LauncherRecoveryRuntime* runtime)
{
    const S64CodeAssetNamespace* editor_namespace =
        s64_code_assets_namespace(S64_MODULE_EDITOR);
    const S64CodeAssetNamespace* publisher_namespace =
        s64_code_assets_namespace(S64_MODULE_PUBLISHER);
    S64ReuRuntimeControl planned;
    if (!runtime || !editor_namespace || !publisher_namespace
        || !runtime->lifecycle.active_record_written
        || runtime->lifecycle.clean_record_written
        || !runtime->system_state.captured || runtime->system_state.restored
        || runtime->system_state.mode != S64_SYSTEM_MODE_IO_SAFE) return 0u;

    if (!load_manifest(runtime, editor_namespace, &editor_assets,
            &runtime->module_manifest.modules[0])
        || !load_manifest(runtime, publisher_namespace, &publisher_assets,
            &runtime->module_manifest.modules[1])) return 0u;

    planned = runtime->reu_control;
    if (!s64_code_assets_allocate_for(editor_namespace, &editor_assets, &planned)
        || !s64_code_assets_allocate_for(publisher_namespace, &publisher_assets, &planned)
        || planned.allocation_count != S64_REU_RUNTIME_MAX_ALLOCATIONS
        || !s64_reu_runtime_store_verified(&planned)) return 0u;
    runtime->reu_control = planned;

    if (!clear_ready_sessions(runtime, editor_namespace, publisher_namespace)
        || !s64_code_assets_preload_for(editor_namespace,
            &runtime->dos, &runtime->reu_control, &runtime->module_manifest.modules[0],
            &editor_assets, &runtime->system_state)
        || !s64_code_assets_preload_for(publisher_namespace,
            &runtime->dos, &runtime->reu_control, &runtime->module_manifest.modules[1],
            &publisher_assets, &runtime->system_state)
        || !s64_code_assets_store_verified_for(editor_namespace,
            &runtime->reu_control, &runtime->lifecycle.journal,
            &runtime->module_manifest.modules[0], &editor_assets)
        || !s64_code_assets_store_verified_for(publisher_namespace,
            &runtime->reu_control, &runtime->lifecycle.journal,
            &runtime->module_manifest.modules[1], &publisher_assets)) {
        (void)clear_ready_sessions(runtime, editor_namespace, publisher_namespace);
        return 0u;
    }
    return 1u;
}

S64SessionLifecycleResult s64_launcher_code_assets_end(S64LauncherRecoveryRuntime* runtime)
{
    const S64CodeAssetNamespace* editor_namespace =
        s64_code_assets_namespace(S64_MODULE_EDITOR);
    const S64CodeAssetNamespace* publisher_namespace =
        s64_code_assets_namespace(S64_MODULE_PUBLISHER);
    if (!runtime || !editor_namespace || !publisher_namespace)
        return S64_SESSION_LIFECYCLE_INVALID;
    if (!runtime->module_session_invalidated
        && (session_reserved(&runtime->reu_control, editor_namespace)
            || session_reserved(&runtime->reu_control, publisher_namespace))) {
        if (s64_system_state_set_mode(&runtime->system_state,
                S64_SYSTEM_MODE_IO_SAFE) != S64_SYSTEM_OK
            || !clear_ready_sessions(runtime, editor_namespace, publisher_namespace))
            return S64_SESSION_LIFECYCLE_RESTORE_FAILED;
    }
    return s64_launcher_session_end(runtime);
}
