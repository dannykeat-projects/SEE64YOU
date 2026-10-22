/* Host simulator diagnostic: use cc65's real C64 model and native REU ABI. */
#include "editor.h"
#include "charset.h"
#include "document.h"
#include "platform/application_state.h"
#include "platform/reu.h"
#include "platform/crc32.h"
#include "platform/package_version.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned char page[256];
static unsigned int metadata_extent;
static S64ReuRuntimeControl control;
static S64SessionJournal journal;
static S64Editor editor;
static S64Document document;
static S64LocalDocument local;
static S64Clipboard clipboard;
static S64ApplicationState state, decoded;
static unsigned char wire[512], transfer[64];
static char search[33];

uint8_t s64_reu_load(uint8_t bank, uint16_t offset, void* out, uint16_t length)
{
    unsigned long address = ((unsigned long)bank << 16) + offset;
    unsigned int position, count;
    unsigned char* bytes = out;
    char name[16];
    FILE* file;
    assert(address + length <= 0x40000UL);
    while (length) {
        position = (unsigned int)(address & 255UL);
        count = 256u - position; if (count > length) count = length;
        sprintf(name, "P%04X.REU", (unsigned int)(address >> 8));
        memset(page, 0, sizeof(page));
        file = fopen(name, "rb");
        if (file) { assert(fread(page, 1, sizeof(page), file) == sizeof(page)); assert(!fclose(file)); }
        memcpy(bytes, page + position, count);
        bytes += count; address += count; length -= count;
    }
    return 1;
}

uint8_t s64_reu_store(uint8_t bank, uint16_t offset, const void* in, uint16_t length)
{
    unsigned long address = ((unsigned long)bank << 16) + offset;
    unsigned int extent, position, count;
    const unsigned char* bytes = in;
    char name[16];
    FILE* file;
    assert(address + length <= 0x40000UL);
    if (bank == S64_REU_HISTORY_META_BANK && offset >= S64_REU_HISTORY_META_BASE
        && (unsigned long)offset + length <= S64_REU_HISTORY_META_LIMIT) {
        extent = offset - S64_REU_HISTORY_META_BASE + length;
        if (extent > metadata_extent) metadata_extent = extent;
    }
    while (length) {
        position = (unsigned int)(address & 255UL);
        count = 256u - position; if (count > length) count = length;
        sprintf(name, "P%04X.REU", (unsigned int)(address >> 8));
        memset(page, 0, sizeof(page));
        file = fopen(name, "rb");
        if (file) { assert(fread(page, 1, sizeof(page), file) == sizeof(page)); assert(!fclose(file)); }
        memcpy(page + position, bytes, count);
        file = fopen(name, "wb"); assert(file);
        assert(fwrite(page, 1, sizeof(page), file) == sizeof(page)); assert(!fclose(file));
        bytes += count; address += count; length -= count;
    }
    return 1;
}

uint8_t s64_reu_copy(uint8_t from_bank, uint16_t from, uint8_t to_bank,
    uint16_t to, uint16_t length)
{
    unsigned long source = ((unsigned long)from_bank << 16) + from;
    unsigned long destination = ((unsigned long)to_bank << 16) + to;
    unsigned int count, position = 0;
    unsigned long a, b;
    unsigned char backwards = destination > source && destination < source + length;
    while (length) {
        count = length > sizeof(transfer) ? sizeof(transfer) : length;
        if (backwards) {
            position = length - count;
        }
        a = source + position; b = destination + position;
        if (!s64_reu_load((uint8_t)(a >> 16), (uint16_t)a, transfer, count)
            || !s64_reu_store((uint8_t)(b >> 16), (uint16_t)b, transfer, count)) return 0;
        length -= count;
        if (!backwards) {
            source += count; destination += count;
        }
        position = 0;
    }
    return 1;
}

/* The fixture uses journal encoding only; any accidental filesystem call fails. */
S64DosResult s64_dos_open_read(S64Dos* d, const char* n) { (void)d; (void)n; assert(0); return S64_DOS_TRANSPORT_ERROR; }
S64DosResult s64_dos_open_write(S64Dos* d, const char* n) { return s64_dos_open_read(d, n); }
S64DosResult s64_dos_close(S64Dos* d) { return s64_dos_open_read(d, 0); }
S64DosResult s64_dos_change_dir(S64Dos* d, const char* n) { return s64_dos_open_read(d, n); }
S64DosResult s64_dos_file_exists(S64Dos* d, const char* n, uint8_t* e) { (void)e; return s64_dos_open_read(d, n); }
S64DosResult s64_dos_write(S64Dos* d, const uint8_t* p, uint16_t n) { (void)p; (void)n; return s64_dos_open_read(d, 0); }
S64DosResult s64_dos_read(S64Dos* d, uint16_t n, S64UciDataSink s, void* c, uint16_t* r)
{ (void)n; (void)s; (void)c; (void)r; return s64_dos_open_read(d, 0); }

static void native(char* out, const char* ascii)
{
    while (*ascii) {
        assert(s64_charset_ascii_to_petscii((uint8_t)*ascii++, (uint8_t*)out++));
    }
    *out = 0;
}

static void expect_body(const char* ascii)
{
    unsigned int i;
    unsigned char value;
    assert(s64_buffer_length(&editor.buffer) == strlen(ascii));
    for (i = 0; ascii[i]; ++i) {
        assert(s64_charset_native_to_ascii(s64_buffer_char_at(&editor.buffer, i), &value));
        assert(value == (unsigned char)ascii[i]);
    }
}

