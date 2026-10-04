#include "platform/managed_editor_state.h"
#include "platform/application_state.h"
#include "platform/reu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t reu[0x40000], payload[0x20000], wire[1024], saved_wire[512];
static unsigned reads, writes, copies, fail_read;
static unsigned transaction, fail_stage, control_writes, control_reads, state_writes, state_reads;
static uint32_t state_address;
static uint8_t fail_history;
static S64ModuleRuntime runtime;
static S64Dos dos;
static S64Editor editor;
static S64Document document;
static S64LocalDocument local;
static S64Clipboard clipboard;
static char search[33];
static S64ApplicationState* decoded;
static S64ApplicationState snapshot;

uint8_t s64_reu_load(uint8_t bank, uint16_t offset, void* out, uint16_t length)
{
    uint32_t address = ((uint32_t)bank << 16) | offset;
    assert(address + length <= sizeof(reu)); ++reads;
    if (transaction && bank == S64_REU_RUNTIME_CONTROL_BANK && offset < S64_REU_RUNTIME_WIRE_SIZE) {
        ++control_reads;
        if ((fail_stage == 2 && control_writes == 1)
            || (fail_stage == 7 && control_writes == 2)
            || (fail_stage == 9 && control_writes >= 2)) return 0;
    }
    if (transaction && address >= state_address && address < state_address + 512) {
        ++state_reads;
        if (fail_stage == 4 && state_reads == 16) return 0;
    }
    if (fail_read) return 0;
    memcpy(out, reu + address, length);
    if (transaction && ((fail_stage == 5 && state_reads == 16 && address >= state_address && address < state_address + 512)
        || (fail_stage == 8 && bank == 2 && control_writes == 2))) ((uint8_t*)out)[length - 1] ^= 1;
    return 1;
}
uint8_t s64_reu_store(uint8_t bank, uint16_t offset, const void* in, uint16_t length)
{
    uint32_t address = ((uint32_t)bank << 16) | offset;
    assert(address + length <= sizeof(reu)); ++writes;
    if (fail_history && bank < 2) return 0;
    if (transaction && bank == S64_REU_RUNTIME_CONTROL_BANK && offset == 0) {
        ++control_writes;
        if ((fail_stage == 1 && control_writes == 1)
            || (fail_stage == 6 && control_writes == 2)) return 0;
        if (((const uint8_t*)in)[12] & S64_REU_RUNTIME_STATE_APPLICATION_VALID)
            assert(state_writes == 1 && state_reads == 16);
    }
    if (transaction && address == state_address) {
        assert(control_writes == 1 && control_reads == 1);
        assert(!(reu[0x20000 + 12] & S64_REU_RUNTIME_STATE_APPLICATION_VALID));
        ++state_writes;
        if (fail_stage == 3) return 0;
    }
    memcpy(reu + address, in, length); return 1;
}
static uint8_t publish(S64ManagedEditorState* authority)
{
    return s64_managed_editor_state_publish(&runtime, authority, &editor,
        &document, &local, &clipboard, search, 1, wire);
}
static void publication_tests(void)
{
    S64ManagedEditorState authority, before;
    S64ReuRuntimeControl persisted;
    S64Editor preserved_editor;
    unsigned failure;
    uint8_t old[512];
    runtime.lifecycle.active_record_written = 1;
    runtime.system_state.captured = 1;
    runtime.system_state.mode = S64_SYSTEM_MODE_IO_SAFE;
    state_address = s64_application_state_allocation(&runtime.reu_control)->address;
    memset(&authority, 0, sizeof(authority));
    runtime.reu_control.state_flags = 0;
    assert(s64_reu_runtime_store_verified(&runtime.reu_control));
    memcpy(payload, reu, sizeof(payload));
    transaction = 1;
    assert(publish(&authority));
    transaction = 0;
    assert(authority.generation == 1 && authority.source_module == S64_MODULE_EDITOR);
    assert(control_writes == 2 && control_reads == 2 && state_writes == 1 && state_reads == 16);
    assert(!memcmp(wire, reu + state_address, S64_APPLICATION_STATE_SIZE));
    assert(!memcmp(payload, reu, sizeof(payload)));
    assert(s64_reu_runtime_load(&persisted) && (persisted.state_flags & S64_REU_RUNTIME_STATE_APPLICATION_VALID));
    assert(s64_managed_editor_state_prepare(&runtime, wire) == 2);
    decoded = s64_managed_editor_state_decode(&runtime, wire);
    assert(decoded && decoded->generation == 1 && decoded->source_module == S64_MODULE_EDITOR);
    assert(!strcmp(decoded->search, search) && !strcmp(decoded->filename, local.filename));
    assert(!strcmp(decoded->document.title, document.title) && decoded->clipboard_length == clipboard.length);
    assert(decoded->editor.buffer.cursor == editor.buffer.cursor && !decoded->editor.history.pending_valid);
    memcpy(old, reu + state_address, sizeof(old));
    authority.document_replaced = 1;
    control_writes = control_reads = state_writes = state_reads = 0;
    transaction = 1; assert(publish(&authority)); transaction = 0;
    assert(authority.generation == 2 && authority.document_replaced == 1);
    assert(memcmp(old, reu + state_address, sizeof(old)));
    assert(s64_managed_editor_state_prepare(&runtime, wire) == 2);
    decoded = s64_managed_editor_state_decode(&runtime, wire);
    assert(decoded && decoded->generation == 2 && decoded->document_replaced);
    assert(s64_managed_editor_state_prepare(&runtime, wire) == 2);
    assert(s64_managed_editor_state_revoke(&runtime));
    assert(!s64_managed_editor_state_decode(&runtime, wire));

    /* No producer output before ownership, capture and IO_SAFE prerequisites. */
    before = authority;
    writes = 0; runtime.current_module = S64_MODULE_PUBLISHER;
    assert(!publish(&authority) && !writes); runtime.current_module = S64_MODULE_EDITOR;
    runtime.lifecycle.active_record_written = 0;
    assert(!publish(&authority) && !writes); runtime.lifecycle.active_record_written = 1;
    runtime.lifecycle.clean_record_written = 1;
    assert(!publish(&authority) && !writes); runtime.lifecycle.clean_record_written = 0;
    runtime.system_state.mode = S64_SYSTEM_MODE_INTERACTIVE;
    assert(!publish(&authority) && !writes); runtime.system_state.mode = S64_SYSTEM_MODE_IO_SAFE;
    authority.generation = 0xffffffffUL;
    assert(!publish(&authority) && !writes); authority = before;
    editor.history.pending_valid = 1;
    assert(!publish(&authority) && !writes); editor.history.pending_valid = 0;
    memset(search, 'x', sizeof(search));
    assert(!publish(&authority) && !writes); strcpy(search, "needle");
    /* Actual dirty-history flush failure must remain retryable. */
    assert(s64_editor_insert(&editor, 'd'));
    fail_history = 1; assert(!publish(&authority)); fail_history = 0;
    assert(authority.generation == before.generation);
    assert(s64_history_prepare_handoff(&editor.history));
    memcpy(payload, reu, sizeof(payload));

    preserved_editor = editor;
    authority.source_module = S64_MODULE_PUBLISHER; before = authority;
    for (failure = 1; failure <= 9; ++failure) {
        runtime.reu_control.state_flags = S64_REU_RUNTIME_STATE_APPLICATION_VALID;
        assert(s64_reu_runtime_store_verified(&runtime.reu_control));
        memcpy(old, reu + state_address, sizeof(old));
        control_writes = control_reads = state_writes = state_reads = 0;
        fail_stage = failure; transaction = 1;
        assert(!publish(&authority));
        transaction = 0; fail_stage = 0;
        assert(authority.generation == before.generation && authority.source_module == before.source_module);
        assert(!(runtime.reu_control.state_flags & S64_REU_RUNTIME_STATE_APPLICATION_VALID));
        if (failure <= 2) assert(!state_writes && !memcmp(old, reu + state_address, sizeof(old)));
        /* Failed initial revocation can retain the original authority; failed
         * final readback + failed cleanup is explicitly uncertain. Neither
         * grants a transition. Retrying revoke must persist a verified clear. */
        if (failure != 1 && failure != 9) {
            assert(s64_reu_runtime_load(&persisted));
            assert(!(persisted.state_flags & S64_REU_RUNTIME_STATE_APPLICATION_VALID));
        }
        assert(!memcmp(payload, reu, sizeof(payload)) && s64_buffer_length(&editor.buffer) == 4);
        assert(!memcmp(&preserved_editor, &editor, sizeof(editor)));
        assert(s64_managed_editor_state_revoke(&runtime));
        assert(s64_reu_runtime_load(&persisted) && !persisted.state_flags);
    }
    /* Invalid encode must leave the revoked authority clear. */
    document.status = (S64DocumentStatus)0;
    assert(!publish(&authority)); document.status = S64_DOCUMENT_PUBLISHED;
    assert(authority.generation == before.generation);
    assert(s64_reu_runtime_load(&persisted) && !persisted.state_flags);
    /* Revocation must not touch an already loaded exit snapshot/workspace. */
    memset(wire, 0x66, sizeof(wire));
    assert(s64_managed_editor_state_revoke(&runtime));
    for (failure = 0; failure < sizeof(wire); ++failure) assert(wire[failure] == 0x66);
    assert(s64_editor_undo(&editor) && s64_editor_redo(&editor));
    puts("managed editor publication/revocation tests passed");
}
uint8_t s64_reu_copy(uint8_t from_bank, uint16_t from, uint8_t to_bank,
    uint16_t to, uint16_t length)
{
    ++copies;
    memmove(reu + ((uint32_t)to_bank << 16) + to,
        reu + ((uint32_t)from_bank << 16) + from, length); return 1;
}
static void no_payload_writes(void)
{
    assert(!writes && !copies);
    assert(!memcmp(payload, reu, sizeof(payload)));
}
static void reject_prepare(void)
{
    uint8_t prior[512];
    memcpy(prior, wire + 512, sizeof(prior));
    reads = writes = copies = 0;
    assert(!s64_managed_editor_state_prepare(&runtime, wire));
    assert(!s64_managed_editor_state_decode(&runtime, wire));
    assert(!memcmp(prior, wire + 512, sizeof(prior)));
    no_payload_writes();
}

