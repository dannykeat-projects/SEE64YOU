#include <stdint.h>

#include "clipboard.h"
#include "editor.h"
#include "platform/reu.h"

#define DIAG_PROGRESS (*(volatile uint8_t*)0xC020u)
#define DIAG_RESULT   (*(volatile uint8_t*)0xC021u)
#define DIAG_DETAIL   (*(volatile uint8_t*)0xC022u)

#define DIAG_PASS 0xA5u
#define DIAG_FAIL 0x5Au

static S64Editor editor;
static S64Clipboard clipboard;

static unsigned char fixture_value(unsigned int index)
{
    unsigned int offset = index % 31u;
    unsigned int line = index / 31u;

    if (offset == 30u) {
        return '\n';
    }
    if (offset == 9u || offset == 19u || offset == 29u) {
        return ' ';
    }
    return (unsigned char)('a' + ((line + offset) % 26u));
}

static void fail(unsigned char detail)
{
    DIAG_DETAIL = detail;
    DIAG_RESULT = DIAG_FAIL;
    for (;;) {
    }
}

static void require_fixture_range(unsigned int start, unsigned int count)
{
    unsigned int i;

    for (i = 0u; i < count; ++i) {
        if (s64_buffer_char_at(&editor.buffer, start + i) != fixture_value(start + i)) {
            fail(9u);
        }
    }
}

int main(void)
{
    unsigned int i;

    DIAG_PROGRESS = 0x10u;
    DIAG_RESULT = 0u;
    DIAG_DETAIL = 0u;

    if (!s64_reu_probe()) {
        fail(1u);
    }

    s64_editor_init(&editor);
    s64_clipboard_init(&clipboard);

    DIAG_PROGRESS = 0x20u;
    for (i = 0u; i < 6000u; ++i) {
        if (!s64_buffer_insert(&editor.buffer, fixture_value(i))) {
            fail(2u);
        }
    }
    if (!s64_buffer_move_cursor(&editor.buffer, 0u)) {
        fail(2u);
    }
    s64_editor_document_loaded(&editor);

    if (s64_buffer_length(&editor.buffer) != 6000u
        || s64_buffer_cursor(&editor.buffer) != 0u
        || editor.line != 1u
        || editor.column != 1u
        || s64_buffer_char_at(&editor.buffer, 4095u) != fixture_value(4095u)
        || s64_buffer_char_at(&editor.buffer, 4096u) != fixture_value(4096u)
        || s64_buffer_char_at(&editor.buffer, 5999u) != fixture_value(5999u)) {
        fail(3u);
    }

    DIAG_PROGRESS = 0x30u;
    if (!s64_editor_go_to_line(&editor, 180u)
        || s64_buffer_cursor(&editor.buffer) != 5549u
        || editor.line != 180u
        || editor.column != 1u) {
        fail(4u);
    }

    s64_editor_end(&editor);
    if (s64_buffer_cursor(&editor.buffer) != 5579u
        || editor.column != 31u
        || !s64_editor_insert(&editor, '!')) {
        fail(5u);
    }
    if (s64_buffer_length(&editor.buffer) != 6001u
        || s64_buffer_char_at(&editor.buffer, 5579u) != '!'
        || s64_buffer_char_at(&editor.buffer, 5580u) != '\n') {
        fail(6u);
    }

    DIAG_PROGRESS = 0x40u;
    if (!s64_editor_go_to_line(&editor, 180u)) {
        fail(7u);
    }
    DIAG_PROGRESS = 0x41u;
    s64_editor_toggle_mark(&editor);
    for (i = 0u; i < 5u; ++i) {
        s64_editor_right(&editor);
    }
    s64_editor_toggle_mark(&editor);
    DIAG_PROGRESS = 0x42u;
    if (!s64_editor_copy(&editor, &clipboard) || clipboard.length != 5u) {
        fail(8u);
    }
    DIAG_PROGRESS = 0x43u;
    if (!s64_editor_cut(&editor, &clipboard)
        || s64_buffer_length(&editor.buffer) != 5996u) {
        fail(8u);
    }
    DIAG_PROGRESS = 0x44u;
    if (!s64_editor_undo(&editor)
        || s64_buffer_length(&editor.buffer) != 6001u) {
        fail(8u);
    }
    DIAG_PROGRESS = 0x45u;
    require_fixture_range(5549u, 5u);
    if (!s64_editor_redo(&editor)
        || s64_buffer_length(&editor.buffer) != 5996u) {
        fail(10u);
    }
    DIAG_PROGRESS = 0x46u;
    if (!s64_editor_paste(&editor, &clipboard)
        || s64_buffer_length(&editor.buffer) != 6001u) {
        fail(10u);
    }
    DIAG_PROGRESS = 0x47u;
    require_fixture_range(5549u, 5u);

    DIAG_PROGRESS = 0x50u;
    if (!s64_editor_go_to_line(&editor, 190u)
        || s64_buffer_cursor(&editor.buffer) != 5860u) {
        fail(11u);
    }
    if (!s64_editor_insert(&editor, 'Q')
        || !s64_editor_insert(&editor, 'Z')
        || !s64_editor_insert(&editor, 'Q')
        || !s64_editor_insert(&editor, 'Z')
        || !s64_editor_insert(&editor, 'Q')) {
        fail(12u);
    }

    if (!s64_editor_go_to_line(&editor, 1u)
        || !s64_editor_find_next(&editor, "QZQZQ", 0u)
        || s64_editor_selection_start(&editor) != 5860u
        || s64_editor_selection_end(&editor) != 5865u) {
        fail(13u);
    }

    DIAG_PROGRESS = 0x60u;
    if (!s64_editor_replace_selection(&editor, "DONE")
        || s64_buffer_length(&editor.buffer) != 6005u
        || s64_buffer_char_at(&editor.buffer, 5860u) != 'D'
        || s64_buffer_char_at(&editor.buffer, 5861u) != 'O'
        || s64_buffer_char_at(&editor.buffer, 5862u) != 'N'
        || s64_buffer_char_at(&editor.buffer, 5863u) != 'E'
        || s64_buffer_char_at(&editor.buffer, 5864u) != fixture_value(5859u)) {
        fail(14u);
    }

    if (!s64_editor_undo(&editor)
        || s64_buffer_length(&editor.buffer) != 6006u
        || s64_buffer_char_at(&editor.buffer, 5860u) != 'Q'
        || !s64_editor_redo(&editor)
        || s64_buffer_length(&editor.buffer) != 6005u
        || s64_buffer_char_at(&editor.buffer, 5860u) != 'D') {
        fail(15u);
    }

    DIAG_PROGRESS = 0x70u;
    if (!s64_editor_go_to_line(&editor, 180u)
        || s64_buffer_cursor(&editor.buffer) != 5549u) {
        fail(16u);
    }
    s64_editor_word_right(&editor);
    if (s64_buffer_cursor(&editor.buffer) != 5559u) {
        fail(17u);
    }
    s64_editor_word_left(&editor);
    if (s64_buffer_cursor(&editor.buffer) != 5549u) {
        fail(18u);
    }
    if (s64_editor_go_to_line(&editor, 999u)
        || s64_buffer_cursor(&editor.buffer) != 5549u
        || editor.line != 180u) {
        fail(19u);
    }

    DIAG_PROGRESS = 0x80u;
    DIAG_RESULT = DIAG_PASS;
    for (;;) {
    }
}
