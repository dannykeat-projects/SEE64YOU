#include "launcher/code_assets_runtime.h"
#include "platform/code_assets_internal.h"
#include <string.h>

static uint8_t wire[S64_CODE_ASSET_MANIFEST_SIZE];
static uint16_t received;
static S64CodeAssets assets;

static uint8_t manifest_sink(const uint8_t* data, uint16_t length, void* context)
{
    (void)context;
    if (!data || length > sizeof(wire) - received) return 0u;
    memcpy(wire + received, data, length);
    received += length;
    return 1u;
}

uint8_t s64_launcher_code_assets_prepare(S64LauncherRecoveryRuntime* runtime)
{
    S64DosResult result, closed;
    uint32_t size;
    uint16_t read;
    if (!runtime || !runtime->lifecycle.active_record_written
        || runtime->lifecycle.clean_record_written
        || !runtime->system_state.captured || runtime->system_state.restored
        || runtime->system_state.mode != S64_SYSTEM_MODE_IO_SAFE
        || s64_dos_change_dir(&runtime->dos, "/sd/") != S64_DOS_OK
        || s64_dos_open_read(&runtime->dos,
            S64_CODE_ASSET_MANIFEST_FILENAME) != S64_DOS_OK) return 0u;
    received = 0u;
    read = 0u;
    result = s64_dos_file_size(&runtime->dos, &size);
    if (result == S64_DOS_OK && size != sizeof(wire)) result = S64_DOS_TARGET_ERROR;
    if (result == S64_DOS_OK)
        result = s64_dos_read(&runtime->dos, sizeof(wire), manifest_sink, 0, &read);
    closed = s64_dos_close(&runtime->dos);
    return result == S64_DOS_OK && closed == S64_DOS_OK
        && read == sizeof(wire) && received == read
        && s64_code_assets_parse(&assets, wire, sizeof(wire),
            &runtime->module_manifest.modules[0])
        && s64_code_assets_allocate(&assets, &runtime->reu_control)
        && s64_reu_runtime_store_verified(&runtime->reu_control)
        && s64_code_assets_preload(&runtime->dos, &runtime->reu_control,
            &runtime->module_manifest.modules[0], &assets, &runtime->system_state)
        && s64_code_assets_store_verified(&runtime->reu_control,
            &runtime->lifecycle.journal, &runtime->module_manifest.modules[0], &assets);
}

S64SessionLifecycleResult s64_launcher_code_assets_end(S64LauncherRecoveryRuntime* runtime)
{
    if (!runtime) return S64_SESSION_LIFECYCLE_INVALID;
    if (!runtime->module_session_invalidated
        && s64_code_assets_session_allocation(&runtime->reu_control)) {
        if (s64_system_state_set_mode(&runtime->system_state,
                S64_SYSTEM_MODE_IO_SAFE) != S64_SYSTEM_OK
            || !s64_code_assets_clear(&runtime->reu_control))
            return S64_SESSION_LIFECYCLE_RESTORE_FAILED;
    }
    return s64_launcher_session_end(runtime);
}
