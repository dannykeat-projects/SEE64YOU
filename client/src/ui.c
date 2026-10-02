#include <conio.h>
#include <string.h>
#include "ui.h"

#define SCREEN_COLUMNS 40u
#define EDITOR_FIRST_ROW 1u
#define EDITOR_ROWS 23u
#define STATUS_ROW 24u
#define SCREEN_RAM ((volatile unsigned char*)0x0400u)

void __fastcall__ s64_ui_status_line_fast(unsigned int value);
void __fastcall__ s64_ui_status_column_fast(unsigned int value);
void s64_ui_status_dirty_fast(void);

static unsigned char http_ready;
static unsigned char dos_ready;
static unsigned char header_rendered;
static unsigned char body_rows;
static unsigned char partial_first_row;
static unsigned char partial_char_edit;
static unsigned char cursor_position_valid;
static unsigned char cursor_col;
static unsigned char cursor_row;
static unsigned char cursor_visible;
static unsigned int rendered_view_start;
static unsigned int rendered_view_column;
static unsigned char rendered_word_wrap;
static unsigned char row_chars[SCREEN_COLUMNS + 1u];
static unsigned char row_reverse[SCREEN_COLUMNS];

static void toggle_editor_cursor(void)
{
    SCREEN_RAM[(unsigned int)cursor_row * SCREEN_COLUMNS + cursor_col] ^= 0x80u;
}

