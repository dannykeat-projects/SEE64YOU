#include <conio.h>
#include <stdint.h>

#include "editor.h"
#include "ui.h"

#define DIAG_PROGRESS (*(volatile uint8_t*)0xC040u)
#define DIAG_RESULT   (*(volatile uint8_t*)0xC041u)
#define DIAG_ACK      (*(volatile uint8_t*)0xC042u)
#define DIAG_DETAIL   (*(volatile uint8_t*)0xC043u)

#define DIAG_PASS 0xA5u
#define DIAG_FAIL 0x5Au

static S64Editor editor;

static void fail(unsigned char detail)
{
    DIAG_DETAIL = detail;
    DIAG_RESULT = DIAG_FAIL;
    for (;;) {
    }
}

static void checkpoint(unsigned char value)
{
    DIAG_PROGRESS = value;
    while (DIAG_ACK != value) {
    }
}

static void append_char(unsigned char value, unsigned char detail)
{
    if (value == '\n') {
        if (!s64_editor_newline(&editor)) {
            fail(detail);
        }
    } else if (!s64_editor_insert(&editor, value)) {
        fail(detail);
    }

    if (!s64_ui_render_append(&editor, value)) s64_ui_render(&editor);
}

static void append_text(const char* text, unsigned char detail)
{
    while (*text != '\0') {
        append_char((unsigned char)*text++, detail);
    }
}

int main(void)
{
    unsigned char i;
    unsigned char previous_row;
    unsigned int screen_position;

    DIAG_PROGRESS = 0x01u;
    DIAG_RESULT = 0u;
    DIAG_ACK = 0u;
    DIAG_DETAIL = 0u;

    s64_editor_init(&editor);
    DIAG_PROGRESS = 0x02u;
    s64_ui_init(0);
    DIAG_PROGRESS = 0x03u;
    s64_ui_render(&editor);
    DIAG_PROGRESS = 0x04u;

    /* Reproduce a real-world hazard for the append fast path: other UI code
     * may leave conio's cursor somewhere unrelated to the editor cursor. */
    gotoxy(39u, 24u);

    append_text(
        "0123456789012345678901234567890123456789012345\n"
        "1111111111\n"
        "2222222222",
        1u
    );
    DIAG_PROGRESS = 0x05u;
    checkpoint(0x10u);

    if (!s64_editor_go_to_line(&editor, 2u)) {
        fail(2u);
    }
    for (i = 0u; i < 5u; ++i) {
        s64_editor_right(&editor);
    }
    s64_ui_render_cursor(&editor);

    previous_row = editor.view_cursor_row;
    if (!s64_editor_insert(&editor, '9')) {
        fail(3u);
    }
    s64_ui_render_edit(
        &editor,
        (unsigned char)(S64_UI_EDIT_CHARACTER | previous_row)
    );
    checkpoint(0x20u);

    previous_row = editor.view_cursor_row;
    if (!s64_editor_newline(&editor)) {
        fail(4u);
    }
    s64_ui_render_edit(&editor, previous_row);
    checkpoint(0x30u);

    previous_row = editor.view_cursor_row;
    if (!s64_editor_backspace(&editor)) {
        fail(5u);
    }
    s64_ui_render_edit(
        &editor,
        previous_row < editor.view_cursor_row
            ? previous_row
            : editor.view_cursor_row
    );
    checkpoint(0x40u);

    /* A modal Help screen must not survive below a short source document. */
    s64_ui_show_help();
    s64_ui_render(&editor);
    for (screen_position = 5u * 40u; screen_position < 24u * 40u; ++screen_position) {
        if ((*(volatile uint8_t*)(0x0400u + screen_position) & 0x7fu) != 0x20u) {
            fail(6u);
        }
    }
#ifdef S64_INDEPENDENT_EDITOR
    for (screen_position = 35u; screen_position <= 37u; ++screen_position) {
        if ((*(volatile uint8_t*)(0x0400u + screen_position) & 0x7fu) != 0x20u) {
            fail(7u);
        }
    }
#endif
    checkpoint(0x45u);

    if (!s64_editor_point(&editor, 2u, 3u, 0u)
        || !s64_editor_point(&editor, 3u, 2u, 1u)) {
        fail(8u);
    }
    s64_ui_render(&editor);
    checkpoint(0x46u);

    s64_editor_init(&editor);
    append_text("12345678901234567890123456789012345 67890", 9u);
    s64_ui_render(&editor);
    checkpoint(0x47u);

    /* Selection must highlight the same cells in either drag direction,
     * without leaving reverse-video artefacts after clearing selection. */
    s64_editor_init(&editor);
    s64_ui_render(&editor);
    append_text("12345", 10u);
    if (!s64_editor_point(&editor, 0u, 1u, 0u)
        || !s64_editor_point(&editor, 0u, 4u, 1u)) fail(11u);
    s64_ui_render(&editor);
    screen_position = 40u;
    if ((*(volatile uint8_t*)(0x0400u + screen_position) & 0x80u) != 0u
        || (*(volatile uint8_t*)(0x0400u + screen_position + 1u) & 0x80u) == 0u
        || (*(volatile uint8_t*)(0x0400u + screen_position + 2u) & 0x80u) == 0u
        || (*(volatile uint8_t*)(0x0400u + screen_position + 3u) & 0x80u) == 0u) {
        fail(12u);
    }

    if (!s64_editor_point(&editor, 0u, 4u, 0u)
        || !s64_editor_point(&editor, 0u, 1u, 1u)) fail(13u);
    s64_ui_render(&editor);
    /* The cursor inverts cell 1, so inspect the other selected cells. */
    if ((*(volatile uint8_t*)(0x0400u + screen_position + 2u) & 0x80u) == 0u
        || (*(volatile uint8_t*)(0x0400u + screen_position + 3u) & 0x80u) == 0u
        || (*(volatile uint8_t*)(0x0400u + screen_position + 4u) & 0x80u) != 0u) {
        fail(14u);
    }
    s64_editor_clear_selection(&editor);
    s64_ui_render(&editor);
    if ((*(volatile uint8_t*)(0x0400u + screen_position + 2u) & 0x80u) != 0u) {
        fail(15u);
    }
    s64_editor_toggle_word_wrap(&editor);
    if (!s64_editor_point(&editor, 0u, 1u, 0u)
        || !s64_editor_point(&editor, 0u, 4u, 1u)) fail(16u);
    s64_ui_render(&editor);
    checkpoint(0x48u);

    s64_editor_right(&editor);
    if (s64_editor_has_selection(&editor)) fail(17u);
    s64_ui_render_cursor(&editor);
    checkpoint(0x49u);

    DIAG_PROGRESS = 0x50u;
    DIAG_RESULT = DIAG_PASS;
    for (;;) {
    }
}
