#include <stdint.h>

#include "clipboard.h"
#include "editor.h"

#define DIAG_PROGRESS (*(volatile uint8_t*)0xC030u)
#define DIAG_RESULT   (*(volatile uint8_t*)0xC031u)
#define DIAG_DETAIL   (*(volatile uint8_t*)0xC032u)

#define DIAG_PASS 0xA5u
#define DIAG_FAIL 0x5Au

static S64Editor editor;
static S64Clipboard clipboard;

static void fail(unsigned char detail)
{
    DIAG_DETAIL = detail;
    DIAG_RESULT = DIAG_FAIL;
    for (;;) {
    }
}

static void require_text(const char* expected, unsigned char detail)
{
    unsigned int i = 0u;

    while (expected[i] != '\0') {
        if (i >= s64_buffer_length(&editor.buffer)
            || s64_buffer_char_at(&editor.buffer, i)
                != (unsigned char)expected[i]) {
            fail(detail);
        }
        ++i;
    }
    if (s64_buffer_length(&editor.buffer) != i) {
        fail(detail);
    }
}

static void insert_text(const char* text, unsigned char detail)
{
    while (*text != '\0') {
        if (!s64_editor_insert(&editor, (unsigned char)*text)) {
            fail(detail);
        }
        ++text;
    }
}

int main(void)
{
    unsigned int replaced = 0u;
    unsigned char i;

    DIAG_PROGRESS = 0x10u;
    DIAG_RESULT = 0u;
    DIAG_DETAIL = 0u;

    s64_editor_init(&editor);
    s64_clipboard_init(&clipboard);

    insert_text("alpha beta alpha\nsecond line\nthird line", 1u);
    s64_editor_mark_saved(&editor);
    if (editor.dirty) {
        fail(2u);
    }

    DIAG_PROGRESS = 0x20u;
    if (!s64_editor_go_to_line(&editor, 2u)
        || s64_buffer_cursor(&editor.buffer) != 17u
        || editor.line != 2u
        || editor.column != 1u) {
        fail(3u);
    }
    s64_editor_right(&editor);
    if (s64_editor_go_to_line(&editor, 99u)
        || s64_buffer_cursor(&editor.buffer) != 18u
        || editor.line != 2u
        || editor.column != 2u) {
        fail(4u);
    }

    DIAG_PROGRESS = 0x30u;
    if (!s64_editor_go_to_line(&editor, 1u)
        || !s64_editor_find_next(&editor, "alpha", 1u)
        || s64_editor_selection_start(&editor) != 0u
        || s64_editor_selection_end(&editor) != 5u
        || !s64_editor_find_next(&editor, "alpha", 1u)
        || s64_editor_selection_start(&editor) != 11u
        || !s64_editor_find_previous(&editor, "alpha", 1u)
        || s64_editor_selection_start(&editor) != 0u) {
        fail(5u);
    }

    if (!s64_editor_copy(&editor, &clipboard)
        || clipboard.length != 5u
        || clipboard.data[0] != 'a'
        || clipboard.data[4] != 'a') {
        fail(6u);
    }

    DIAG_PROGRESS = 0x40u;
    if (!s64_editor_cut(&editor, &clipboard)) {
        fail(7u);
    }
    require_text(" beta alpha\nsecond line\nthird line", 8u);
    if (!s64_editor_undo(&editor) || editor.dirty) {
        fail(9u);
    }
    require_text("alpha beta alpha\nsecond line\nthird line", 10u);
    if (!s64_editor_redo(&editor) || !editor.dirty) {
        fail(11u);
    }
    require_text(" beta alpha\nsecond line\nthird line", 12u);
    if (!s64_editor_paste(&editor, &clipboard)) {
        fail(13u);
    }
    require_text("alpha beta alpha\nsecond line\nthird line", 14u);
    if (!s64_editor_undo(&editor)) {
        fail(15u);
    }
    require_text(" beta alpha\nsecond line\nthird line", 16u);
    if (!s64_editor_redo(&editor)) {
        fail(17u);
    }
    require_text("alpha beta alpha\nsecond line\nthird line", 18u);

    DIAG_PROGRESS = 0x50u;
    s64_editor_init(&editor);
    insert_text("alpha beta alpha", 19u);
    s64_editor_mark_saved(&editor);
    s64_editor_home(&editor);
    if (!s64_editor_find_next(&editor, "beta", 0u)
        || !s64_editor_replace_selection(&editor, "B")) {
        fail(20u);
    }
    require_text("alpha B alpha", 21u);
    if (!s64_editor_undo(&editor) || editor.dirty) {
        fail(22u);
    }
    require_text("alpha beta alpha", 23u);
    if (!s64_editor_redo(&editor)) {
        fail(24u);
    }
    require_text("alpha B alpha", 25u);

    DIAG_PROGRESS = 0x60u;
    s64_editor_init(&editor);
    insert_text("aa aa aa", 26u);
    s64_editor_mark_saved(&editor);
    if (!s64_editor_replace_all(&editor, "aa", "b", &replaced)
        || replaced != 3u) {
        fail(27u);
    }
    require_text("b b b", 28u);
    if (!s64_editor_undo(&editor) || editor.dirty) {
        fail(29u);
    }
    require_text("aa aa aa", 30u);
    if (!s64_editor_redo(&editor)) {
        fail(31u);
    }
    require_text("b b b", 32u);

    DIAG_PROGRESS = 0x70u;
    s64_editor_init(&editor);
    for (i = 0u; i < 45u; ++i) {
        if (!s64_editor_insert(&editor, (unsigned char)('0' + (i % 10u)))) {
            fail(33u);
        }
    }
    if (!editor.word_wrap || editor.view_column != 0u) {
        fail(34u);
    }
    s64_editor_toggle_word_wrap(&editor);
    if (editor.word_wrap || editor.view_column != 6u) {
        fail(35u);
    }
    s64_editor_home(&editor);
    if (editor.view_column != 0u) {
        fail(36u);
    }
    s64_editor_end(&editor);
    if (editor.view_column != 6u) {
        fail(37u);
    }
    s64_editor_toggle_word_wrap(&editor);
    if (!editor.word_wrap || editor.view_column != 0u) {
        fail(38u);
    }

    DIAG_PROGRESS = 0x78u;
    s64_editor_init(&editor);
    insert_text("hello", 39u);
    s64_editor_mark_saved(&editor);
    s64_editor_select_all(&editor);
    if (!s64_editor_format_bold(&editor)) {
        fail(40u);
    }
    require_text("**hello**", 41u);
    if (!s64_editor_undo(&editor) || editor.dirty) {
        fail(42u);
    }
    require_text("hello", 43u);
    if (!s64_editor_redo(&editor)) {
        fail(44u);
    }
    require_text("**hello**", 45u);

    DIAG_PROGRESS = 0x80u;
    DIAG_RESULT = DIAG_PASS;
    for (;;) {
    }
}
