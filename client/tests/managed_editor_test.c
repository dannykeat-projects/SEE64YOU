#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "platform/managed_editor.h"
#include "platform/code_assets.h"
#include "platform/reu.h"

static unsigned init_calls, adopt_calls, mode_calls, bind_calls;
static unsigned clear_calls, end_calls;
static unsigned char adopt_ok, mode_ok, bind_ok, clear_ok;
static S64SessionLifecycleResult end_result;
static char events[16];
static unsigned event_count;

static void event(char value)
{
    assert(event_count < sizeof(events));
    events[event_count++] = value;
}

void s64_module_runtime_init_hardware(S64ModuleRuntime* runtime, S64Dos* dos)
{
    assert(runtime != 0 && dos != 0);
    ++init_calls;
    event('I');
}

uint8_t s64_module_runtime_adopt(S64ModuleRuntime* runtime,
    S64ModuleId destination, uint16_t package_version, uint8_t trusted_arrival)
{
    assert(runtime != 0);
    assert(destination == S64_MODULE_EDITOR);
    assert(package_version == 7u);
    assert(trusted_arrival == 1u);
    ++adopt_calls;
    event('A');
    return adopt_ok;
}

S64SystemResult s64_system_state_set_mode(S64SystemState* state,
    S64SystemMode mode)
{
    assert(state != 0 && mode == S64_SYSTEM_MODE_IO_SAFE);
    ++mode_calls;
    event('M');
    return mode_ok ? S64_SYSTEM_OK : S64_SYSTEM_APPLY_FAILED;
}

uint8_t s64_code_assets_bind(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const S64ModuleDescriptor* editor,
    uint8_t trusted_arrival, void* slot, uint16_t capacity,
    S64CodeOverlayReader reader)
{
    assert(control != 0 && journal != 0 && editor != 0);
    assert(trusted_arrival == 1u);
    assert((uintptr_t)slot == S64_CODE_OVERLAY_ADDRESS);
    assert(capacity == S64_CODE_OVERLAY_CAPACITY);
    assert(reader == s64_reu_load);
    ++bind_calls;
    event('B');
    return bind_ok;
}

uint8_t s64_code_assets_clear(const S64ReuRuntimeControl* control)
{
    assert(control != 0);
    ++clear_calls;
    event('C');
    return clear_ok;
}

S64SessionLifecycleResult s64_module_runtime_end(S64ModuleRuntime* runtime)
{
    assert(runtime != 0);
    ++end_calls;
    event('E');
    if (end_result == S64_SESSION_LIFECYCLE_JOURNAL_CLEAN_FAILED)
        runtime->shared_invalidated = 1u;
    return end_result;
}

uint8_t s64_reu_load(uint8_t bank, uint16_t offset, void* destination,
    uint16_t length)
{
    (void)bank;
    (void)offset;
    (void)destination;
    (void)length;
    return 1u;
}

static void reset(void)
{
    init_calls = adopt_calls = mode_calls = bind_calls = 0;
    clear_calls = end_calls = event_count = 0;
    adopt_ok = mode_ok = bind_ok = clear_ok = 1u;
    end_result = S64_SESSION_LIFECYCLE_OK;
    memset(events, 0, sizeof(events));
}

int main(void)
{
    S64ModuleRuntime runtime;
    S64Dos dos;
    memset(&runtime, 0, sizeof(runtime));
    memset(&dos, 0, sizeof(dos));

    reset();
    assert(!s64_managed_editor_adopt(0, &dos, 7u));
    assert(!s64_managed_editor_adopt(&runtime, 0, 7u));
    assert(!s64_managed_editor_adopt(&runtime, &dos, 0u));
    assert(init_calls == 0u && adopt_calls == 0u && mode_calls == 0u && bind_calls == 0u);

    reset();
    adopt_ok = 0u;
    assert(!s64_managed_editor_adopt(&runtime, &dos, 7u));
    assert(init_calls == 1u && adopt_calls == 1u);
    assert(mode_calls == 0u && bind_calls == 0u);
    assert(event_count == 2u && !memcmp(events, "IA", 2u));

    reset();
    mode_ok = 0u;
    assert(!s64_managed_editor_adopt(&runtime, &dos, 7u));
    assert(init_calls == 1u && adopt_calls == 1u && mode_calls == 1u);
    assert(bind_calls == 0u);
    assert(event_count == 3u && !memcmp(events, "IAM", 3u));

    reset();
    bind_ok = 0u;
    assert(!s64_managed_editor_adopt(&runtime, &dos, 7u));
    assert(init_calls == 1u && adopt_calls == 1u && mode_calls == 1u);
    assert(bind_calls == 1u && event_count == 4u && !memcmp(events, "IAMB", 4u));

    reset();
    assert(s64_managed_editor_adopt(&runtime, &dos, 7u));
    assert(init_calls == 1u && adopt_calls == 1u && mode_calls == 1u && bind_calls == 1u);
    assert(event_count == 4u && !memcmp(events, "IAMB", 4u));

    reset();
    assert(s64_managed_editor_end(0) == S64_SESSION_LIFECYCLE_INVALID);
    assert(mode_calls == 0u && clear_calls == 0u && end_calls == 0u);

    reset();
    mode_ok = 0u;
    assert(s64_managed_editor_end(&runtime) == S64_SESSION_LIFECYCLE_RESTORE_FAILED);
    assert(mode_calls == 1u && clear_calls == 0u && end_calls == 0u);
    assert(event_count == 1u && events[0] == 'M');

    reset();
    clear_ok = 0u;
    assert(s64_managed_editor_end(&runtime) == S64_SESSION_LIFECYCLE_RESTORE_FAILED);
    assert(mode_calls == 1u && clear_calls == 1u && end_calls == 0u);
    assert(event_count == 2u && !memcmp(events, "MC", 2u));

    reset();
    end_result = S64_SESSION_LIFECYCLE_JOURNAL_CLEAN_FAILED;
    assert(s64_managed_editor_end(&runtime) == S64_SESSION_LIFECYCLE_JOURNAL_CLEAN_FAILED);
    assert(mode_calls == 1u && clear_calls == 1u && end_calls == 1u);
    assert(event_count == 3u && !memcmp(events, "MCE", 3u));
    reset();
    end_result = S64_SESSION_LIFECYCLE_OK;
    assert(s64_managed_editor_end(&runtime) == S64_SESSION_LIFECYCLE_OK);
    assert(mode_calls == 0u && clear_calls == 0u && end_calls == 1u);
    assert(event_count == 1u && events[0] == 'E');

    puts("managed editor adoption/exit tests passed");
    return 0;
}
