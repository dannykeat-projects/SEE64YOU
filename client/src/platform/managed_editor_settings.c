#include "platform/managed_editor_settings.h"

#include "publisher/settings_session.h"
#include "publisher/settings_store.h"

#include <string.h>

typedef struct {
    uint8_t* target;
    uint16_t capacity;
    uint16_t received;
} S64ManagedSettingsRead;

static uint8_t settings_sink(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64ManagedSettingsRead* read = (S64ManagedSettingsRead*)context;

    if (!data || !read || !read->target
        || length > read->capacity - read->received) {
        return 0u;
    }
    memcpy(read->target + read->received, data, length);
    read->received = (uint16_t)(read->received + length);
    return 1u;
}

uint8_t s64_managed_editor_settings_prepare(
    S64ModuleRuntime* runtime,
    uint8_t* scratch
)
{
    S64ManagedSettingsRead read_state;
    S64SettingsExchange* settings = (S64SettingsExchange*)scratch;
    S64DosResult result;
    S64DosResult closed;
    uint32_t size;
    uint16_t bytes_read = 0u;

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

    /* Authority is revoked first, so a failed/missing/corrupt settings file
     * cannot leave an older managed Publisher settings record usable. */
    if (!s64_settings_session_revoke()
        || s64_dos_change_dir(runtime->dos, "/sd/") != S64_DOS_OK
        || s64_dos_open_read(runtime->dos, S64_SETTINGS_FILENAME)
            != S64_DOS_OK) {
        return 0u;
    }

    memset(settings, 0, sizeof(*settings));
    read_state.target = scratch;
    read_state.capacity = (uint16_t)sizeof(*settings);
    read_state.received = 0u;

    result = s64_dos_file_size(runtime->dos, &size);
    if (result == S64_DOS_OK && size != (uint32_t)sizeof(*settings)) {
        result = S64_DOS_TARGET_ERROR;
    }
    if (result == S64_DOS_OK) {
        result = s64_dos_read(
            runtime->dos,
            (uint16_t)sizeof(*settings),
            settings_sink,
            &read_state,
            &bytes_read
        );
    }
    closed = s64_dos_close(runtime->dos);

    return result == S64_DOS_OK
        && closed == S64_DOS_OK
        && bytes_read == (uint16_t)sizeof(*settings)
        && read_state.received == bytes_read
        && settings->state == S64_SETTINGS_STATE_READY
        && s64_settings_exchange_validate(settings)
        && s64_settings_session_publish(
            &runtime->reu_control,
            &runtime->lifecycle.journal,
            settings
        );
}