static void export_file(FILE* header, const char* name, const char* macro,
    unsigned long address, unsigned int length)
{
    FILE* out = fopen(name, "wb");
    unsigned int count, remaining = length;
    unsigned long crc = 0xffffffffUL;
    assert(out);
    while (remaining) {
        count = remaining > sizeof(transfer) ? sizeof(transfer) : remaining;
        assert(s64_reu_load((uint8_t)(address >> 16), (uint16_t)address, transfer, count));
        assert(fwrite(transfer, 1, count, out) == count);
        crc = s64_crc32_update(crc, transfer, count);
        remaining -= count; address += count;
    }
    assert(!fclose(out));
    fprintf(header, "#define S64_RESUME_%s_LENGTH %uU\n", macro, length);
    fprintf(header, "#define S64_RESUME_%s_CRC 0x%08lXUL\n", macro, crc ^ 0xffffffffUL);
}

int main(void)
{
    FILE* header, *out;
    unsigned int i;
    char body[64];
    unsigned long state_crc;
    assert(sizeof(unsigned int) == 2);
    s64_editor_init(&editor);
    s64_document_init(&document, &editor.buffer);
    s64_clipboard_init(&clipboard);
    native(body, "RESUME GATE\nMARKER\n");
    for (i = 0; body[i]; ++i) assert(s64_editor_insert(&editor, (unsigned char)body[i]));
    s64_history_break_coalesce(&editor.history);
    assert(s64_editor_insert(&editor, 0xc1)); /* Distinct final uppercase A. */
    expect_body("RESUME GATE\nMARKER\nA");
    assert(s64_editor_undo(&editor)); expect_body("RESUME GATE\nMARKER\n");
    assert(s64_editor_redo(&editor)); expect_body("RESUME GATE\nMARKER\nA");
    native(document.slug, "resume-gate"); native(document.title, "Resume Gate");
    native(document.date, "2026-10-05");
    document.type = S64_DOCUMENT_PAGE; document.status = S64_DOCUMENT_DRAFT;
    native(local.filename, "RESUME.S64"); native(search, "MARKER");
    native(body, "CLIP");
    assert(s64_clipboard_write(&clipboard, 0, (unsigned char*)body, 4)); clipboard.length = 4;
    assert(s64_editor_find_next(&editor, search, 1));
    editor.view_start = 12; /* Show the selected MARKER line on resumed entry. */
    s64_editor_ensure_cursor_visible(&editor);
    s64_reu_runtime_init(&control, S64_PACKAGE_VERSION, 123);
    memset(&journal, 0, sizeof(journal));
    journal.state = S64_SESSION_STATE_ACTIVE; journal.package_version = S64_PACKAGE_VERSION;
    journal.session_id = 123; journal.sequence = 7;
    assert(s64_application_state_reserve(&control));
    assert(s64_application_state_capture(&state, &editor, &document, &local, &clipboard, search, 1, 41));
    assert(state.editor.selection_valid && state.editor.buffer.cursor > 0);
    assert(state.editor.view_start == 12 && state.editor.selection_anchor == 12
        && state.editor.buffer.cursor == 18);
    assert(s64_application_state_store(&control, &journal, &state, wire));
    assert(s64_application_state_load(&control, &journal, &decoded, wire));
    assert(!memcmp(&decoded, &state, sizeof(state)));
    header = fopen("resume_fixture.h", "w"); assert(header);
    fputs("/* Generated by real cc65 model; diagnostic use only. */\n#ifndef S64_RESUME_FIXTURE_H\n#define S64_RESUME_FIXTURE_H\n", header);
    export_file(header, "S64RDOC.DAT", "DOCUMENT", 0, S64_REU_DOCUMENT_CAPACITY);
    export_file(header, "S64RHP.DAT", "HISTORY_PAYLOAD", S64_REU_HISTORY_PAYLOAD_BASE, S64_REU_HISTORY_PAYLOAD_CAPACITY);
    export_file(header, "S64RHM.DAT", "HISTORY_META", 0x10000UL + S64_REU_HISTORY_META_BASE, metadata_extent);
    export_file(header, "S64RCLP.DAT", "CLIPBOARD", 0x10000UL + S64_REU_CLIPBOARD_BASE, 4);
    /* Bind only on the real ACTIVE run: dummy identity and CRC are not authority. */
    memset(wire + 8, 0, S64_SESSION_JOURNAL_WIRE_SIZE); memset(wire + 508, 0, 4);
    out = fopen("S64RAPP.DAT", "wb"); assert(out);
    assert(fwrite(wire, 1, sizeof(wire), out) == sizeof(wire)); assert(!fclose(out));
    state_crc = s64_crc32_update(0xffffffffUL, wire, sizeof(wire)) ^ 0xffffffffUL;
    fprintf(header, "#define S64_RESUME_STATE_LENGTH 512U\n#define S64_RESUME_STATE_CRC 0x%08lXUL\n#endif\n", state_crc);
    assert(!fclose(header));
    editor = decoded.editor;
    assert(s64_editor_undo(&editor)); expect_body("RESUME GATE\nMARKER\n");
    assert(s64_editor_redo(&editor)); expect_body("RESUME GATE\nMARKER\nA");
    s64_editor_clear_selection(&editor);
    assert(s64_buffer_move_cursor(&editor.buffer, s64_buffer_length(&editor.buffer)));
    s64_editor_recalculate_position(&editor);
    assert(s64_editor_paste(&editor, &clipboard)); expect_body("RESUME GATE\nMARKER\nACLIP");
    puts("Real cc65 resumed Editor fixture generated; Undo/Redo/Paste verified.");
    return 0;
}
