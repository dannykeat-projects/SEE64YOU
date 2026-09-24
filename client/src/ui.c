#include <conio.h>
#include <stdio.h>
#include <string.h>
#include "ui.h"

#define SCREEN_COLUMNS 40u
#define EDITOR_FIRST_ROW 1u
#define EDITOR_ROWS 23u
#define STATUS_ROW 24u
#define EDITOR_CELLS (EDITOR_ROWS * SCREEN_COLUMNS)
#define REVERSE_CACHE_BYTES ((EDITOR_CELLS + 7u) / 8u)

static unsigned char http_ready;
static unsigned char dos_ready;
static unsigned char header_rendered;
static unsigned char body_cache_valid;
static unsigned char status_cache_valid;
static unsigned char body_cache[EDITOR_CELLS];
static unsigned char body_reverse_cache[REVERSE_CACHE_BYTES];
static unsigned char row_chars[SCREEN_COLUMNS];
static unsigned char row_reverse[SCREEN_COLUMNS];
static unsigned char status_cache[SCREEN_COLUMNS];

static const unsigned char bit_masks[8] = {
    0x01u, 0x02u, 0x04u, 0x08u,
    0x10u, 0x20u, 0x40u, 0x80u
};

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
    if (header_rendered) {
        return;
    }

    clear_row(0u);
    revers(1);
    gotoxy(0u, 0u);
    cputs(" SEE64YOU  FILE EDIT SEARCH FORMAT PUB ");
    revers(0);
    header_rendered = 1u;
}

static unsigned char cached_reverse(unsigned int cell)
{
    return (body_reverse_cache[cell >> 3] & bit_masks[cell & 7u]) != 0u;
}

static void set_cached_reverse(unsigned int cell, unsigned char reverse)
{
    unsigned char* value = &body_reverse_cache[cell >> 3];
    unsigned char mask = bit_masks[cell & 7u];

    if (reverse) {
        *value |= mask;
    } else {
        *value &= (unsigned char)~mask;
    }
}

static void prepare_row(void)
{
    memset(row_chars, ' ', sizeof(row_chars));
    memset(row_reverse, 0, sizeof(row_reverse));
}

static unsigned char position_selected(
    const S64Editor* editor,
    unsigned int position,
    unsigned char has_selection,
    unsigned int selection_start,
    unsigned int selection_end
)
{
    (void)editor;
    return has_selection
        && position >= selection_start
        && position < selection_end;
}

static void flush_body_row(unsigned char row)
{
    unsigned int base = (unsigned int)(row - EDITOR_FIRST_ROW) * SCREEN_COLUMNS;
    unsigned int cell;
    unsigned char col = 0u;
    unsigned char reverse = 0u;

    while (col < SCREEN_COLUMNS) {
        cell = base + col;

        if (body_cache_valid
            && body_cache[cell] == row_chars[col]
            && cached_reverse(cell) == row_reverse[col]) {
            ++col;
            continue;
        }

        gotoxy(col, row);
        reverse = row_reverse[col];
        revers(reverse);

        while (col < SCREEN_COLUMNS) {
            cell = base + col;

            if (body_cache_valid
                && body_cache[cell] == row_chars[col]
                && cached_reverse(cell) == row_reverse[col]) {
                break;
            }

            if (row_reverse[col] != reverse) {
                reverse = row_reverse[col];
                revers(reverse);
            }

            cputc(row_chars[col]);
            body_cache[cell] = row_chars[col];
            set_cached_reverse(cell, row_reverse[col]);
            ++col;
        }
    }

    revers(0);
}

