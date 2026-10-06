#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "launcher/code_assets_runtime.h"
#include "platform/code_assets.h"\n#include "platform/package_paths.h"

enum { FAIL_NONE, FAIL_PARSE, FAIL_ALLOCATE, FAIL_CONTROL, FAIL_PRELOAD, FAIL_READY };
static uint8_t wire[S64_CODE_ASSET_MANIFEST_SIZE + 1u];
static uint8_t fail_stage, wrong_size, short_read, overflow_read, close_fail;
static uint8_t open_file, fail_mode, clear_ok, retry_clean;
static unsigned dirs, opens, sizes, reads, closes, parses, allocates;
static unsigned controls, preloads, ready_attempts, ready_published;
static unsigned mode_calls, clear_calls, end_calls;
static unsigned event_count;
static char events[32];

static void event(char value)
{
    assert(event_count < sizeof(events));
    events[event_count++] = value;
}

static void reset(void)
{
    fail_stage = wrong_size = short_read = overflow_read = close_fail = 0u;
    open_file = fail_mode = 0u;
    clear_ok = 1u;
    retry_clean = 0u;
    dirs = opens = sizes = reads = closes = parses = allocates = 0u;
    controls = preloads = ready_attempts = ready_published = 0u;
    mode_calls = clear_calls = end_calls = event_count = 0u;
    memset(events, 0, sizeof(events));
}

S64DosResult s64_dos_change_dir(S64Dos* dos, const char* path)
{ assert(dos && !strcmp(path, S64_PACKAGE_DIRECTORY)); ++dirs; event('D'); return S64_DOS_OK; }
S64DosResult s64_dos_open_read(S64Dos* dos, const char* name)
{
    assert(dos && !open_file && !strcmp(name, S64_CODE_ASSET_MANIFEST_FILENAME));
    ++opens; open_file = 1u; event('O'); return S64_DOS_OK;
}
S64DosResult s64_dos_file_size(S64Dos* dos, uint32_t* size)
{
    assert(dos && open_file && size); ++sizes; event('S');
    *size = wrong_size ? S64_CODE_ASSET_MANIFEST_SIZE - 1u : S64_CODE_ASSET_MANIFEST_SIZE;
    return S64_DOS_OK;
}
S64DosResult s64_dos_read(S64Dos* dos, uint16_t requested, S64UciDataSink sink,
    void* context, uint16_t* bytes_read)
{
    uint16_t offset = 0u, amount;
    assert(dos && open_file && requested == S64_CODE_ASSET_MANIFEST_SIZE && sink && bytes_read);
    ++reads; event('R'); *bytes_read = 0u;
    if (overflow_read) {
        assert(!sink(wire, S64_CODE_ASSET_MANIFEST_SIZE + 1u, context));
        return S64_DOS_CONSUMER_ABORTED;
    }
    while (offset < requested - (short_read ? 1u : 0u)) {
        amount = (uint16_t)(requested - offset);
        if (amount > 11u) amount = 11u;
        if (short_read && offset + amount == requested) --amount;
        if (!sink(wire + offset, amount, context)) return S64_DOS_CONSUMER_ABORTED;
        offset = (uint16_t)(offset + amount);
    }
    *bytes_read = offset;
    return S64_DOS_OK;
}
S64DosResult s64_dos_close(S64Dos* dos)
{
    assert(dos && open_file); ++closes; open_file = 0u; event('C');
    return close_fail ? S64_DOS_TARGET_ERROR : S64_DOS_OK;
}

uint8_t s64_code_assets_parse(S64CodeAssets* assets, const uint8_t* bytes,
    uint16_t length, const S64ModuleDescriptor* editor)
{
    assert(assets && editor && length == S64_CODE_ASSET_MANIFEST_SIZE);
    assert(memcmp(bytes, wire, length) == 0);
    ++parses; event('P'); return fail_stage != FAIL_PARSE;
}
uint8_t s64_code_assets_allocate(S64CodeAssets* assets, S64ReuRuntimeControl* control)
{
    assert(assets && control && parses == 1u); ++allocates; event('A');
    return fail_stage != FAIL_ALLOCATE;
}
uint8_t s64_reu_runtime_store_verified(const S64ReuRuntimeControl* control)
{
    assert(control && allocates == 1u); ++controls; event('V');
    return fail_stage != FAIL_CONTROL;
}
uint8_t s64_code_assets_preload(S64Dos* dos, const S64ReuRuntimeControl* control,
    const S64ModuleDescriptor* editor, S64CodeAssets* assets, S64SystemState* system)
{
    assert(dos && control && editor && assets && system && controls == 1u);
    ++preloads; event('L'); return fail_stage != FAIL_PRELOAD;
}
uint8_t s64_code_assets_store_verified(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const S64ModuleDescriptor* editor,
    const S64CodeAssets* assets)
{
    assert(control && journal && editor && assets && preloads == 1u);
    ++ready_attempts; event('R');
    if (fail_stage == FAIL_READY) return 0u;
    ++ready_published; event('Y'); return 1u;
}
const S64ReuAllocation* s64_code_assets_session_allocation(const S64ReuRuntimeControl* control)
{
    return control && control->allocation_count ? &control->allocations[0] : 0;
}
S64SystemResult s64_system_state_set_mode(S64SystemState* state, S64SystemMode mode)
{
    assert(state && mode == S64_SYSTEM_MODE_IO_SAFE);
    ++mode_calls; event('M'); return fail_mode ? S64_SYSTEM_APPLY_FAILED : S64_SYSTEM_OK;
}
uint8_t s64_code_assets_clear(const S64ReuRuntimeControl* control)
{
    assert(control); ++clear_calls; event('X'); return clear_ok;
}
S64SessionLifecycleResult s64_launcher_session_end(S64LauncherRecoveryRuntime* runtime)
{
    assert(runtime); ++end_calls; event('E');
    if (retry_clean && end_calls == 1u) {
        runtime->module_session_invalidated = 1u;
        return S64_SESSION_LIFECYCLE_JOURNAL_CLEAN_FAILED;
    }
    return S64_SESSION_LIFECYCLE_OK;
}

