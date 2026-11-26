#include <conio.h>
#include <string.h>
#include "ui.h"
#include "menu.h"
#include "document.h"
#include "wrap.h"

#define SCREEN_COLUMNS 40u
#define EDITOR_FIRST_ROW 1u
#define EDITOR_ROWS 23u
#define STATUS_ROW 24u
#define SCREEN_RAM ((volatile unsigned char*)0x0400u)

void __fastcall__ s64_ui_status_line_fast(unsigned int value);
void __fastcall__ s64_ui_status_column_fast(unsigned int value);
void s64_ui_status_dirty_fast(void);

static unsigned char http_ready;
static S64DocumentKind current_kind;
static const S64Document* active_document;
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
static unsigned char rendered_selection;
static unsigned char row_chars[SCREEN_COLUMNS + 1u];
static unsigned char row_selection_start;
static unsigned char row_selection_end;

#if defined(S64_MANAGED_OVERLAY) && defined(S64_INDEPENDENT_EDITOR)
#pragma code-name (push, "CODE")
#endif
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

static void show_editor_cursor(unsigned char col, unsigned char row)
{
    gotoxy(col, row);
    cursor_col = col;
    cursor_row = row;
    toggle_editor_cursor();
    cursor_visible = 1u;
}

static unsigned char rendered_view_matches(const S64Editor* editor)
{
    return rendered_view_start == editor->view_start
        && rendered_view_column == editor->view_column
        && rendered_word_wrap == editor->word_wrap;
}

#if defined(S64_MANAGED_OVERLAY) && defined(S64_INDEPENDENT_EDITOR)
#pragma code-name (pop)
#endif

/* UI runs in FILE, while the public selection getters run in MODEL. Read the
 * existing state here to avoid reloading both overlays for every redraw. */
static unsigned char render_selection_bounds(
    const S64Editor* editor, unsigned int* start, unsigned int* end
)
{
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    unsigned int anchor = editor->selection_anchor;

    if (!editor->selection_valid || anchor == cursor) return 0u;
    if (anchor > cursor) {
        unsigned int swap = anchor;
        anchor = cursor;
        cursor = swap;
    }
    if (start != 0) *start = anchor;
    if (end != 0) *end = cursor;
    return 1u;
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
    S64DocumentKind kind = active_document != 0
        ? active_document->kind : S64_DOCUMENT_KIND_8TML;
    if (header_rendered && kind == current_kind) return;
    current_kind = kind;

    clear_row(0u);
    revers(1);
    gotoxy(0u, 0u);
    cputs(S64_MENU_BAR);
    gotoxy(35u, 0u);
    cputs(current_kind == S64_DOCUMENT_KIND_8SS ? "8SS "
        : current_kind == S64_DOCUMENT_KIND_TEXT ? "TEXT" : "8TML");
    revers(0);
    header_rendered = 1u;
}

int s64_ui_parse_line_number(const char* text, unsigned int* line_number)
{
    unsigned int value = 0u;
    unsigned char digit;

    if (*text == '\0') {
        return 0;
    }

    while (*text != '\0') {
        if (*text < '0' || *text > '9') {
            return 0;
        }

        digit = (unsigned char)(*text - '0');
#ifdef S64_REU_BUFFER
        if (value > 6553u || (value == 6553u && digit > 5u)) {
            return 0;
        }
#else
        if (value > 819u) {
            return 0;
        }
#endif

        value = (unsigned int)(value * 10u + digit);
        if (value > S64_BUFFER_CAPACITY + 1u) {
            return 0;
        }
        ++text;
    }

    if (value == 0u) {
        return 0;
    }

    *line_number = value;
    return 1;
}


static void prepare_row(void)
{
    memset(row_chars, ' ', SCREEN_COLUMNS);
    row_chars[SCREEN_COLUMNS] = '\0';
    row_selection_end = 0u;
}

static void select_row_span(
    unsigned int position, unsigned char count,
    unsigned int selection_start, unsigned int selection_end
)
{
    unsigned int end = position + count;

    if (selection_start >= end || selection_end <= position) return;
    row_selection_start = selection_start > position
        ? (unsigned char)(selection_start - position) : 0u;
    row_selection_end = selection_end < end
        ? (unsigned char)(selection_end - position) : count;
}