static void render_status_text(const char* text)
{
    unsigned char col;
    unsigned char length = (unsigned char)strlen(text);

    if (length > SCREEN_COLUMNS) {
        length = SCREEN_COLUMNS;
    }

    revers(1);
    col = 0u;
    while (col < SCREEN_COLUMNS) {
        unsigned char value = col < length ? (unsigned char)text[col] : (unsigned char)' ';

        if (status_cache_valid && status_cache[col] == value) {
            ++col;
            continue;
        }

        gotoxy(col, STATUS_ROW);

        while (col < SCREEN_COLUMNS) {
            value = col < length ? (unsigned char)text[col] : (unsigned char)' ';

            if (status_cache_valid && status_cache[col] == value) {
                break;
            }

            cputc(value);
            status_cache[col] = value;
            ++col;
        }
    }
    revers(0);
    status_cache_valid = 1u;
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

    render_status_text(status);
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

static void render_body_wrapped(
    const S64Editor* editor,
    unsigned char* cursor_col,
    unsigned char* cursor_row
)
{
    unsigned int position = editor->view_start;
    unsigned int length = s64_buffer_length(&editor->buffer);
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    unsigned int selection_start = 0u;
    unsigned int selection_end = 0u;
    unsigned char has_selection = s64_editor_has_selection(editor);
    unsigned char found = 0u;
    unsigned char row;
    unsigned char col;

    if (has_selection) {
        selection_start = s64_editor_selection_start(editor);
        selection_end = s64_editor_selection_end(editor);
    }

    for (row = EDITOR_FIRST_ROW; row < STATUS_ROW; ++row) {
        prepare_row();
        col = 0u;

        while (position < length && col < SCREEN_COLUMNS) {
            if (position == cursor) {
                *cursor_row = row;
                *cursor_col = col;
                found = 1u;
            }

            if (s64_buffer_char_at(&editor->buffer, position) == '\n') {
                ++position;
                break;
            }

            row_chars[col] = s64_buffer_char_at(&editor->buffer, position);
            row_reverse[col] = position_selected(
                editor,
                position,
                has_selection,
                selection_start,
                selection_end
            );
            ++position;
            ++col;
        }

        if (!found && position == cursor) {
            *cursor_row = row;
            *cursor_col = col;
            found = 1u;
        }

        flush_body_row(row);
    }

    if (!found) {
        *cursor_row = EDITOR_FIRST_ROW;
        *cursor_col = 0u;
    }
}

static void render_body_horizontal(
    const S64Editor* editor,
    unsigned char* cursor_col,
    unsigned char* cursor_row
)
{
    unsigned int line = editor->view_start;
    unsigned int length = s64_buffer_length(&editor->buffer);
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    unsigned int selection_start = 0u;
    unsigned int selection_end = 0u;
    unsigned int end;
    unsigned int display;
    unsigned int cursor_offset;
    unsigned char has_selection = s64_editor_has_selection(editor);
    unsigned char found = 0u;
    unsigned char row;
    unsigned char col;

    if (has_selection) {
        selection_start = s64_editor_selection_start(editor);
        selection_end = s64_editor_selection_end(editor);
    }

    for (row = EDITOR_FIRST_ROW; row < STATUS_ROW; ++row) {
        prepare_row();

        if (line > length) {
            flush_body_row(row);
            continue;
        }

        end = render_line_end(&editor->buffer, line);

        if (cursor >= line && cursor <= end) {
            cursor_offset = cursor - line;
            if (cursor_offset >= editor->view_column
                && cursor_offset < editor->view_column + SCREEN_COLUMNS) {
                *cursor_row = row;
                *cursor_col = (unsigned char)(cursor_offset - editor->view_column);
                found = 1u;
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
            row_chars[col] = s64_buffer_char_at(&editor->buffer, display);
            row_reverse[col] = position_selected(
                editor,
                display,
                has_selection,
                selection_start,
                selection_end
            );
            ++display;
            ++col;
        }

        flush_body_row(row);

        if (end == length) {
            line = length + 1u;
        } else {
            line = end + 1u;
        }
    }

    if (!found) {
        *cursor_row = EDITOR_FIRST_ROW;
        *cursor_col = 0u;
    }
}

static void render_body(
    const S64Editor* editor,
    unsigned char* cursor_col,
    unsigned char* cursor_row
)
{
    if (editor->word_wrap) {
        render_body_wrapped(editor, cursor_col, cursor_row);
    } else {
        render_body_horizontal(editor, cursor_col, cursor_row);
    }

    body_cache_valid = 1u;
}

void s64_ui_init(void)
{
    bgcolor(COLOR_BLUE);
    bordercolor(COLOR_BLUE);
    textcolor(COLOR_LIGHTBLUE);
    clrscr();

    header_rendered = 0u;
    body_cache_valid = 0u;
    status_cache_valid = 0u;
    memset(body_reverse_cache, 0, sizeof(body_reverse_cache));
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
    unsigned char cursor_col = 0u;
    unsigned char cursor_row = EDITOR_FIRST_ROW;

    render_header();
    render_body(editor, &cursor_col, &cursor_row);
    render_status(editor);
    gotoxy(cursor_col, cursor_row);
}

void s64_ui_message(const char* message)
{
    render_status_text(message);
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
    status_cache_valid = 0u;
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

    status_cache_valid = 0u;
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
