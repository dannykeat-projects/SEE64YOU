#include <conio.h>
#include <string.h>
#include "ui.h"

#define SCREEN_COLUMNS 40u
#define EDITOR_FIRST_ROW 1u
#define EDITOR_ROWS 23u
#define STATUS_ROW 24u

void __fastcall__ s64_ui_status_line_fast(unsigned int value);
void __fastcall__ s64_ui_status_column_fast(unsigned int value);
void s64_ui_status_dirty_fast(void);

static unsigned char http_ready;
static unsigned char dos_ready;
static unsigned char header_rendered;
static unsigned char body_rows;
static unsigned char cursor_position_valid;
static unsigned char cursor_col;
static unsigned char cursor_row;
static unsigned int rendered_view_start;
static unsigned int rendered_view_column;
static unsigned char rendered_word_wrap;
static unsigned char row_chars[SCREEN_COLUMNS + 1u];
static unsigned char row_reverse[SCREEN_COLUMNS];

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

static void prepare_row(void)
{
    memset(row_chars, ' ', sizeof(row_chars));
    memset(row_reverse, 0, sizeof(row_reverse));
}

static unsigned char position_selected(
    unsigned int position,
    unsigned char has_selection,
    unsigned int selection_start,
    unsigned int selection_end
)
{
    return has_selection
        && position >= selection_start
        && position < selection_end;
}

static void flush_body_row(unsigned char row)
{
    unsigned char col = 0u;
    unsigned char reverse = 0u;

    gotoxy(0u, row);
    revers(0);
    while (col < SCREEN_COLUMNS) {
        if (row_reverse[col] != reverse) {
            reverse = row_reverse[col];
            revers(reverse);
        }
        cputc(row_chars[col]);
        ++col;
    }

    revers(0);
}

static void finish_body_rows(
    unsigned char rendered_rows,
    unsigned char previous_rows
)
{
    unsigned char desired_rows = rendered_rows;

    while (rendered_rows < previous_rows) {
        prepare_row();
        flush_body_row((unsigned char)(EDITOR_FIRST_ROW + rendered_rows));
        ++rendered_rows;
    }

    body_rows = desired_rows;
}

static void render_status_text(const char* text)
{
    unsigned char col = 0u;

    revers(1);
    gotoxy(0u, STATUS_ROW);
    while (col < SCREEN_COLUMNS && text[col] != '\0') {
        cputc((unsigned char)text[col]);
        ++col;
    }
    while (col < SCREEN_COLUMNS) {
        cputc(' ');
        ++col;
    }
    revers(0);
}

static void render_status(const S64Editor* editor)
{
    memcpy(
        row_chars,
        "Ln 0000 Col 0000 INS     OFF  WRP       ",
        SCREEN_COLUMNS
    );
    row_chars[SCREEN_COLUMNS] = '\0';

    if (!editor->insert_mode) {
        memcpy(&row_chars[17], "OVR", 3u);
    }
    if (editor->dirty) {
        memcpy(&row_chars[20], " MOD", 4u);
    }
    if (dos_ready) {
        memcpy(&row_chars[25], "DOS ", 4u);
    } else if (http_ready) {
        memcpy(&row_chars[25], "HTTP", 4u);
    }
    if (!editor->word_wrap) {
        memcpy(&row_chars[30], "HOR", 3u);
    }
    if (editor->mark_mode) {
        memcpy(&row_chars[33], " MARK", 5u);
    } else if (s64_editor_has_selection(editor)) {
        memcpy(&row_chars[33], " SEL ", 5u);
    }

    render_status_text((const char*)row_chars);
    s64_ui_status_line_fast(editor->line);
    s64_ui_status_column_fast(editor->column);

}

