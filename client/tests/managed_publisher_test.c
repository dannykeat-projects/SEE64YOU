#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "platform/managed_publisher.h"
#include "platform/code_assets.h"
#include "platform/reu.h"
#include "platform/application_state.h"
#include "publisher/workspace.h"

S64PublisherWorkspace s64_publisher_workspace;
static uint8_t state_ok, wire_ok = 1u;
static S64ApplicationState loaded_state, stored_state;
static S64ModuleRuntime* expected_runtime;
static unsigned state_store_calls, control_store_calls, resume_calls;
static unsigned control_store_fail_at;
static uint8_t state_store_ok = 1u, resume_ok = 1u;
uint8_t s64_application_state_load_wire(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, uint8_t* wire)
{
    assert(control == &expected_runtime->reu_control);
    assert(journal == &expected_runtime->lifecycle.journal);
    assert(wire == s64_publisher_workspace.bytes);
    return wire_ok;
}
uint8_t s64_application_state_decode_payload(S64ApplicationState* state,
    const uint8_t* wire)
{
    assert(wire == s64_publisher_workspace.bytes);
    if (!state_ok) return 0u;
    *state = loaded_state;
    return 1u;
}
uint8_t s64_application_state_store(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const S64ApplicationState* state,
    uint8_t* wire)
{
    assert(control == &expected_runtime->reu_control);
    assert(journal == &expected_runtime->lifecycle.journal);
    assert(wire == s64_publisher_workspace.bytes);
    ++state_store_calls;
    if (!state_store_ok) return 0u;
    stored_state = *state;
    return 1u;
}
uint8_t s64_reu_runtime_store_verified(const S64ReuRuntimeControl* control)
{
    assert(control == &expected_runtime->reu_control);
    ++control_store_calls;
    return control_store_fail_at != control_store_calls;
}
S64SystemResult s64_module_runtime_resume_interactive(S64ModuleRuntime* runtime)
{
    assert(runtime == expected_runtime);
    ++resume_calls;
    if (!resume_ok) return S64_SYSTEM_APPLY_FAILED;
    runtime->system_state.mode = S64_SYSTEM_MODE_INTERACTIVE;
    return S64_SYSTEM_OK;
}

static unsigned init_calls, adopt_calls, mode_calls, bind_calls;
static unsigned char adopt_ok, mode_ok, bind_ok;
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
    assert(destination == S64_MODULE_PUBLISHER);
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
    assert(editor == &expected_runtime->module_manifest.modules[1]);
    assert(trusted_arrival == 1u);
    assert((uintptr_t)slot == S64_CODE_OVERLAY_ADDRESS);
    assert(capacity == S64_CODE_OVERLAY_CAPACITY);
    assert(reader == s64_reu_load);
    ++bind_calls;
    event('B');
    return bind_ok;
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
    state_store_calls = control_store_calls = resume_calls = 0;
    control_store_fail_at = 0u;
    state_store_ok = resume_ok = 1u;
    event_count = 0;
    adopt_ok = mode_ok = bind_ok = 1u;
    memset(events, 0, sizeof(events));
    memset(&stored_state, 0, sizeof(stored_state));
}

