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

    s64_ui_render_append(&editor, value);
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

    DIAG_PROGRESS = 0u;
    DIAG_RESULT = 0u;
    DIAG_ACK = 0u;
    DIAG_DETAIL = 0u;

    s64_editor_init(&editor);
    s64_ui_init();
    s64_ui_render(&editor);

    append_text(
        "0123456789012345678901234567890123456789012345\n"
        "1111111111\n"
        "2222222222",
        1u
    );
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

    DIAG_PROGRESS = 0x50u;
    DIAG_RESULT = DIAG_PASS;
    for (;;) {
    }
}