static unsigned int render_line_end(
    const S64Buffer* buffer,
    unsigned int position
)
{
    unsigned int length = s64_buffer_length(buffer);
    unsigned int physical = position;

    if (physical >= buffer->gap_start) {
        physical += buffer->gap_end - buffer->gap_start;
    }

    while (position < length && buffer->data[physical] != '\n') {
        ++position;
        ++physical;
        if (physical == buffer->gap_start) {
            physical = buffer->gap_end;
        }
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
    unsigned int physical = position;
    unsigned int selection_start = 0u;
    unsigned int selection_end = 0u;
    unsigned char has_selection = s64_editor_has_selection(editor);
    unsigned char found = 0u;
    unsigned char row;
    unsigned char col;
    unsigned char line_break;
    unsigned char previous_rows = body_rows;
    unsigned char rendered_rows = 0u;

    if (physical >= editor->buffer.gap_start) {
        physical += editor->buffer.gap_end - editor->buffer.gap_start;
    }

    if (has_selection) {
        selection_start = s64_editor_selection_start(editor);
        selection_end = s64_editor_selection_end(editor);
    }

    for (row = EDITOR_FIRST_ROW; row < STATUS_ROW; ++row) {
        prepare_row();
        col = 0u;
        line_break = 0u;

        if (!found && position == cursor) {
            *cursor_row = row;
            *cursor_col = 0u;
            found = 1u;
        }

        while (position < length && col < SCREEN_COLUMNS) {
            if (!found && position == cursor) {
                *cursor_row = row;
                *cursor_col = col;
                found = 1u;
            }

            if (editor->buffer.data[physical] == '\n') {
                ++position;
                ++physical;
                if (physical == editor->buffer.gap_start) {
                    physical = editor->buffer.gap_end;
                }
                line_break = 1u;
                break;
            }

            row_chars[col] = editor->buffer.data[physical];
            if (has_selection) {
                row_reverse[col] = position_selected(
                    position,
                    has_selection,
                    selection_start,
                    selection_end
                );
            }
            ++position;
            ++physical;
            if (physical == editor->buffer.gap_start) {
                physical = editor->buffer.gap_end;
            }
            ++col;
        }

        if (!found
            && !line_break
            && col < SCREEN_COLUMNS
            && position == cursor) {
            *cursor_row = row;
            *cursor_col = col;
            found = 1u;
        }

        flush_body_row(row);
        ++rendered_rows;

        if (position >= length && found) {
            break;
        }
    }

    finish_body_rows(rendered_rows, previous_rows);

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
    unsigned int physical;
    unsigned char has_selection = s64_editor_has_selection(editor);
    unsigned char found = 0u;
    unsigned char row;
    unsigned char col;
    unsigned char previous_rows = body_rows;
    unsigned char rendered_rows = 0u;

    if (has_selection) {
        selection_start = s64_editor_selection_start(editor);
        selection_end = s64_editor_selection_end(editor);
    }

    for (row = EDITOR_FIRST_ROW; row < STATUS_ROW; ++row) {
        prepare_row();

        if (line > length) {
            break;
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
        if (display >= editor->buffer.gap_start) {
            physical = display
                + (editor->buffer.gap_end - editor->buffer.gap_start);
        } else {
            physical = display;
        }
        while (display < end && col < SCREEN_COLUMNS) {
            row_chars[col] = editor->buffer.data[physical];
            if (has_selection) {
                row_reverse[col] = position_selected(
                    display,
                    has_selection,
                    selection_start,
                    selection_end
                );
            }
            ++display;
            ++physical;
            if (physical == editor->buffer.gap_start) {
                physical = editor->buffer.gap_end;
            }
            ++col;
        }

        flush_body_row(row);
        ++rendered_rows;

        if (end == length) {
            break;
        } else {
            line = end + 1u;
        }
    }

    finish_body_rows(rendered_rows, previous_rows);

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

}

void s64_ui_init(void)
{
    bgcolor(COLOR_BLUE);
    bordercolor(COLOR_BLUE);
    textcolor(COLOR_LIGHTBLUE);
    clrscr();

    header_rendered = 0u;
    body_rows = 0u;
    cursor_position_valid = 0u;
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
    unsigned char next_cursor_col = 0u;
    unsigned char next_cursor_row = EDITOR_FIRST_ROW;

    render_header();
    render_body(editor, &next_cursor_col, &next_cursor_row);
    render_status(editor);
    gotoxy(next_cursor_col, next_cursor_row);
    cursor_col = next_cursor_col;
    cursor_row = next_cursor_row;
    cursor_position_valid = 1u;
    rendered_view_start = editor->view_start;
    rendered_view_column = editor->view_column;
    rendered_word_wrap = editor->word_wrap;
}

void s64_ui_render_append(const S64Editor* editor, unsigned char value)
{
    unsigned char col = cursor_col;
    unsigned char row = cursor_row;
    unsigned char visible_rows;

    if (!cursor_position_valid) {
        s64_ui_render(editor);
        return;
    }

    revers(0);
    if (value == '\n') {
        col = 0u;
        ++row;
    } else {
        cputc(value);
        ++col;
        if (col >= SCREEN_COLUMNS) {
            col = 0u;
            ++row;
        }
    }

    if (row >= STATUS_ROW) {
        s64_ui_render(editor);
        return;
    }

    visible_rows = (unsigned char)(row - EDITOR_FIRST_ROW + 1u);
    if (visible_rows > body_rows) {
        body_rows = visible_rows;
    }

    s64_ui_status_column_fast(editor->column);
    if (value == '\n') {
        s64_ui_status_line_fast(editor->line);
        gotoxy(col, row);
    }
    if (editor->dirty) {
        s64_ui_status_dirty_fast();
    }

    cursor_col = col;
    cursor_row = row;
}

void s64_ui_render_cursor(const S64Editor* editor)
{
    unsigned char col = editor->view_cursor_column;
    unsigned char row = (unsigned char)(EDITOR_FIRST_ROW + editor->view_cursor_row);

    if (!cursor_position_valid
        || rendered_view_start != editor->view_start
        || rendered_view_column != editor->view_column
        || rendered_word_wrap != editor->word_wrap
        || col >= SCREEN_COLUMNS
        || row >= STATUS_ROW) {
        s64_ui_render(editor);
        return;
    }

    render_status(editor);
    gotoxy(col, row);
    cursor_col = col;
    cursor_row = row;
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