static void hide_editor_cursor(void)
{
    if (cursor_visible) {
        toggle_editor_cursor();
        cursor_visible = 0u;
    }
}

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

    if (partial_first_row != 0u && row < partial_first_row) {
        return;
    }

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
    return s64_buffer_find_byte(buffer, position, '\n');
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
    unsigned int read_count;
    unsigned int logical;
    unsigned char has_selection = s64_editor_has_selection(editor);
    unsigned char found = 0u;
    unsigned char row;
    unsigned char col;
    unsigned char line_break;
    unsigned char previous_rows = body_rows;
    unsigned char rendered_rows = 0u;

    row = EDITOR_FIRST_ROW;
    if (partial_char_edit) {
        position = cursor - editor->view_cursor_column;
        rendered_rows = (unsigned char)(partial_first_row - EDITOR_FIRST_ROW);
        row = partial_first_row;
    }

    if (has_selection) {
        selection_start = s64_editor_selection_start(editor);
        selection_end = s64_editor_selection_end(editor);
    }

    for (; row < STATUS_ROW; ++row) {
        prepare_row();
        col = 0u;
        line_break = 0u;

        if (!found && position == cursor) {
            *cursor_row = row;
            *cursor_col = 0u;
            found = 1u;
        }

        read_count = s64_buffer_read(
            &editor->buffer,
            position,
            row_chars,
            SCREEN_COLUMNS
        );

        while ((unsigned int)col < read_count) {
            logical = position + col;

            if (!found && logical == cursor) {
                *cursor_row = row;
                *cursor_col = col;
                found = 1u;
            }

            if (row_chars[col] == '\n') {
                memset(&row_chars[col], ' ', SCREEN_COLUMNS - col);
                position = logical + 1u;
                line_break = 1u;
                break;
            }

            if (has_selection) {
                row_reverse[col] = position_selected(
                    logical,
                    has_selection,
                    selection_start,
                    selection_end
                );
            }
            ++col;
        }

        if (!line_break) {
            position += read_count;
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

        if (partial_char_edit
            && row >= partial_first_row
            && line_break
            && col < SCREEN_COLUMNS - 1u) {
            rendered_rows = previous_rows;
            break;
        }

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
    unsigned int read_count;
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

        read_count = end - display;
        if (read_count > SCREEN_COLUMNS) {
            read_count = SCREEN_COLUMNS;
        }
        (void)s64_buffer_read(&editor->buffer, display, row_chars, read_count);

        if (has_selection) {
            for (col = 0u; (unsigned int)col < read_count; ++col) {
                row_reverse[col] = position_selected(
                    display + col,
                    has_selection,
                    selection_start,
                    selection_end
                );
            }
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
    bgcolor(COLOR_BLACK);
    bordercolor(COLOR_BLACK);
    textcolor(COLOR_GREEN);
    clrscr();
    cursor(0);

    header_rendered = 0u;
    body_rows = 0u;
    partial_first_row = 0u;
    partial_char_edit = 0u;
    cursor_position_valid = 0u;
    cursor_visible = 0u;
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

    hide_editor_cursor();
    render_header();
    render_body(editor, &next_cursor_col, &next_cursor_row);
    if (partial_first_row != 0u) {
        s64_ui_status_line_fast(editor->line);
        s64_ui_status_column_fast(editor->column);
        if (editor->dirty) {
            s64_ui_status_dirty_fast();
        }
    } else {
        render_status(editor);
    }
    gotoxy(next_cursor_col, next_cursor_row);
    cursor_col = next_cursor_col;
    cursor_row = next_cursor_row;
    cursor_position_valid = 1u;
    rendered_view_start = editor->view_start;
    rendered_view_column = editor->view_column;
    rendered_word_wrap = editor->word_wrap;
    partial_first_row = 0u;
    partial_char_edit = 0u;
    toggle_editor_cursor();
    cursor_visible = 1u;
}

void s64_ui_render_edit(const S64Editor* editor, unsigned char first_body_row)
{
    partial_char_edit = (unsigned char)(first_body_row & 0x80u);
    first_body_row &= 0x7fu;

    if (!editor->word_wrap
        || editor->mark_mode
        || s64_editor_has_selection(editor)
        || rendered_view_start != editor->view_start
        || rendered_view_column != editor->view_column
        || rendered_word_wrap != editor->word_wrap
        || first_body_row >= EDITOR_ROWS) {
        partial_char_edit = 0u;
        s64_ui_render(editor);
        return;
    }

    partial_first_row = (unsigned char)(EDITOR_FIRST_ROW + first_body_row);
    s64_ui_render(editor);
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

    hide_editor_cursor();
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
    toggle_editor_cursor();
    cursor_visible = 1u;
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

    hide_editor_cursor();
    s64_ui_status_line_fast(editor->line);
    s64_ui_status_column_fast(editor->column);
    gotoxy(col, row);
    cursor_col = col;
    cursor_row = row;
    toggle_editor_cursor();
    cursor_visible = 1u;
}

void s64_ui_message(const char* message)
{
    render_status_text(message);
}

void s64_ui_show_help(void)
{
    static const char* const lines[] = {
        "SEE64YOU HELP                    F8",
        "",
        "TOP BAR = COMMAND GROUPS",
        "FILE   Ctrl+N New   Ctrl+O Open",
        "       Ctrl+S Save  Ctrl+Shift+S As",
        "EDIT   F1 Mark F3 Copy F5 Cut F7 Paste",
        "       Ctrl+A All Ctrl+Z Undo/Redo",
        "SEARCH Ctrl+F Find F2 Next F4 Previous",
        "       Ctrl+G Line Ctrl+H/F6 Replace",
        "FORMAT Ctrl+B Bold Ctrl+I Italic",
        "       Ctrl+L List Ctrl+W Word Wrap",
        "PUB    Ctrl+P Publisher",
        "",
        "CBM+F1 New F3 Open F5 Save F7 Save As",
        "Ctrl+Left/Right = word movement",
        "Ctrl+Up/Down = page movement",
        "Ctrl/CBM+Delete = forward delete",
        "RUN/STOP = cancel or exit",
        "",
        "Top labels are not menus yet.",
        "Use these shortcuts for their commands.",
        "",
        "PRESS ANY KEY TO RETURN"
    };
    unsigned char row;

    hide_editor_cursor();
    revers(0);
    clrscr();
    for (row = 0u; row < (unsigned char)(sizeof(lines) / sizeof(lines[0])); ++row) {
        gotoxy(0u, row);
        cputs(lines[row]);
    }
    (void)cgetc();

    header_rendered = 0u;
    body_rows = 0u;
    partial_first_row = 0u;
    partial_char_edit = 0u;
    cursor_position_valid = 0u;
    cursor_visible = 0u;
    clrscr();
}


#ifdef S64_REU_BUFFER
static void render_scrolling_prompt(
    const char* label,
    const char* value,
    unsigned char length,
    unsigned char label_length
)
{
    unsigned char visible = (unsigned char)(SCREEN_COLUMNS - label_length - 1u);
    unsigned char start = length > visible
        ? (unsigned char)(length - visible)
        : 0u;
    unsigned char shown = 0u;

    clear_row(STATUS_ROW);
    revers(1);
    gotoxy(0u, STATUS_ROW);
    cputs(label);

    while (shown < visible && (unsigned char)(start + shown) < length) {
        cputc((unsigned char)value[start + shown]);
        ++shown;
    }
    while (shown < visible) {
        cputc(' ');
        ++shown;
    }

    gotoxy(
        (unsigned char)(label_length + (length - start)),
        STATUS_ROW
    );
}
#endif

unsigned char s64_ui_prompt_text(const char* label, char* value, unsigned char capacity)
{
    unsigned char key;
    unsigned char length = 0u;
    unsigned char label_length = (unsigned char)strlen(label);

    if (capacity == 0u || label_length >= SCREEN_COLUMNS) {
        return 0u;
    }

    value[0] = '\0';
#ifdef S64_REU_BUFFER
    render_scrolling_prompt(label, value, length, label_length);
#else
    clear_row(STATUS_ROW);
    revers(1);
    gotoxy(0u, STATUS_ROW);
    cputs(label);
#endif

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
                value[length] = '\0';
#ifdef S64_REU_BUFFER
                render_scrolling_prompt(label, value, length, label_length);
#else
                gotoxy((unsigned char)(label_length + length), STATUS_ROW);
                cputc(' ');
                gotoxy((unsigned char)(label_length + length), STATUS_ROW);
#endif
            }
            continue;
        }

        if (key >= 0x20u && key <= 0x7eu
            && length + 1u < capacity
#ifndef S64_REU_BUFFER
            && label_length + length < SCREEN_COLUMNS
#endif
        ) {
            value[length++] = (char)key;
            value[length] = '\0';
#ifdef S64_REU_BUFFER
            render_scrolling_prompt(label, value, length, label_length);
#else
            cputc(key);
#endif
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
