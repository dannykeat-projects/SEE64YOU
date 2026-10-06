#include "platform/managed_editor_settings.h"

#include "platform/reu.h"
#include "platform/package_paths.h"
#include "publisher/settings_session.h"
#include "publisher/settings_store.h"

uint8_t s64_managed_editor_settings_prepare(
    S64ModuleRuntime* runtime,
    uint8_t* scratch
)
{
    S64SettingsExchange* settings = (S64SettingsExchange*)scratch;
    S64DosResult result;
    S64DosResult closed;
    uint32_t size;

    if (!runtime || !runtime->dos || !scratch
        || runtime->current_module != S64_MODULE_EDITOR
        || !runtime->lifecycle.active_record_written
        || runtime->lifecycle.clean_record_written
        || runtime->shared_invalidated
        || !runtime->system_state.captured
        || runtime->system_state.restored
        || runtime->system_state.mode != S64_SYSTEM_MODE_IO_SAFE) {
        return 0u;
    }

    /* Revoke authority before touching SD. Ultimate-DOS can stream this exact
     * fixed-size file directly into the existing REU settings reservation, so
     * no extra C-side read/copy buffer is needed. */
    if (!s64_settings_session_revoke()) return 0u;

    if (s64_dos_change_dir(runtime->dos, S64_PACKAGE_DIRECTORY)
            != S64_DOS_OK) {
        return 0u;
    }
    result = s64_dos_open_read(runtime->dos, S64_SETTINGS_FILENAME);
    if (result == S64_DOS_NOT_FOUND) {
        if (s64_dos_change_dir(runtime->dos, S64_SD_ROOT_DIRECTORY)
                != S64_DOS_OK) {
            return 0u;
        }
        result = s64_dos_open_read(runtime->dos, S64_SETTINGS_FILENAME);
    }
    if (result != S64_DOS_OK) return 0u;

    result = s64_dos_file_size(runtime->dos, &size);
    if (result == S64_DOS_OK && size != (uint32_t)sizeof(*settings)) {
        result = S64_DOS_TARGET_ERROR;
    }
    if (result == S64_DOS_OK) {
        result = s64_dos_load_reu(
            runtime->dos,
            ((uint32_t)S64_REU_SETTINGS_BANK << 16)
                | (uint32_t)S64_REU_SETTINGS_BASE,
            (uint32_t)sizeof(*settings)
        );
    }
    closed = s64_dos_close(runtime->dos);

    return result == S64_DOS_OK
        && closed == S64_DOS_OK
        && s64_reu_load(
            S64_REU_SETTINGS_BANK,
            S64_REU_SETTINGS_BASE,
            settings,
            (uint16_t)sizeof(*settings)
        )
        && s64_settings_session_publish(
            &runtime->reu_control,
            &runtime->lifecycle.journal,
            settings
        );
}
