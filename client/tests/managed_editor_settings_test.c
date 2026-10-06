#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "platform/managed_editor_settings.h"
#include "platform/package_paths.h"
#include "platform/reu.h"
#include "platform/reu_layout.h"
#include "publisher/settings_session.h"
#include "publisher/settings_store.h"

static uint8_t reu_settings[sizeof(S64SettingsExchange)];
static uint8_t current_path;
static uint8_t package_exists, legacy_exists;
static uint8_t revoke_ok, transfer_ok, publish_ok;
static unsigned package_dirs, root_dirs, opens, closes, transfers;
static unsigned reu_reads, revokes, publishes;

static void reset_case(void)
{
    memset(reu_settings, 0x5a, sizeof(reu_settings));
    current_path = 0u;
    package_exists = legacy_exists = 0u;
    revoke_ok = transfer_ok = publish_ok = 1u;
    package_dirs = root_dirs = opens = closes = transfers = 0u;
    reu_reads = revokes = publishes = 0u;
}

static void setup_runtime(S64ModuleRuntime* runtime, S64Dos* dos)
{
    memset(runtime, 0, sizeof(*runtime));
    memset(dos, 0, sizeof(*dos));
    runtime->dos = dos;
    runtime->current_module = S64_MODULE_EDITOR;
    runtime->lifecycle.active_record_written = 1u;
    runtime->system_state.captured = 1u;
    runtime->system_state.mode = S64_SYSTEM_MODE_IO_SAFE;
}

S64DosResult s64_dos_change_dir(S64Dos* dos, const char* directory)
{
    (void)dos;
    if (!strcmp(directory, S64_PACKAGE_DIRECTORY)) {
        current_path = 1u;
        ++package_dirs;
        return S64_DOS_OK;
    }
    if (!strcmp(directory, S64_SD_ROOT_DIRECTORY)) {
        current_path = 2u;
        ++root_dirs;
        return S64_DOS_OK;
    }
    return S64_DOS_INVALID_ARGUMENT;
}

S64DosResult s64_dos_open_read(S64Dos* dos, const char* filename)
{
    (void)dos;
    assert(!strcmp(filename, S64_SETTINGS_FILENAME));
    ++opens;
    if ((current_path == 1u && package_exists)
        || (current_path == 2u && legacy_exists)) {
        return S64_DOS_OK;
    }
    return S64_DOS_NOT_FOUND;
}

S64DosResult s64_dos_load_reu(S64Dos* dos, uint32_t address, uint32_t length)
{
    (void)dos;
    ++transfers;
    assert(address == (((uint32_t)S64_REU_SETTINGS_BANK << 16)
        | (uint32_t)S64_REU_SETTINGS_BASE));
    assert(length == sizeof(S64SettingsExchange));
    return transfer_ok ? S64_DOS_OK : S64_DOS_TARGET_ERROR;
}

S64DosResult s64_dos_close(S64Dos* dos)
{
    (void)dos;
    ++closes;
    return S64_DOS_OK;
}

uint8_t s64_reu_load(uint8_t bank, uint16_t address, void* destination,
    uint16_t length)
{
    ++reu_reads;
    assert(bank == S64_REU_SETTINGS_BANK);
    assert(address == S64_REU_SETTINGS_BASE);
    assert(length == sizeof(reu_settings));
    memcpy(destination, reu_settings, length);
    return 1u;
}

uint8_t s64_settings_session_revoke(void)
{
    ++revokes;
    return revoke_ok;
}

uint8_t s64_settings_session_publish(
    const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal,
    S64SettingsExchange* settings
)
{
    (void)control;
    (void)journal;
    ++publishes;
    assert(settings != 0);
    return publish_ok;
}

int main(void)
{
    S64ModuleRuntime runtime;
    S64Dos dos;
    uint8_t scratch[sizeof(S64SettingsExchange)];

    reset_case();
    setup_runtime(&runtime, &dos);
    package_exists = legacy_exists = 1u;
    assert(s64_managed_editor_settings_prepare(&runtime, scratch));
    assert(revokes == 1u && package_dirs == 1u && root_dirs == 0u);
    assert(opens == 1u && transfers == 1u && closes == 1u);
    assert(reu_reads == 1u && publishes == 1u);

    reset_case();
    setup_runtime(&runtime, &dos);
    legacy_exists = 1u;
    assert(s64_managed_editor_settings_prepare(&runtime, scratch));
    assert(package_dirs == 1u && root_dirs == 1u);
    assert(opens == 2u && transfers == 1u && publishes == 1u);

    reset_case();
    setup_runtime(&runtime, &dos);
    assert(!s64_managed_editor_settings_prepare(&runtime, scratch));
    assert(package_dirs == 1u && root_dirs == 1u);
    assert(opens == 2u && transfers == 0u && publishes == 0u);

    reset_case();
    setup_runtime(&runtime, &dos);
    package_exists = 1u;
    transfer_ok = 0u;
    assert(!s64_managed_editor_settings_prepare(&runtime, scratch));
    assert(closes == 1u && publishes == 0u);

    reset_case();
    setup_runtime(&runtime, &dos);
    package_exists = 1u;
    publish_ok = 0u;
    assert(!s64_managed_editor_settings_prepare(&runtime, scratch));
    assert(transfers == 1u && reu_reads == 1u && publishes == 1u);

    reset_case();
    setup_runtime(&runtime, &dos);
    package_exists = 1u;
    revoke_ok = 0u;
    assert(!s64_managed_editor_settings_prepare(&runtime, scratch));
    assert(package_dirs == 0u && opens == 0u && publishes == 0u);

    puts("managed editor settings path/fallback tests passed");
    return 0;
}
