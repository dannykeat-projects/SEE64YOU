#include <conio.h>
#include <stdio.h>
#include <string.h>
#include "ui.h"

#define SCREEN_COLUMNS 40u
#define EDITOR_FIRST_ROW 1u
#define EDITOR_ROWS 23u
#define STATUS_ROW 24u

static unsigned char http_ready;
static unsigned char dos_ready;

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
    const char* wrap = editor->word_wrap ? "WRP" : "HOR";
    const char* selection = editor->mark_mode
        ? " MARK"
        : (s64_editor_has_selection(editor) ? " SEL" : "");

    sprintf(
        status,
        "Ln %u Col %u %s%s %s %s%s",
        editor->line,
        editor->column,
        mode,
        dirty,
        dos_ready ? "DOS" : (http_ready ? "HTTP" : "OFF"),
        wrap,
        selection
    );

    clear_row(STATUS_ROW);
    revers(1);
    gotoxy(0u, STATUS_ROW);
    cputs(status);
    revers(0);
}

static void render_character(
    const S64Editor* editor,
    unsigned int position,
    unsigned char col,
    unsigned char row
)
{
    gotoxy(col, row);
    if (s64_editor_has_selection(editor)
        && position >= s64_editor_selection_start(editor)
        && position < s64_editor_selection_end(editor)) {
        revers(1);
        cputc(s64_buffer_char_at(&editor->buffer, position));
        revers(0);
    } else {
        cputc(s64_buffer_char_at(&editor->buffer, position));
    }
}

static unsigned int render_line_end(
    const S64Buffer* buffer,
    unsigned int position
)
{
    unsigned int length = s64_buffer_length(buffer);

    while (position < length && s64_buffer_char_at(buffer, position) != '\n') {
        ++position;
    }
    return position;
}

static void render_body_wrapped(const S64Editor* editor)
{
    unsigned int i = editor->view_start;
    unsigned int length = s64_buffer_length(&editor->buffer);
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    unsigned char row = EDITOR_FIRST_ROW;
    unsigned char col = 0u;
    unsigned char cursor_row = EDITOR_FIRST_ROW;
    unsigned char cursor_col = 0u;
    unsigned char cursor_found = 0u;

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

        render_character(editor, i, col, row);
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

static void render_body_horizontal(const S64Editor* editor)
{
    unsigned int line = editor->view_start;
    unsigned int length = s64_buffer_length(&editor->buffer);
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    unsigned int end;
    unsigned int display;
    unsigned int cursor_offset;
    unsigned char row = EDITOR_FIRST_ROW;
    unsigned char col;
    unsigned char cursor_row = EDITOR_FIRST_ROW;
    unsigned char cursor_col = 0u;
    unsigned char cursor_found = 0u;

    while (line <= length && row < STATUS_ROW) {
        end = render_line_end(&editor->buffer, line);

        if (cursor >= line && cursor <= end) {
            cursor_offset = cursor - line;
            if (cursor_offset >= editor->view_column
                && cursor_offset < editor->view_column + SCREEN_COLUMNS) {
                cursor_row = row;
                cursor_col = (unsigned char)(cursor_offset - editor->view_column);
                cursor_found = 1u;
            }
        }

        display = line;
        if (editor->view_column <= end - line) {
            display += editor->view_column;
        } else {
            display = end;
        }

        col = 0u;
        while (display < end && col < SCREEN_COLUMNS) {
            render_character(editor, display, col, row);
            ++display;
            ++col;
        }

        if (end == length) {
            break;
        }

        line = end + 1u;
        ++row;
    }

    if (!cursor_found) {
        cursor_row = EDITOR_FIRST_ROW;
        cursor_col = 0u;
    }

    gotoxy(cursor_col, cursor_row);
}

static void render_body(const S64Editor* editor)
{
    unsigned char row;

    for (row = EDITOR_FIRST_ROW; row < STATUS_ROW; ++row) {
        clear_row(row);
    }

    if (editor->word_wrap) {
        render_body_wrapped(editor);
    } else {
        render_body_horizontal(editor);
    }
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

void s64_ui_set_dos_ready(unsigned char ready)
{
    dos_ready = ready ? 1u : 0u;
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

unsigned char s64_ui_prompt_text(const char* label, char* value, unsigned char capacity)
{
    unsigned char key;
    unsigned char length = 0u;
    unsigned char label_length = (unsigned char)strlen(label);

    if (capacity == 0u || label_length >= SCREEN_COLUMNS) {
        return 0u;
    }

    value[0] = '\0';
    clear_row(STATUS_ROW);
    revers(1);
    gotoxy(0u, STATUS_ROW);
    cputs(label);

    for (;;) {
        key = (unsigned char)cgetc();

        if (key == CH_STOP) {
            value[0] = '\0';
            revers(0);
            return 0u;
        }

        if (key == CH_ENTER) {
            value[length] = '\0';
            revers(0);
            return 1u;
        }

        if (key == CH_DEL) {
            if (length > 0u) {
                --length;
                gotoxy((unsigned char)(label_length + length), STATUS_ROW);
                cputc(' ');
                gotoxy((unsigned char)(label_length + length), STATUS_ROW);
            }
            continue;
        }

        if (key >= 0x20u && key <= 0x7eu
            && length + 1u < capacity
            && label_length + length < SCREEN_COLUMNS) {
            value[length++] = (char)key;
            cputc(key);
        }
    }
}

unsigned char s64_ui_prompt_yes_no(const char* label)
{
    unsigned char key;

    clear_row(STATUS_ROW);
    revers(1);
    gotoxy(0u, STATUS_ROW);
    cputs(label);

    for (;;) {
        key = (unsigned char)cgetc();
        if (key == 'y' || key == 'Y') {
            revers(0);
            return 1u;
        }
        if (key == 'n' || key == 'N' || key == CH_STOP) {
            revers(0);
            return 0u;
        }
    }
}
