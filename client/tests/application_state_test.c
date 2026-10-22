#include "platform/application_state.h"
#include "platform/crc32.h"
#include "platform/reu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t reu[0x40000], wire[512], saved_wire[512];
static unsigned fail_read, fail_write, corrupt_read, reads, writes;
static S64ReuRuntimeControl control;
static S64SessionJournal journal;
static S64ApplicationState state, output, before;

uint8_t s64_reu_load(uint8_t bank, uint16_t offset, void* out, uint16_t length)
{
    uint32_t address = ((uint32_t)bank << 16) | offset;
    assert(address + length <= sizeof(reu));
    ++reads;
    if (fail_read) return 0;
    memcpy(out, reu + address, length);
    if (corrupt_read) ((uint8_t*)out)[0] ^= 1;
    return 1;
}
uint8_t s64_reu_store(uint8_t bank, uint16_t offset, const void* in, uint16_t length)
{
    uint32_t address = ((uint32_t)bank << 16) | offset;
    assert(address + length <= sizeof(reu));
    ++writes;
    if (fail_write) return 0;
    memcpy(reu + address, in, length);
    return 1;
}
/* DMA primitive exercised by the real REU buffer/history/editor implementation. */
uint8_t s64_reu_copy(uint8_t from_bank, uint16_t from, uint8_t to_bank,
    uint16_t to, uint16_t length)
{
    memmove(reu + ((uint32_t)to_bank << 16) + to,
        reu + ((uint32_t)from_bank << 16) + from, length);
    return 1;
}
static void checksum(void)
{
    uint32_t crc = s64_crc32_update(0xffffffffUL, wire, 508) ^ 0xffffffffUL;
    unsigned i;
    for (i = 0; i < 4; ++i) wire[508 + i] = (uint8_t)(crc >> (8 * i));
}
static void reject_decode(void)
{
    memset(&output, 0x77, sizeof(output)); before = output;
    assert(!s64_application_state_decode(&control, &journal, &output, wire));
    assert(!memcmp(&output, &before, sizeof(output)));
}
static void reject_load(void)
{
    memset(&output, 0x88, sizeof(output)); before = output;
    assert(!s64_application_state_load(&control, &journal, &output, wire));
    assert(!memcmp(&output, &before, sizeof(output)));
}
static void round_trip(void)
{
    assert(s64_application_state_store(&control, &journal, &state, wire));
    memset(&output, 0, sizeof(output));
    assert(s64_application_state_load(&control, &journal, &output, wire));
    assert(!memcmp(&output, &state, sizeof(state)));
}
int main(void)
{
    S64Editor editor;
    S64Document document;
    S64LocalDocument local;
    S64Clipboard clipboard;
    S64ReuRuntimeControl original;
    S64SessionJournal active;
    S64Buffer replacement;
    char search[33] = "query";
    uint32_t address;
    unsigned i;

    s64_reu_runtime_init(&control, 514, 123);
    memset(&journal, 0, sizeof(journal));
    journal.state = S64_SESSION_STATE_ACTIVE; journal.package_version = 514;
    journal.session_id = 123; journal.sequence = 7; journal.recovery_ref = 42;
    journal.restore_flags = S64_SESSION_JOURNAL_RESTORE_TURBO; journal.turbo_control = 2;
    active = journal;
    assert(s64_application_state_reserve(&control));
    assert(control.allocation_count == 1);
    address = s64_application_state_allocation(&control)->address;
    assert(!(address & 255) && control.allocations[0].owner == S64_REU_OWNER_RUNTIME);
    original = control;
    assert(!s64_application_state_reserve(&control));
    assert(!memcmp(&control, &original, sizeof(control)));
    memset(&editor, 0, sizeof(editor));
    s64_editor_init(&editor);
    assert(s64_editor_insert(&editor, 'a'));
    assert(s64_editor_insert(&editor, 'b'));
    assert(s64_editor_insert(&editor, 'c'));
    s64_document_init(&document, &editor.buffer);
    strcpy(document.slug, "test"); strcpy(document.title, "State test");
    strcpy(document.date, "2026-10-04"); document.order = -100;
    memset(&local, 0, sizeof(local)); strcpy(local.filename, "test.s64");
    s64_clipboard_init(&clipboard);
    assert(s64_clipboard_write(&clipboard, 0, (const unsigned char*)"clip", 4));
    clipboard.length = 4;
    editor.selection_anchor = 1; editor.selection_valid = 1; editor.mark_mode = 1;
    editor.preferred_column = 2; editor.preferred_column_valid = 1;
    editor.view_start = 1; editor.view_column = 2;
    editor.view_cursor_row = 2; editor.view_cursor_column = 3;
    assert(s64_application_state_capture(&state, &editor, &document, &local,
        &clipboard, search, 1, 9));
    assert(!editor.history.pending_valid && state.editor.history.count);
    round_trip(); /* includes every Editor field, metadata, clipboard and search */
    assert(s64_editor_undo(&output.editor)); /* history metadata was flushed */
    assert(s64_buffer_length(&output.editor.buffer) == 0);
    assert(s64_editor_redo(&output.editor));
    assert(s64_buffer_length(&output.editor.buffer) == 3);
    memcpy(saved_wire, wire, sizeof(wire));

    /* Every byte participates in validation, including reserved bytes and CRC. */
    for (i = 0; i < sizeof(wire); ++i) {
        memcpy(wire, saved_wire, sizeof(wire)); wire[i] ^= 1; reject_decode();
    }
    memcpy(wire, saved_wire, sizeof(wire)); wire[337] = 1; checksum(); reject_decode();
    memcpy(wire, saved_wire, sizeof(wire)); wire[49] = 1; checksum(); reject_decode();
    memcpy(wire, saved_wire, sizeof(wire)); wire[47] = 17; checksum(); reject_decode();
    memcpy(wire, saved_wire, sizeof(wire)); wire[67] = 40; checksum(); reject_decode();
    memcpy(wire, saved_wire, sizeof(wire)); wire[44] = 100; checksum(); reject_decode();
    memcpy(wire, saved_wire, sizeof(wire)); memset(wire + 303, 'x', 33); checksum(); reject_decode();
    memcpy(wire, saved_wire, sizeof(wire)); wire[76] = 3; checksum(); reject_decode();
    memcpy(wire, saved_wire, sizeof(wire));
    journal.state = S64_SESSION_STATE_CLEAN; reject_decode(); reject_load(); journal = active;
    ++journal.sequence; reject_decode(); reject_load(); journal = active;
    ++journal.session_id; reject_decode(); reject_load(); journal = active;
    ++journal.package_version; reject_decode(); reject_load(); journal = active;
    ++journal.recovery_ref; reject_decode(); reject_load(); journal = active;
    journal.turbo_control ^= 1; reject_decode(); reject_load(); journal = active;

    control.allocations[0].owner = S64_REU_OWNER_EDITOR; reject_load(); control = original;
    control.allocations[0].flags = 5; reject_load(); control = original;
    --control.allocations[0].length; reject_load(); control = original;
    ++control.allocations[0].address; ++control.next_free; reject_load(); control = original;
    assert(s64_reu_runtime_allocate(&control, S64_REU_OWNER_RUNTIME, 512, 256, &control.allocations[1]));
    control.allocations[1].flags = 6; reject_load(); control = original;
    fail_read = 1; reject_load(); fail_read = 0;
    reu[address] ^= 1; reject_load(); reu[address] ^= 1;
    fail_write = 1; assert(!s64_application_state_store(&control, &journal, &state, wire)); fail_write = 0;
    fail_read = 1; assert(!s64_application_state_store(&control, &journal, &state, wire)); fail_read = 0;
    corrupt_read = 1; assert(!s64_application_state_store(&control, &journal, &state, wire)); corrupt_read = 0;

    /* Pending history and dirty-cache flush failures never publish a snapshot. */
    before = state; editor.history.pending_valid = 1;
    assert(!s64_application_state_capture(&state, &editor, &document, &local,
        &clipboard, search, 1, 10));
    assert(!memcmp(&state, &before, sizeof(state))); editor.history.pending_valid = 0;
    assert(s64_editor_insert(&editor, 'd'));
    fail_write = 1;
    assert(!s64_application_state_capture(&state, &editor, &document, &local,
        &clipboard, search, 1, 10));
    assert(!memcmp(&state, &before, sizeof(state))); fail_write = 0;
    assert(s64_application_state_capture(&state, &editor, &document, &local,
        &clipboard, search, 1, 10));
    round_trip();

    /* A no-op Publisher return changes writer/generation only, preserving UI. */
    before = state; state.source_module = S64_MODULE_PUBLISHER; ++state.generation;
    round_trip();
    assert(!memcmp(&state.editor, &before.editor, sizeof(state.editor)));
    assert(!memcmp(state.filename, before.filename, sizeof(state.filename)));
    assert(!state.document_replaced);
    replacement.gap_start = 7; replacement.gap_end = S64_BUFFER_CAPACITY;
    replacement.cursor = 7; strcpy(document.slug, "remote");
    assert(s64_application_state_replace(&state, &replacement, &document));
    assert(state.document_replaced && state.generation == 12 && state.editor.dirty);
    assert(state.editor.buffer.cursor == 0 && state.editor.buffer.gap_start == 7);
    assert(state.editor.line == 1 && state.editor.column == 1);
    assert(!state.editor.history.count && !state.editor.history.saved_valid);
    assert(!state.editor.view_start && !state.editor.selection_valid && !state.editor.mark_mode);
    assert(!state.filename[0] && !strcmp(state.document.slug, "remote"));
    assert(state.clipboard_length == 4 && !memcmp(reu + 0x10000, "clip", 4));
    assert(!strcmp(state.search, search) && state.search_valid);
    round_trip();
    state.generation = 0xffffffffUL; before = state;
    assert(!s64_application_state_replace(&state, &replacement, &document));
    assert(!memcmp(&state, &before, sizeof(state)));
    state = before;
    state.document.type = (S64DocumentType)257;
    assert(!s64_application_state_encode(&control, &journal, &state, wire));
    state = before;
    state.editor.buffer.gap_start = 65536u;
    assert(!s64_application_state_encode(&control, &journal, &state, wire));
    /* Three module caches + module session + capsule + SYS return + both
     * three-overlay/session namespaces + the single application record. */
    s64_reu_runtime_init(&control, 514, 123);
    for (i = 0; i < 14; ++i) {
        S64ReuAllocation item;
        assert(s64_reu_runtime_allocate(&control, S64_REU_OWNER_MODULE, 256, 256, &item));
    }
    assert(s64_application_state_reserve(&control));
    assert(control.allocation_count == 15);
    original = control;
    assert(!s64_application_state_reserve(&control));
    assert(!memcmp(&control, &original, sizeof(control)));
    for (i = 15; i < 16; ++i) {
        S64ReuAllocation item;
        assert(s64_reu_runtime_allocate(&control, S64_REU_OWNER_PREVIEW, 256, 256, &item));
    }
    /* Exhausted reservation is atomic, even with no matching flag. */
    control.allocations[14].flags = 0; original = control;
    assert(!s64_application_state_reserve(&control));
    assert(!memcmp(&control, &original, sizeof(control)));
    /* After undo, prepare must fetch the earlier entry rather than the cached
     * undone entry. A failed fetch must not reset history and report success. */
    s64_editor_init(&editor);
    assert(s64_editor_insert(&editor, 'a'));
    assert(s64_history_prepare_handoff(&editor.history));
    assert(s64_editor_insert(&editor, 'b'));
    assert(s64_history_prepare_handoff(&editor.history));
    assert(s64_editor_undo(&editor));
    assert(editor.history.count == 2 && editor.history.cursor == 1);
    before = state;
    fail_read = 1;
    assert(!s64_application_state_capture(&state, &editor, &document, &local,
        &clipboard, search, 1, 20));
    assert(editor.history.count == 2 && editor.history.cursor == 1);
    assert(!memcmp(&state, &before, sizeof(state)));
    fail_read = 0;
    assert(s64_history_prepare_handoff(&editor.history));
    assert(reads && writes);
    puts("Shared application-state wire/session/ownership/history/replacement tests passed");
    return 0;
}
