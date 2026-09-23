#include <conio.h>
#include <stdio.h>
#include <string.h>
#include "ui.h"

#define SCREEN_COLUMNS 40u
#define EDITOR_FIRST_ROW 1u
#define EDITOR_ROWS 23u
#define STATUS_ROW 24u

static unsigned char http_ready;

static void clear_row(unsigned char row)
{
    unsigned char i;
    gotoxy(0u, row);
    for (i = 0u; i < SCREEN_COLUMNS; ++i) {
        cputc(' ');
    }
}

static void render_header(void)
{
    clear_row(0u);
    revers(1);
    gotoxy(0u, 0u);
    cputs(" SEE64YOU  FILE EDIT SEARCH FORMAT PUB ");
    revers(0);
}

static void render_status(const S64Editor* editor)
{
    char status[41];
    const char* mode = editor->insert_mode ? "INS" : "OVR";
    const char* dirty = editor->dirty ? " MOD" : "";
    const char* selection = editor->mark_mode
        ? " MARK"
        : (s64_editor_has_selection(editor) ? " SEL" : "");

    sprintf(
        status,
        "Ln %u Col %u %s%s %s%s",
        editor->line,
        editor->column,
        mode,
        dirty,
        http_ready ? "HTTP" : "OFF",
        selection
    );

    clear_row(STATUS_ROW);
    revers(1);
    gotoxy(0u, STATUS_ROW);
    cputs(status);
    revers(0);
}

static void render_body(const S64Editor* editor)
{
    unsigned int i = editor->view_start;
    unsigned int length = s64_buffer_length(&editor->buffer);
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    unsigned char row = EDITOR_FIRST_ROW;
    unsigned char col = 0u;
    unsigned char cursor_row = EDITOR_FIRST_ROW;
    unsigned char cursor_col = 0u;
    unsigned char cursor_found = 0u;

    for (row = EDITOR_FIRST_ROW; row < STATUS_ROW; ++row) {
        clear_row(row);
    }

    row = EDITOR_FIRST_ROW;

    while (i <= length && row < STATUS_ROW) {
        if (i == cursor) {
            cursor_row = row;
            cursor_col = col;
            cursor_found = 1u;
        }

        if (i == length) {
            break;
        }

        if (s64_buffer_char_at(&editor->buffer, i) == '\n') {
            ++row;
            col = 0u;
            ++i;
            continue;
        }

        gotoxy(col, row);
        if (s64_editor_has_selection(editor)
            && i >= s64_editor_selection_start(editor)
            && i < s64_editor_selection_end(editor)) {
            revers(1);
            cputc(s64_buffer_char_at(&editor->buffer, i));
            revers(0);
        } else {
            cputc(s64_buffer_char_at(&editor->buffer, i));
        }
        ++col;
        ++i;

        if (col >= SCREEN_COLUMNS) {
            ++row;
            col = 0u;
        }
    }

    if (!cursor_found) {
        cursor_row = EDITOR_FIRST_ROW;
        cursor_col = 0u;
    }

    gotoxy(cursor_col, cursor_row);
}

void s64_ui_init(void)
{
    bgcolor(COLOR_BLUE);
    bordercolor(COLOR_BLUE);
    textcolor(COLOR_LIGHTBLUE);
    clrscr();
}

void s64_ui_set_http_ready(unsigned char ready)
{
    http_ready = ready ? 1u : 0u;
}

void s64_ui_render(const S64Editor* editor)
{
    render_header();
    render_body(editor);
    render_status(editor);
}

void s64_ui_message(const char* message)
{
    clear_row(STATUS_ROW);
    revers(1);
    gotoxy(0u, STATUS_ROW);
    cputs(message);
    revers(0);
}