static void prepare_runtime(S64LauncherRecoveryRuntime* runtime)
{
    memset(runtime, 0, sizeof(*runtime));
    runtime->lifecycle.active_record_written = 1u;
    runtime->system_state.mode = S64_SYSTEM_MODE_IO_SAFE;
    runtime->system_state.captured = 1u;
    runtime->reu_control.allocation_count = 1u;
    runtime->reu_control.allocations[0].owner = S64_REU_OWNER_RUNTIME;
    runtime->reu_control.allocations[0].flags = S64_CODE_ASSET_SESSION_FLAG;
    runtime->reu_control.allocations[0].length = S64_CODE_ASSET_SESSION_SIZE;
}

static void require_no_ready(void)
{ assert(ready_attempts == 0u && ready_published == 0u); }

int main(void)
{
    S64LauncherRecoveryRuntime runtime;
    memset(wire, 0x5a, sizeof(wire));

    reset(); prepare_runtime(&runtime);
    assert(s64_launcher_code_assets_prepare(&runtime));
    assert(!open_file && dirs == 1u && opens == 1u && sizes == 1u && reads == 1u && closes == 1u);
    assert(parses == 1u && allocates == 1u && controls == 1u && preloads == 1u);
    assert(ready_attempts == 1u && ready_published == 1u);
    assert(event_count == 11u && !memcmp(events, "DOSRCPAVLRY", 11u));

    for (unsigned failure = 1u; failure <= 4u; ++failure) {
        reset(); prepare_runtime(&runtime);
        wrong_size = failure == 1u; short_read = failure == 2u;
        overflow_read = failure == 3u; close_fail = failure == 4u;
        assert(!s64_launcher_code_assets_prepare(&runtime));
        require_no_ready();
        assert(!open_file && closes == 1u);
        assert(parses == 0u && allocates == 0u && controls == 0u && preloads == 0u);
    }
    for (uint8_t stage = FAIL_PARSE; stage <= FAIL_READY; ++stage) {
        reset(); prepare_runtime(&runtime); fail_stage = stage;
        assert(!s64_launcher_code_assets_prepare(&runtime));
        assert(!open_file && closes == 1u);
        assert(ready_published == 0u);
        if (stage < FAIL_READY) require_no_ready();
        if (stage <= FAIL_ALLOCATE) assert(controls == 0u && preloads == 0u);
        if (stage == FAIL_CONTROL) assert(preloads == 0u && ready_attempts == 0u);
        if (stage == FAIL_PRELOAD) assert(ready_attempts == 0u);
        if (stage == FAIL_READY) assert(allocates == 1u && preloads == 1u && ready_attempts == 1u);
    }

    reset(); prepare_runtime(&runtime); runtime.lifecycle.active_record_written = 0u;
    assert(!s64_launcher_code_assets_prepare(&runtime)); assert(dirs == 0u);
    prepare_runtime(&runtime); runtime.lifecycle.clean_record_written = 1u;
    assert(!s64_launcher_code_assets_prepare(&runtime)); assert(dirs == 0u);
    prepare_runtime(&runtime); runtime.system_state.mode = S64_SYSTEM_MODE_INTERACTIVE;
    assert(!s64_launcher_code_assets_prepare(&runtime)); assert(dirs == 0u);
    prepare_runtime(&runtime); runtime.system_state.captured = 0u;
    assert(!s64_launcher_code_assets_prepare(&runtime)); assert(dirs == 0u);
    prepare_runtime(&runtime); runtime.system_state.restored = 1u;
    assert(!s64_launcher_code_assets_prepare(&runtime)); assert(dirs == 0u);

    reset(); prepare_runtime(&runtime); fail_mode = 1u;
    assert(s64_launcher_code_assets_end(&runtime) == S64_SESSION_LIFECYCLE_RESTORE_FAILED);
    assert(mode_calls == 1u && clear_calls == 0u && end_calls == 0u);
    reset(); prepare_runtime(&runtime); clear_ok = 0u;
    assert(s64_launcher_code_assets_end(&runtime) == S64_SESSION_LIFECYCLE_RESTORE_FAILED);
    assert(mode_calls == 1u && clear_calls == 1u && end_calls == 0u);

    reset(); prepare_runtime(&runtime); retry_clean = 1u;
    assert(s64_launcher_code_assets_end(&runtime) == S64_SESSION_LIFECYCLE_JOURNAL_CLEAN_FAILED);
    assert(event_count == 3u && !memcmp(events, "MXE", 3u));
    reset();
    assert(s64_launcher_code_assets_end(&runtime) == S64_SESSION_LIFECYCLE_OK);
    assert(mode_calls == 0u && clear_calls == 0u && end_calls == 1u);
    assert(event_count == 1u && events[0] == 'E');
    assert(s64_launcher_code_assets_end(0) == S64_SESSION_LIFECYCLE_INVALID);

    puts("launcher code asset coordination tests passed");
    return 0;
}