int main(void)
{
    uint32_t address;
    unsigned i;
    S64SessionJournal active;
    S64ReuRuntimeControl control;
    unsigned char clip[4];
    memset(reu, 0xa5, sizeof(reu));
    memset(&runtime, 0, sizeof(runtime)); runtime.dos = &dos;
    runtime.current_module = S64_MODULE_EDITOR;
    s64_reu_runtime_init(&runtime.reu_control, 514, 123);
    runtime.lifecycle.journal.state = S64_SESSION_STATE_ACTIVE;
    runtime.lifecycle.journal.package_version = 514;
    runtime.lifecycle.journal.session_id = 123;
    runtime.lifecycle.journal.sequence = 7;
    runtime.lifecycle.journal.recovery_ref = 42;
    runtime.lifecycle.journal.restore_flags = S64_SESSION_JOURNAL_RESTORE_TURBO;
    runtime.lifecycle.journal.turbo_control = 2;
    active = runtime.lifecycle.journal;
    assert(s64_application_state_reserve(&runtime.reu_control));
    address = s64_application_state_allocation(&runtime.reu_control)->address;
    memcpy(payload, reu, sizeof(payload));

    /* Clear validity is the only fresh-entry path; it ignores stale record bytes. */
    memset(wire, 0x77, sizeof(wire));
    assert(s64_managed_editor_state_prepare(&runtime, wire) == 1);
    assert(!s64_managed_editor_state_decode(&runtime, wire));
    assert(!reads); no_payload_writes();
    assert(wire[512] == 0x77);
    memset(&editor, 0, sizeof(editor));
    s64_editor_init(&editor); s64_document_init(&document, &editor.buffer);
    s64_local_init(&local, &dos, &document); s64_clipboard_init(&clipboard);

    assert(s64_editor_insert(&editor, 'a')); assert(s64_editor_insert(&editor, 'b'));
    assert(s64_editor_insert(&editor, 'c'));
    strcpy(document.slug, "resume"); strcpy(document.title, "Preserved metadata");
    strcpy(document.date, "2026-10-04"); document.order = -42;
    document.type = S64_DOCUMENT_POST; document.status = S64_DOCUMENT_PUBLISHED;
    strcpy(local.filename, "resume.s64"); strcpy(search, "needle");
    assert(s64_clipboard_write(&clipboard, 0, (const unsigned char*)"clip", 4)); clipboard.length = 4;
    editor.selection_anchor = 1; editor.selection_valid = 1; editor.mark_mode = 1;
    editor.view_start = 1; editor.view_column = 2;
    assert(s64_application_state_capture(&snapshot, &editor, &document, &local, &clipboard, search, 1, 9));
    snapshot.source_module = S64_MODULE_PUBLISHER;
    snapshot.document_replaced = 1;
    assert(s64_application_state_store(&runtime.reu_control, &active, &snapshot, wire));
    memcpy(saved_wire, wire, sizeof(saved_wire));
    runtime.reu_control.state_flags = S64_REU_RUNTIME_STATE_APPLICATION_VALID;
    control = runtime.reu_control;
    memcpy(payload, reu, sizeof(payload)); reads = writes = copies = 0;
    assert(s64_managed_editor_state_prepare(&runtime, wire) == 2);
    decoded = s64_managed_editor_state_decode(&runtime, wire);
    assert(decoded == (S64ApplicationState*)(wire + 512));
    assert(reads == 1); no_payload_writes();
    /* Compare fields: host struct padding is not part of the wire contract. */
#define SAME(field) assert(decoded->editor.field == snapshot.editor.field)
    SAME(buffer.gap_start); SAME(buffer.gap_end); SAME(buffer.cursor);
    SAME(history.start); SAME(history.count); SAME(history.cursor);
    SAME(history.pending_valid); SAME(history.saved_valid); SAME(history.saved_cursor);
    SAME(view_start); SAME(view_column); SAME(line); SAME(column); SAME(position_cursor);
    SAME(selection_anchor); SAME(preferred_column); SAME(view_cursor_row); SAME(view_cursor_column);
    SAME(selection_valid); SAME(preferred_column_valid); SAME(mark_mode); SAME(dirty);
    SAME(insert_mode); SAME(word_wrap);
#undef SAME
    assert(!strcmp(decoded->filename, snapshot.filename) && decoded->document_replaced);
    assert(!strcmp(decoded->search, snapshot.search) && decoded->search_valid);
    assert(decoded->clipboard_length == snapshot.clipboard_length);
    assert(decoded->document.type == snapshot.document.type && decoded->document.status == snapshot.document.status);
    assert(decoded->document.order == snapshot.document.order && !strcmp(decoded->document.slug, snapshot.document.slug));
    assert(!strcmp(decoded->document.title, snapshot.document.title) && !strcmp(decoded->document.date, snapshot.document.date));
    assert(decoded->generation == 9 && decoded->source_module == S64_MODULE_PUBLISHER);
    assert(!s64_managed_editor_state_decode(&runtime, wire)); no_payload_writes();

    /* Payload range validation runs before writing decoded scratch. */
    {
        uint8_t prior[512];
        memcpy(prior, wire + 512, sizeof(prior));
        assert(s64_managed_editor_state_prepare(&runtime, wire) == 2);
        wire[337] = 1;
        assert(!s64_managed_editor_state_decode(&runtime, wire));
        assert(!memcmp(prior, wire + 512, sizeof(prior)));
        no_payload_writes();
    }
    fail_read = 1; reject_prepare(); fail_read = 0;
    runtime.reu_control.allocation_count = 0; reject_prepare(); runtime.reu_control = control;
    for (i = 0; i < sizeof(saved_wire); ++i) {
        reu[address + i] ^= 1; reject_prepare(); reu[address + i] ^= 1;
    }
    ++runtime.lifecycle.journal.sequence; reject_prepare(); runtime.lifecycle.journal = active;
    ++runtime.lifecycle.journal.session_id; reject_prepare(); runtime.lifecycle.journal = active;
    ++runtime.lifecycle.journal.package_version; reject_prepare(); runtime.lifecycle.journal = active;
    ++runtime.lifecycle.journal.recovery_ref; reject_prepare(); runtime.lifecycle.journal = active;
    runtime.lifecycle.journal.turbo_control ^= 1; reject_prepare(); runtime.lifecycle.journal = active;
    runtime.lifecycle.journal.state = S64_SESSION_STATE_CLEAN; reject_prepare(); runtime.lifecycle.journal = active;
    runtime.current_module = S64_MODULE_PUBLISHER; reject_prepare(); runtime.current_module = S64_MODULE_EDITOR;
    memset(reu + address, 0, sizeof(saved_wire)); reject_prepare();
    memcpy(reu + address, saved_wire, sizeof(saved_wire));

    /* The preserved controls refer to usable payload, including flushed history. */
    clipboard.length = decoded->clipboard_length;
    assert(s64_clipboard_read(&clipboard, 0, clip, sizeof(clip)) == 4 && !memcmp(clip, "clip", 4));
    assert(s64_editor_undo(&decoded->editor)); assert(!s64_buffer_length(&decoded->editor.buffer));
    assert(s64_editor_redo(&decoded->editor)); assert(s64_buffer_length(&decoded->editor.buffer) == 3);
    editor = decoded->editor;
    publication_tests();
    puts("managed editor state tests passed"); return 0;
}