int main(void)
{
    S64ModuleRuntime runtime;
    S64Dos dos;
    S64Buffer body, before_body;
    S64Document document, before_document;
    uint8_t dirty = 99u, replaced = 99u;
    expected_runtime = &runtime;
    memset(&runtime, 0, sizeof(runtime));
    memset(&dos, 0, sizeof(dos));

    reset();
    assert(!s64_managed_publisher_adopt(0, &dos, 7u));
    assert(!s64_managed_publisher_adopt(&runtime, 0, 7u));
    assert(!s64_managed_publisher_adopt(&runtime, &dos, 0u));
    assert(init_calls == 0u && adopt_calls == 0u && mode_calls == 0u && bind_calls == 0u);

    reset();
    adopt_ok = 0u;
    assert(!s64_managed_publisher_adopt(&runtime, &dos, 7u));
    assert(init_calls == 1u && adopt_calls == 1u);
    assert(mode_calls == 0u && bind_calls == 0u);
    assert(event_count == 2u && !memcmp(events, "IA", 2u));

    reset();
    mode_ok = 0u;
    assert(!s64_managed_publisher_adopt(&runtime, &dos, 7u));
    assert(init_calls == 1u && adopt_calls == 1u && mode_calls == 1u);
    assert(bind_calls == 0u);
    assert(event_count == 3u && !memcmp(events, "IAM", 3u));

    reset();
    bind_ok = 0u;
    assert(!s64_managed_publisher_adopt(&runtime, &dos, 7u));
    assert(init_calls == 1u && adopt_calls == 1u && mode_calls == 1u);
    assert(bind_calls == 1u && event_count == 4u && !memcmp(events, "IAMB", 4u));

    reset();
    assert(s64_managed_publisher_adopt(&runtime, &dos, 7u));
    assert(init_calls == 1u && adopt_calls == 1u && mode_calls == 1u && bind_calls == 1u);
    assert(event_count == 4u && !memcmp(events, "IAMB", 4u));

    wire_ok = 0u;
    assert(!s64_managed_publisher_adopt(&runtime, &dos, 7u));
    wire_ok = 1u;

    memset(&body, 0x55, sizeof(body)); before_body = body;
    memset(&document, 0x55, sizeof(document)); before_document = document;
    runtime.current_module = S64_MODULE_PUBLISHER;
    state_ok = 0u;
    reset();
    assert(s64_managed_publisher_adopt(&runtime, &dos, 7u));
    assert(!s64_managed_publisher_document(&runtime, &body, &document, &dirty, &replaced));
    assert(!memcmp(&body, &before_body, sizeof(body)));
    assert(!memcmp(&document, &before_document, sizeof(document)));
    assert(dirty == 99u && replaced == 99u);
    state_ok = 1u;
    memset(&loaded_state, 0, sizeof(loaded_state));
    loaded_state.editor.buffer.gap_start = 42u;
    loaded_state.editor.dirty = 1u;
    loaded_state.document.type = S64_DOCUMENT_POST;
    strcpy(loaded_state.document.title, "Owned document");
    loaded_state.document_replaced = 1u;
    reset();
    assert(s64_managed_publisher_adopt(&runtime, &dos, 7u));
    assert(s64_managed_publisher_document(&runtime, &body, &document, &dirty, &replaced));
    assert(body.gap_start == 42u && document.body == &body);
    assert(document.type == S64_DOCUMENT_POST && !strcmp(document.title, "Owned document"));
    assert(dirty == 1u && replaced == 1u);
    assert(!s64_managed_publisher_document(&runtime, &body, &document, &dirty, &replaced));

    /* A no-op Publisher return advances authority/generation but preserves
     * the Editor UI/history/filename state. */
    loaded_state.generation = 10u;
    loaded_state.source_module = S64_MODULE_EDITOR;
    loaded_state.editor.history.count = 3u;
    strcpy(loaded_state.filename, "owned.s64");
    reset();
    assert(s64_managed_publisher_adopt(&runtime, &dos, 7u));
    assert(s64_managed_publisher_document(&runtime, &body, &document, &dirty, &replaced));
    runtime.current_module = S64_MODULE_PUBLISHER;
    runtime.lifecycle.active_record_written = 1u;
    runtime.lifecycle.clean_record_written = 0u;
    runtime.system_state.captured = 1u;
    runtime.system_state.restored = 0u;
    runtime.system_state.mode = S64_SYSTEM_MODE_INTERACTIVE;
    runtime.reu_control.state_flags = S64_REU_RUNTIME_STATE_APPLICATION_VALID;
    document.body = &body;
    assert(s64_managed_publisher_return_state(&runtime, &body, &document, 0u,
        s64_publisher_workspace.bytes) == 1u);
    assert(state_store_calls == 1u && control_store_calls == 2u && resume_calls == 1u);
    assert(stored_state.generation == 11u
        && stored_state.source_module == S64_MODULE_PUBLISHER);
    assert(stored_state.editor.history.count == 3u
        && !strcmp(stored_state.filename, "owned.s64"));
    assert(runtime.reu_control.state_flags & S64_REU_RUNTIME_STATE_APPLICATION_VALID);

    /* Replacement deliberately resets Editor position/history/local filename. */
    loaded_state.generation = 20u;
    loaded_state.source_module = S64_MODULE_EDITOR;
    loaded_state.editor.insert_mode = 1u;
    loaded_state.editor.word_wrap = 1u;
    loaded_state.editor.history.count = 4u;
    strcpy(loaded_state.filename, "old.s64");
    reset();
    assert(s64_managed_publisher_adopt(&runtime, &dos, 7u));
    assert(s64_managed_publisher_document(&runtime, &body, &document, &dirty, &replaced));
    body.gap_start = 7u;
    body.gap_end = S64_BUFFER_CAPACITY;
    body.cursor = 7u;
    strcpy(document.title, "Remote replacement");
    document.body = &body;
    runtime.current_module = S64_MODULE_PUBLISHER;
    runtime.lifecycle.active_record_written = 1u;
    runtime.lifecycle.clean_record_written = 0u;
    runtime.system_state.captured = 1u;
    runtime.system_state.restored = 0u;
    runtime.system_state.mode = S64_SYSTEM_MODE_INTERACTIVE;
    runtime.reu_control.state_flags = S64_REU_RUNTIME_STATE_APPLICATION_VALID;
    assert(s64_managed_publisher_return_state(&runtime, &body, &document, 1u,
        s64_publisher_workspace.bytes) == 1u);
    assert(stored_state.document_replaced && stored_state.editor.dirty);
    assert(stored_state.editor.buffer.gap_start == 7u
        && stored_state.editor.buffer.cursor == 0u);
    assert(!stored_state.editor.history.count && !stored_state.filename[0]);
    assert(stored_state.editor.insert_mode && stored_state.editor.word_wrap);

    /* Failed payload store leaves authority revoked but safely interactive. */
    loaded_state.generation = 30u;
    reset();
    assert(s64_managed_publisher_adopt(&runtime, &dos, 7u));
    assert(s64_managed_publisher_document(&runtime, &body, &document, &dirty, &replaced));
    runtime.current_module = S64_MODULE_PUBLISHER;
    runtime.lifecycle.active_record_written = 1u;
    runtime.system_state.captured = 1u;
    runtime.system_state.mode = S64_SYSTEM_MODE_INTERACTIVE;
    runtime.reu_control.state_flags = S64_REU_RUNTIME_STATE_APPLICATION_VALID;
    document.body = &body;
    state_store_ok = 0u;
    assert(s64_managed_publisher_return_state(&runtime, &body, &document, 0u,
        s64_publisher_workspace.bytes) == 0u);
    assert(!(runtime.reu_control.state_flags & S64_REU_RUNTIME_STATE_APPLICATION_VALID));
    assert(resume_calls == 1u);

    assert(!s64_managed_publisher_adopt(0, &dos, 7u));
    puts("managed publisher adoption/document tests passed");
    return 0;
}