static void flush_body_row(unsigned char row)
{
    unsigned char col;

    if (partial_first_row != 0u && row < partial_first_row) {
        return;
    }

    gotoxy(0u, row);
    revers(0);
    if (row_selection_end == 0u) {
        cputs((const char*)row_chars);
        return;
    }

    for (col = 0u; col < row_selection_start; ++col) cputc(row_chars[col]);
    revers(1);
    for (; col < row_selection_end; ++col) cputc(row_chars[col]);
    revers(0);
    cputs((const char*)&row_chars[col]);
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
    rendered_selection = (unsigned char)(editor->mark_mode
        || render_selection_bounds(editor, 0, 0));
    if (editor->mark_mode) {
        memcpy(&row_chars[33], " MARK", 5u);
    } else if (rendered_selection) {
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
    unsigned char has_selection;
    unsigned char found = 0u;
    unsigned char row;
    unsigned char col;
    unsigned char line_break;
    unsigned char previous_rows = body_rows;
    unsigned char rendered_rows = 0u;

    has_selection = render_selection_bounds(editor, &selection_start, &selection_end);
    row = EDITOR_FIRST_ROW;
    if (partial_char_edit) {
        position = cursor - editor->view_cursor_column;
        rendered_rows = (unsigned char)(partial_first_row - EDITOR_FIRST_ROW);
        row = partial_first_row;
    }

    for (; row < STATUS_ROW; ++row) {
        prepare_row();
        read_count = s64_buffer_read(
            &editor->buffer,
            position,
            row_chars,
            sizeof(row_chars)
        );

        col = s64_wrap_width(row_chars, (unsigned char)read_count);
        line_break = col < SCREEN_COLUMNS && col < read_count
            && row_chars[col] == '\n';
        memset(&row_chars[col], ' ', SCREEN_COLUMNS - col);
        row_chars[SCREEN_COLUMNS] = '\0';

        if (!found && cursor >= position && cursor - position <= col
            && (cursor - position < col || line_break || col == read_count)
            && cursor - position < SCREEN_COLUMNS) {
            *cursor_row = row;
            *cursor_col = (unsigned char)(cursor - position);
            found = 1u;
        }
        if (has_selection) {
            select_row_span(position, col, selection_start, selection_end);
        }
        position += col + line_break;

        if (partial_char_edit
            && row >= partial_first_row
            && line_break
            && col < SCREEN_COLUMNS - 1u) {
            /* A single-cell edit within a short row leaves later rows intact.
             * Include one trailing blank to erase a deleted final character. */
            unsigned char changed = editor->view_cursor_column;
            if (changed != 0u) --changed;
            gotoxy(changed, row);
            revers(0);
            while (changed <= col) {
                cputc(row_chars[changed++]);
            }
            rendered_rows = previous_rows;
            break;
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
    unsigned int read_count;
    unsigned char has_selection;
    unsigned char found = 0u;
    unsigned char row;
    unsigned char previous_rows = body_rows;
    unsigned char rendered_rows = 0u;

    has_selection = render_selection_bounds(editor, &selection_start, &selection_end);
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
            select_row_span(display, (unsigned char)read_count, selection_start, selection_end);
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

void s64_ui_init(const S64Document* document)
{
    active_document = document;
    current_kind = 0;
    bgcolor(COLOR_BLACK);
    bordercolor(COLOR_BLACK);
    textcolor(COLOR_WHITE);
    clrscr();
    cursor(0);

    header_rendered = 0u;
    body_rows = 0u;
    partial_first_row = 0u;
    partial_char_edit = 0u;
    cursor_position_valid = 0u;
    cursor_visible = 0u;
    rendered_selection = 0u;
}

S64DocumentKind s64_ui_select_document_kind(void)
{
    unsigned char key;
    s64_ui_message("New:1 8TML 2 8SS 3 TXT 0 Cancel");
    key = cgetc();
    if (key < '1' || key > '3') return 0;
    return (S64DocumentKind)(key - '0');
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
    render_status(editor);
    cursor_position_valid = 1u;
    rendered_view_start = editor->view_start;
    rendered_view_column = editor->view_column;
    rendered_word_wrap = editor->word_wrap;
    partial_first_row = 0u;
    partial_char_edit = 0u;
    show_editor_cursor(next_cursor_col, next_cursor_row);
}

void s64_ui_render_edit(const S64Editor* editor, unsigned char first_body_row)
{
    partial_char_edit = (unsigned char)(first_body_row & 0x80u);
    first_body_row &= 0x7fu;

    if (!cursor_position_valid
        || !editor->word_wrap
        || editor->mark_mode
        || rendered_selection
        || render_selection_bounds(editor, 0, 0)
        || !rendered_view_matches(editor)
        || first_body_row >= EDITOR_ROWS) {
        partial_char_edit = 0u;
        s64_ui_render(editor);
        return;
    }

    partial_first_row = (unsigned char)(EDITOR_FIRST_ROW + first_body_row);
    s64_ui_render(editor);
}

#if defined(S64_MANAGED_OVERLAY) && defined(S64_INDEPENDENT_EDITOR)
/* Appending should leave the model overlay loaded for the next keystroke. */
#pragma code-name (push, "CODE")
#endif
unsigned char s64_ui_render_append(const S64Editor* editor, unsigned char value)
{
    if (!cursor_position_valid) return 0u;

    hide_editor_cursor();
    if (value != '\n') {
        revers(0);
        gotoxy(cursor_col, cursor_row);
        cputc(value);
    }

    if (value == '\n' || ++cursor_col == SCREEN_COLUMNS) {
        cursor_col = 0u;
        if (++cursor_row == STATUS_ROW) return 0u;
        if (cursor_row > body_rows) body_rows = cursor_row;
        gotoxy(0u, cursor_row);
    }
    s64_ui_status_column_fast(editor->column);
    if (value == '\n') s64_ui_status_line_fast(editor->line);
    if (editor->dirty) s64_ui_status_dirty_fast();

    toggle_editor_cursor();
    cursor_visible = 1u;
    return 1u;
}
#if defined(S64_MANAGED_OVERLAY) && defined(S64_INDEPENDENT_EDITOR)
#pragma code-name (pop)
#endif

void s64_ui_render_cursor(const S64Editor* editor)
{
    unsigned char col = editor->view_cursor_column;
    unsigned char row = (unsigned char)(EDITOR_FIRST_ROW + editor->view_cursor_row);

    /* Moving out of a selection also has to clear its old screen cells. */
    if (!cursor_position_valid
        || rendered_selection
        || !rendered_view_matches(editor)
        || col >= SCREEN_COLUMNS
        || row >= STATUS_ROW) {
        s64_ui_render(editor);
        return;
    }

    hide_editor_cursor();
    s64_ui_status_line_fast(editor->line);
    s64_ui_status_column_fast(editor->column);
    show_editor_cursor(col, row);
}

void s64_ui_render_pointer(
    const S64Editor* editor, unsigned int previous_cursor, unsigned char drag
)
{
    unsigned char col = editor->view_cursor_column;
    unsigned char row = (unsigned char)(EDITOR_FIRST_ROW + editor->view_cursor_row);
    unsigned char first;
    unsigned char last;
    volatile unsigned char* screen;

    if (!cursor_position_valid || !rendered_view_matches(editor)
        || col >= SCREEN_COLUMNS || row >= STATUS_ROW
        || (!drag && rendered_selection)
        || (drag && (row != cursor_row
            || s64_buffer_cursor(&editor->buffer) - col != previous_cursor - cursor_col
            || (!rendered_selection && editor->selection_anchor != previous_cursor)))) {
        s64_ui_render(editor);
        return;
    }

    hide_editor_cursor();
    if (drag) {
        /* The anchor stays fixed during a drag. On the same rendered row,
         * exactly the cells between the old and new endpoints change. */
        screen = SCREEN_RAM + (unsigned int)row * SCREEN_COLUMNS;
        first = col < cursor_col ? col : cursor_col;
        last = col < cursor_col ? cursor_col : col;
        while (first < last) screen[first++] ^= 0x80u;
    }
    render_status(editor);
    show_editor_cursor(col, row);
}

void s64_ui_message(const char* message)
{
    render_status_text(message);
    /* Append rendering prints at the conio cursor, so retain its body position. */
    if (cursor_position_valid) {
        gotoxy(cursor_col, cursor_row);
    }
}

void s64_ui_begin_overlay(void)
{
    hide_editor_cursor();
    /* Modal content can occupy rows beyond the current source viewport. */
    header_rendered = 0u;
    body_rows = EDITOR_ROWS;
    partial_first_row = 0u;
    partial_char_edit = 0u;
    cursor_position_valid = 0u;
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
        "       Ctrl+Z Undo  Ctrl+Shift+Z Redo",
        "SEARCH Ctrl+F Find F2 Next F4 Previous",
        "       Ctrl+G Line Ctrl+H/F6 Replace",
        "8TML   Ctrl+B Bold Ctrl+I Italic",
        "       Ctrl+L List Ctrl+K Link",
#ifndef S64_INDEPENDENT_EDITOR
        "PUB    Ctrl+P Publisher",
#endif
        "",
        "ALL    Ctrl+A Select all  Ctrl+W Wrap",
        "CBM+F1 New F3 Open F5 Save F7 Save As",
        "Ctrl+Left/Right = word movement",
        "Ctrl+Up/Down = page movement",
        "Ctrl/CBM+Delete = forward delete",
        "RUN/STOP = cancel or exit",
        "",
        "CBM+M OR JOYSTICK FIRE OPENS MENUS",
        "ARROWS/JOYSTICK MOVE, ENTER/FIRE SELECT",
        "MOUSE: CLICK TOP LABEL, THEN ITEM",
        "PRESS ANY KEY/CLICK/FIRE TO RETURN"
    };
    unsigned char row;

    s64_ui_begin_overlay();
    revers(0);
    clrscr();
    for (row = 0u; row < (unsigned char)(sizeof(lines) / sizeof(lines[0])); ++row) {
        gotoxy(0u, row);
        cputs(lines[row]);
    }
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

        /* Shifted PETSCII letters occupy $C1-$DA. Keep native input intact. */
        if (key >= 0x20u && (key <= 0x7eu
                || (unsigned char)(key - 0xc1u) <= 25u)
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
