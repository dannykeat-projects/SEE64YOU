#include "launcher/code_assets_runtime.h"
#include "launcher/code_assets_dual.h"
#include "launcher/code_assets_scratch.h"
#include <string.h>

typedef struct {
    uint8_t* data;
    uint16_t capacity;
    uint16_t received;
} S64AssetManifestRead;

static uint8_t manifest_sink(const uint8_t* data, uint16_t length, void* context)
{
    S64AssetManifestRead* read = (S64AssetManifestRead*)context;
    if (!data || !read || length > read->capacity - read->received) return 0u;
    memcpy(read->data + read->received, data, length);
    read->received = (uint16_t)(read->received + length);
    return 1u;
}

static uint8_t load_manifest(
    S64LauncherRecoveryRuntime* runtime,
    S64ModuleId module,
    S64CodeAssets* assets,
    const S64ModuleDescriptor* descriptor
)
{
    S64AssetManifestRead manifest_read;
    S64DosResult result, closed;
    const char* filename = module == S64_MODULE_EDITOR
        ? "S64OVL.MNF"
        : (module == S64_MODULE_PUBLISHER ? "S64POVL.MNF" : 0);
    uint16_t expected =
        (uint16_t)(28u + (uint16_t)s64_dual_code_assets_count(module) * 8u);
    uint32_t size;
    uint16_t read = 0u;

    if (!runtime || !assets || !descriptor || !filename
        || !s64_dual_code_assets_count(module)
        || expected > S64_CODE_ASSET_SESSION_SIZE
        || s64_dos_change_dir(&runtime->dos, "/sd/") != S64_DOS_OK
        || s64_dos_open_read(&runtime->dos, filename) != S64_DOS_OK)
        return 0u;

    manifest_read.data = S64_DUAL_ASSET_RECORD;
    manifest_read.capacity = expected;
    manifest_read.received = 0u;
    result = s64_dos_file_size(&runtime->dos, &size);
    if (result == S64_DOS_OK && size != expected) result = S64_DOS_TARGET_ERROR;
    if (result == S64_DOS_OK)
        result = s64_dos_read(&runtime->dos, expected, manifest_sink,
            &manifest_read, &read);
    closed = s64_dos_close(&runtime->dos);

    return result == S64_DOS_OK && closed == S64_DOS_OK
        && read == expected && manifest_read.received == read
        && s64_dual_code_assets_parse(module, assets,
            S64_DUAL_ASSET_RECORD, expected, descriptor);
}

uint8_t s64_launcher_code_assets_prepare(S64LauncherRecoveryRuntime* runtime)
{
    S64ReuRuntimeControl* planned = S64_DUAL_CONTROL;
    S64CodeAssets* editor_assets = S64_DUAL_EDITOR_ASSETS;
    S64CodeAssets* publisher_assets = S64_DUAL_PUBLISHER_ASSETS;

    if (!runtime || !runtime->lifecycle.active_record_written
        || runtime->lifecycle.clean_record_written
        || !runtime->system_state.captured || runtime->system_state.restored
        || runtime->system_state.mode != S64_SYSTEM_MODE_IO_SAFE) return 0u;

    if (!load_manifest(runtime, S64_MODULE_EDITOR, editor_assets,
            &runtime->module_manifest.modules[0])
        || !load_manifest(runtime, S64_MODULE_PUBLISHER, publisher_assets,
            &runtime->module_manifest.modules[1])) return 0u;

    *planned = runtime->reu_control;
    if (!s64_dual_code_assets_allocate(S64_MODULE_EDITOR,
            editor_assets, planned)
        || !s64_dual_code_assets_allocate(S64_MODULE_PUBLISHER,
            publisher_assets, planned)
        || planned->allocation_count != S64_REU_RUNTIME_MAX_ALLOCATIONS
        || !s64_reu_runtime_store_verified(planned)) return 0u;
    runtime->reu_control = *planned;

    if (!s64_dual_code_assets_preload(S64_MODULE_EDITOR, &runtime->dos,
            &runtime->reu_control, &runtime->module_manifest.modules[0],
            editor_assets, &runtime->system_state)
        || !s64_dual_code_assets_preload(S64_MODULE_PUBLISHER, &runtime->dos,
            &runtime->reu_control, &runtime->module_manifest.modules[1],
            publisher_assets, &runtime->system_state)
        || !s64_dual_code_assets_store_verified(S64_MODULE_EDITOR,
            &runtime->reu_control, &runtime->lifecycle.journal,
            &runtime->module_manifest.modules[0], editor_assets)
        || !s64_dual_code_assets_store_verified(S64_MODULE_PUBLISHER,
            &runtime->reu_control, &runtime->lifecycle.journal,
            &runtime->module_manifest.modules[1], publisher_assets)) {
        (void)s64_dual_code_assets_clear(S64_MODULE_EDITOR,
            &runtime->reu_control);
        (void)s64_dual_code_assets_clear(S64_MODULE_PUBLISHER,
            &runtime->reu_control);
        return 0u;
    }
    return 1u;
}

S64SessionLifecycleResult s64_launcher_code_assets_end(
    S64LauncherRecoveryRuntime* runtime
)
{
    if (!runtime) return S64_SESSION_LIFECYCLE_INVALID;
    if (!runtime->module_session_invalidated
        && runtime->reu_control.allocation_count
            == S64_REU_RUNTIME_MAX_ALLOCATIONS) {
        if (s64_system_state_set_mode(&runtime->system_state,
                S64_SYSTEM_MODE_IO_SAFE) != S64_SYSTEM_OK)
            return S64_SESSION_LIFECYCLE_RESTORE_FAILED;
        if (!s64_dual_code_assets_clear(S64_MODULE_EDITOR,
                &runtime->reu_control)) {
            (void)s64_dual_code_assets_clear(S64_MODULE_PUBLISHER,
                &runtime->reu_control);
            return S64_SESSION_LIFECYCLE_RESTORE_FAILED;
        }
        if (!s64_dual_code_assets_clear(S64_MODULE_PUBLISHER,
                &runtime->reu_control))
            return S64_SESSION_LIFECYCLE_RESTORE_FAILED;
    }
    return s64_launcher_session_end(runtime);
}
