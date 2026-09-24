#include "editor.h"

#define VIEW_COLUMNS 40u
#define VIEW_ROWS 23u

static unsigned int line_start(const S64Buffer* buffer, unsigned int position)
{
    while (position > 0u) {
        if (s64_buffer_char_at(buffer, position - 1u) == '\n') {
            break;
        }
        --position;
    }
    return position;
}

static unsigned int line_end(const S64Buffer* buffer, unsigned int position)
{
    unsigned int length = s64_buffer_length(buffer);

    while (position < length && s64_buffer_char_at(buffer, position) != '\n') {
        ++position;
    }
    return position;
}

static unsigned char is_word_char(unsigned char value)
{
    return (value >= 'A' && value <= 'Z')
        || (value >= 'a' && value <= 'z')
        || (value >= '0' && value <= '9')
        || value == '_';
}

static void reset_preferred_column(S64Editor* editor)
{
    editor->preferred_column_valid = 0u;
}

static int text_length(const char* text, unsigned int* length)
{
    unsigned int i = 0u;

    while (text[i] != '\0') {
        if (i >= S64_BUFFER_CAPACITY) {
            return 0;
        }
        ++i;
    }

    *length = i;
    return 1;
}

static unsigned char matches_at(
    const S64Buffer* buffer,
    unsigned int position,
    const char* query,
    unsigned int query_length
)
{
    unsigned int i;
    unsigned int length = s64_buffer_length(buffer);

    if (query_length == 0u || position > length || query_length > length - position) {
        return 0u;
    }

    for (i = 0u; i < query_length; ++i) {
        if (s64_buffer_char_at(buffer, position + i) != (unsigned char)query[i]) {
            return 0u;
        }
    }

    return 1u;
}

static int find_forward_position(
    const S64Buffer* buffer,
    const char* query,
    unsigned int query_length,
    unsigned int start,
    unsigned int* found
)
{
    unsigned int position;
    unsigned int length = s64_buffer_length(buffer);
    unsigned int last;

    if (query_length == 0u || query_length > length) {
        return 0;
    }

    last = length - query_length;
    if (start > last) {
        return 0;
    }

    for (position = start; position <= last; ++position) {
        if (matches_at(buffer, position, query, query_length)) {
            *found = position;
            return 1;
        }
    }

    return 0;
}

static int find_backward_position(
    const S64Buffer* buffer,
    const char* query,
    unsigned int query_length,
    unsigned int before,
    unsigned int* found
)
{
    unsigned int position;
    unsigned int length = s64_buffer_length(buffer);

    if (query_length == 0u || query_length > length || before < query_length) {
        return 0;
    }

    position = before - query_length;
    if (position > length - query_length) {
        position = length - query_length;
    }

    for (;;) {
        if (matches_at(buffer, position, query, query_length)) {
            *found = position;
            return 1;
        }
        if (position == 0u) {
            break;
        }
        --position;
    }

    return 0;
}

static void select_range(S64Editor* editor, unsigned int start, unsigned int length)
{
    editor->selection_anchor = start;
    s64_buffer_move_cursor(&editor->buffer, start + length);
    editor->selection_valid = length != 0u;
    editor->mark_mode = 0u;
    reset_preferred_column(editor);
    s64_editor_recalculate_position(editor);
    s64_editor_ensure_cursor_visible(editor);
}


static unsigned int preferred_column(S64Editor* editor, unsigned int cursor, unsigned int start)
{
    if (!editor->preferred_column_valid) {
        editor->preferred_column = cursor - start;
        editor->preferred_column_valid = 1u;
    }

    return editor->preferred_column;
}

unsigned char s64_editor_has_selection(const S64Editor* editor)
{
    return editor->selection_valid
        && editor->selection_anchor != s64_buffer_cursor(&editor->buffer);
}

unsigned int s64_editor_selection_start(const S64Editor* editor)
{
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);

    return editor->selection_anchor < cursor ? editor->selection_anchor : cursor;
}

unsigned int s64_editor_selection_end(const S64Editor* editor)
{
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);

    return editor->selection_anchor > cursor ? editor->selection_anchor : cursor;
}

void s64_editor_clear_selection(S64Editor* editor)
{
    editor->selection_anchor = s64_buffer_cursor(&editor->buffer);
    editor->selection_valid = 0u;
    editor->mark_mode = 0u;
}

static void update_selection_after_move(S64Editor* editor)
{
    if (editor->mark_mode) {
        editor->selection_valid = editor->selection_anchor
            != s64_buffer_cursor(&editor->buffer);
    } else {
        editor->selection_valid = 0u;
        editor->selection_anchor = s64_buffer_cursor(&editor->buffer);
    }
}

static int delete_selection(S64Editor* editor)
{
    unsigned int start;
    unsigned int end;

    if (!s64_editor_has_selection(editor)) {
        return 0;
    }

    start = s64_editor_selection_start(editor);
    end = s64_editor_selection_end(editor);

    if (!s64_buffer_delete_range(&editor->buffer, start, end)) {
        return 0;
    }

    editor->selection_anchor = start;
    editor->selection_valid = 0u;
    reset_preferred_column(editor);
    editor->mark_mode = 0u;
    editor->dirty = 1u;
    s64_editor_recalculate_position(editor);
    s64_editor_ensure_cursor_visible(editor);
    return 1;
}

void s64_editor_init(S64Editor* editor)
{
    s64_buffer_init(&editor->buffer);
    editor->view_start = 0u;
    editor->view_column = 0u;
    editor->line = 1u;
    editor->column = 1u;
    editor->selection_anchor = 0u;
    editor->preferred_column = 0u;
    editor->selection_valid = 0u;
    editor->preferred_column_valid = 0u;
    editor->mark_mode = 0u;
    editor->dirty = 0u;
    editor->insert_mode = 1u;
    editor->word_wrap = 1u;
}

void s64_editor_recalculate_position(S64Editor* editor)
{
    unsigned int i;
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    unsigned int line = 1u;
    unsigned int column = 1u;

    for (i = 0u; i < cursor; ++i) {
        if (s64_buffer_char_at(&editor->buffer, i) == '\n') {
            ++line;
            column = 1u;
        } else {
            ++column;
        }
    }

    editor->line = line;
    editor->column = column;
}

void s64_editor_ensure_cursor_visible(S64Editor* editor)
{
    unsigned int i;
    unsigned int row = 0u;
    unsigned int col = 0u;
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    unsigned int cursor_start = line_start(&editor->buffer, cursor);
    unsigned int cursor_column = cursor - cursor_start;

    if (editor->word_wrap) {
        editor->view_column = 0u;

        if (cursor < editor->view_start) {
            editor->view_start = cursor_start;
            return;
        }

        for (i = editor->view_start; i < cursor; ++i) {
            if (s64_buffer_char_at(&editor->buffer, i) == '\n') {
                ++row;
                col = 0u;
            } else {
                ++col;
                if (col >= VIEW_COLUMNS) {
                    ++row;
                    col = 0u;
                }
            }

            if (row >= VIEW_ROWS) {
                editor->view_start = cursor_start;
                return;
            }
        }
        return;
    }

    if (cursor < editor->view_start) {
        editor->view_start = cursor_start;
    } else {
        for (i = editor->view_start; i < cursor_start; ++i) {
            if (s64_buffer_char_at(&editor->buffer, i) == '\n') {
                ++row;
                if (row >= VIEW_ROWS) {
                    editor->view_start = cursor_start;
                    break;
                }
            }
        }
    }

    if (cursor_column < editor->view_column) {
        editor->view_column = cursor_column;
    } else if (cursor_column >= editor->view_column + VIEW_COLUMNS) {
        editor->view_column = cursor_column - (VIEW_COLUMNS - 1u);
    }
}

int s64_editor_insert(S64Editor* editor, unsigned char value)
{
    int changed;

    delete_selection(editor);

    if (!editor->insert_mode
        && s64_buffer_cursor(&editor->buffer) < s64_buffer_length(&editor->buffer)) {
        s64_buffer_delete(&editor->buffer);
    }

    changed = s64_buffer_insert(&editor->buffer, value);
    if (changed) {
        editor->dirty = 1u;
        editor->selection_anchor = s64_buffer_cursor(&editor->buffer);
        reset_preferred_column(editor);
        s64_editor_recalculate_position(editor);
        s64_editor_ensure_cursor_visible(editor);
    }
    return changed;
}

int s64_editor_newline(S64Editor* editor)
{
    return s64_editor_insert(editor, '\n');
}

int s64_editor_backspace(S64Editor* editor)
{
    int changed;

    if (delete_selection(editor)) {
        return 1;
    }

    changed = s64_buffer_backspace(&editor->buffer);
    if (changed) {
        editor->dirty = 1u;
        editor->selection_anchor = s64_buffer_cursor(&editor->buffer);
        reset_preferred_column(editor);
        s64_editor_recalculate_position(editor);
        s64_editor_ensure_cursor_visible(editor);
    }
    return changed;
}

int s64_editor_delete(S64Editor* editor)
{
    int changed;

    if (delete_selection(editor)) {
        return 1;
    }

    changed = s64_buffer_delete(&editor->buffer);
    if (changed) {
        editor->dirty = 1u;
        editor->selection_anchor = s64_buffer_cursor(&editor->buffer);
        reset_preferred_column(editor);
        s64_editor_recalculate_position(editor);
    }
    return changed;
}

void s64_editor_left(S64Editor* editor)
{
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);

    if (cursor > 0u) {
        s64_buffer_move_cursor(&editor->buffer, cursor - 1u);
        reset_preferred_column(editor);
        update_selection_after_move(editor);
        s64_editor_recalculate_position(editor);
        s64_editor_ensure_cursor_visible(editor);
    }
}

void s64_editor_right(S64Editor* editor)
{
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);

    if (cursor < s64_buffer_length(&editor->buffer)) {
        s64_buffer_move_cursor(&editor->buffer, cursor + 1u);
        reset_preferred_column(editor);
        update_selection_after_move(editor);
        s64_editor_recalculate_position(editor);
        s64_editor_ensure_cursor_visible(editor);
    }
}

void s64_editor_up(S64Editor* editor)
{
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    unsigned int current_start = line_start(&editor->buffer, cursor);
    unsigned int previous_end;
    unsigned int previous_start;
    unsigned int desired_column;

    if (current_start == 0u) {
        return;
    }

    desired_column = preferred_column(editor, cursor, current_start);
    previous_end = current_start - 1u;
    previous_start = line_start(&editor->buffer, previous_end);

    if (previous_start + desired_column > previous_end) {
        s64_buffer_move_cursor(&editor->buffer, previous_end);
    } else {
        s64_buffer_move_cursor(&editor->buffer, previous_start + desired_column);
    }

    update_selection_after_move(editor);
    s64_editor_recalculate_position(editor);
    s64_editor_ensure_cursor_visible(editor);
}

void s64_editor_down(S64Editor* editor)
{
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    unsigned int current_start = line_start(&editor->buffer, cursor);
    unsigned int current_end = line_end(&editor->buffer, cursor);
    unsigned int next_start;
    unsigned int next_end;
    unsigned int desired_column;

    if (current_end >= s64_buffer_length(&editor->buffer)) {
        return;
    }

    desired_column = preferred_column(editor, cursor, current_start);
    next_start = current_end + 1u;
    next_end = line_end(&editor->buffer, next_start);

    if (next_start + desired_column > next_end) {
        s64_buffer_move_cursor(&editor->buffer, next_end);
    } else {
        s64_buffer_move_cursor(&editor->buffer, next_start + desired_column);
    }

    update_selection_after_move(editor);
    s64_editor_recalculate_position(editor);
    s64_editor_ensure_cursor_visible(editor);
}

void s64_editor_home(S64Editor* editor)
{
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    s64_buffer_move_cursor(&editor->buffer, line_start(&editor->buffer, cursor));
    reset_preferred_column(editor);
    update_selection_after_move(editor);
    s64_editor_recalculate_position(editor);
    s64_editor_ensure_cursor_visible(editor);
}

void s64_editor_end(S64Editor* editor)
{
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    s64_buffer_move_cursor(&editor->buffer, line_end(&editor->buffer, cursor));
    reset_preferred_column(editor);
    update_selection_after_move(editor);
    s64_editor_recalculate_position(editor);
    s64_editor_ensure_cursor_visible(editor);
}

void s64_editor_page_up(S64Editor* editor)
{
    unsigned char i;

    for (i = 0u; i < VIEW_ROWS - 1u; ++i) {
        unsigned int before = s64_buffer_cursor(&editor->buffer);
        s64_editor_up(editor);
        if (s64_buffer_cursor(&editor->buffer) == before) {
            break;
        }
    }
}

void s64_editor_page_down(S64Editor* editor)
{
    unsigned char i;

    for (i = 0u; i < VIEW_ROWS - 1u; ++i) {
        unsigned int before = s64_buffer_cursor(&editor->buffer);
        s64_editor_down(editor);
        if (s64_buffer_cursor(&editor->buffer) == before) {
            break;
        }
    }
}

void s64_editor_word_left(S64Editor* editor)
{
    unsigned int position = s64_buffer_cursor(&editor->buffer);

    while (position > 0u && !is_word_char(s64_buffer_char_at(&editor->buffer, position - 1u))) {
        --position;
    }
    while (position > 0u && is_word_char(s64_buffer_char_at(&editor->buffer, position - 1u))) {
        --position;
    }

    s64_buffer_move_cursor(&editor->buffer, position);
    reset_preferred_column(editor);
    update_selection_after_move(editor);
    s64_editor_recalculate_position(editor);
    s64_editor_ensure_cursor_visible(editor);
}

void s64_editor_word_right(S64Editor* editor)
{
    unsigned int position = s64_buffer_cursor(&editor->buffer);
    unsigned int length = s64_buffer_length(&editor->buffer);

    while (position < length && is_word_char(s64_buffer_char_at(&editor->buffer, position))) {
        ++position;
    }
    while (position < length && !is_word_char(s64_buffer_char_at(&editor->buffer, position))) {
        ++position;
    }

    s64_buffer_move_cursor(&editor->buffer, position);
    reset_preferred_column(editor);
    update_selection_after_move(editor);
    s64_editor_recalculate_position(editor);
    s64_editor_ensure_cursor_visible(editor);
}

int s64_editor_go_to_line(S64Editor* editor, unsigned int line_number)
{
    unsigned int i;
    unsigned int current_line = 1u;
    unsigned int length = s64_buffer_length(&editor->buffer);

    if (line_number == 0u) {
        return 0;
    }

    if (line_number == 1u) {
        s64_buffer_move_cursor(&editor->buffer, 0u);
    } else {
        for (i = 0u; i < length; ++i) {
            if (s64_buffer_char_at(&editor->buffer, i) == '\n') {
                ++current_line;
                if (current_line == line_number) {
                    s64_buffer_move_cursor(&editor->buffer, i + 1u);
                    break;
                }
            }
        }

        if (current_line != line_number) {
            return 0;
        }
    }

    reset_preferred_column(editor);
    update_selection_after_move(editor);
    s64_editor_recalculate_position(editor);
    s64_editor_ensure_cursor_visible(editor);
    return 1;
}

int s64_editor_find_next(S64Editor* editor, const char* query, unsigned char wrap)
{
    unsigned int query_length;
    unsigned int start;
    unsigned int found;

    if (!text_length(query, &query_length) || query_length == 0u) {
        return 0;
    }

    start = s64_editor_has_selection(editor)
        ? s64_editor_selection_end(editor)
        : s64_buffer_cursor(&editor->buffer);

    if (!find_forward_position(&editor->buffer, query, query_length, start, &found)) {
        if (!wrap || start == 0u
            || !find_forward_position(&editor->buffer, query, query_length, 0u, &found)) {
            return 0;
        }
    }

    select_range(editor, found, query_length);
    return 1;
}

int s64_editor_find_previous(S64Editor* editor, const char* query, unsigned char wrap)
{
    unsigned int query_length;
    unsigned int before;
    unsigned int found;
    unsigned int length = s64_buffer_length(&editor->buffer);

    if (!text_length(query, &query_length) || query_length == 0u) {
        return 0;
    }

    before = s64_editor_has_selection(editor)
        ? s64_editor_selection_start(editor)
        : s64_buffer_cursor(&editor->buffer);

    if (!find_backward_position(&editor->buffer, query, query_length, before, &found)) {
        if (!wrap || before == length
            || !find_backward_position(&editor->buffer, query, query_length, length, &found)) {
            return 0;
        }
    }

    select_range(editor, found, query_length);
    return 1;
}

int s64_editor_replace_selection(S64Editor* editor, const char* replacement)
{
    unsigned int i;
    unsigned int replacement_length;
    unsigned int selected;

    if (!s64_editor_has_selection(editor)
        || !text_length(replacement, &replacement_length)) {
        return 0;
    }

    selected = s64_editor_selection_end(editor) - s64_editor_selection_start(editor);
    if (replacement_length > (unsigned int)(s64_buffer_free(&editor->buffer) + selected)) {
        return 0;
    }

    if (!delete_selection(editor)) {
        return 0;
    }

    for (i = 0u; i < replacement_length; ++i) {
        if (!s64_buffer_insert(&editor->buffer, (unsigned char)replacement[i])) {
            return 0;
        }
    }

    editor->dirty = 1u;
    editor->selection_anchor = s64_buffer_cursor(&editor->buffer);
    editor->selection_valid = 0u;
    editor->mark_mode = 0u;
    reset_preferred_column(editor);
    s64_editor_recalculate_position(editor);
    s64_editor_ensure_cursor_visible(editor);
    return 1;
}

int s64_editor_replace_all(
    S64Editor* editor,
    const char* query,
    const char* replacement,
    unsigned int* replacements
)
{
    unsigned int query_length;
    unsigned int replacement_length;
    unsigned int count = 0u;
    unsigned int position = 0u;
    unsigned int found;
    unsigned int length = s64_buffer_length(&editor->buffer);
    unsigned long final_length;

    if (replacements != 0) {
        *replacements = 0u;
    }

    if (!text_length(query, &query_length) || query_length == 0u
        || !text_length(replacement, &replacement_length)) {
        return 0;
    }

    while (find_forward_position(&editor->buffer, query, query_length, position, &found)) {
        ++count;
        position = found + query_length;
    }

    final_length = (unsigned long)length;
    if (replacement_length >= query_length) {
        final_length += (unsigned long)count
            * (unsigned long)(replacement_length - query_length);
    } else {
        final_length -= (unsigned long)count
            * (unsigned long)(query_length - replacement_length);
    }

    if (final_length > (unsigned long)S64_BUFFER_CAPACITY) {
        return 0;
    }

    position = 0u;
    while (find_forward_position(&editor->buffer, query, query_length, position, &found)) {
        select_range(editor, found, query_length);
        if (!s64_editor_replace_selection(editor, replacement)) {
            return 0;
        }
        position = s64_buffer_cursor(&editor->buffer);
    }

    if (replacements != 0) {
        *replacements = count;
    }
    return 1;
}

void s64_editor_toggle_insert(S64Editor* editor)
{
    editor->insert_mode = editor->insert_mode ? 0u : 1u;
}

void s64_editor_toggle_word_wrap(S64Editor* editor)
{
    editor->word_wrap = editor->word_wrap ? 0u : 1u;
    editor->view_column = 0u;
    s64_editor_ensure_cursor_visible(editor);
}

void s64_editor_toggle_mark(S64Editor* editor)
{
    if (editor->mark_mode) {
        editor->mark_mode = 0u;
        editor->selection_valid = editor->selection_anchor
            != s64_buffer_cursor(&editor->buffer);
        return;
    }

    editor->selection_anchor = s64_buffer_cursor(&editor->buffer);
    editor->selection_valid = 0u;
    editor->mark_mode = 1u;
}

void s64_editor_select_all(S64Editor* editor)
{
    editor->selection_anchor = 0u;
    s64_buffer_move_cursor(&editor->buffer, s64_buffer_length(&editor->buffer));
    reset_preferred_column(editor);
    editor->selection_valid = s64_buffer_length(&editor->buffer) != 0u;
    editor->mark_mode = 0u;
    s64_editor_recalculate_position(editor);
    s64_editor_ensure_cursor_visible(editor);
}

int s64_editor_copy(const S64Editor* editor, S64Clipboard* clipboard)
{
    unsigned int i;
    unsigned int start;
    unsigned int end;
    unsigned int length;

    if (!s64_editor_has_selection(editor)) {
        return 0;
    }

    start = s64_editor_selection_start(editor);
    end = s64_editor_selection_end(editor);
    length = end - start;

    if (length > S64_CLIPBOARD_CAPACITY) {
        return 0;
    }

    for (i = 0u; i < length; ++i) {
        clipboard->data[i] = s64_buffer_char_at(&editor->buffer, start + i);
    }
    clipboard->length = length;
    return 1;
}

int s64_editor_cut(S64Editor* editor, S64Clipboard* clipboard)
{
    if (!s64_editor_copy(editor, clipboard)) {
        return 0;
    }

    return delete_selection(editor);
}

int s64_editor_paste(S64Editor* editor, const S64Clipboard* clipboard)
{
    unsigned int i;
    unsigned int selected = 0u;

    if (clipboard->length == 0u) {
        return 0;
    }

    if (s64_editor_has_selection(editor)) {
        selected = s64_editor_selection_end(editor) - s64_editor_selection_start(editor);
    }

    if (clipboard->length > (unsigned int)(s64_buffer_free(&editor->buffer) + selected)) {
        return 0;
    }

    delete_selection(editor);

    for (i = 0u; i < clipboard->length; ++i) {
        if (!s64_buffer_insert(&editor->buffer, clipboard->data[i])) {
            return 0;
        }
    }

    editor->dirty = 1u;
    editor->selection_anchor = s64_buffer_cursor(&editor->buffer);
    editor->selection_valid = 0u;
    editor->mark_mode = 0u;
    reset_preferred_column(editor);
    s64_editor_recalculate_position(editor);
    s64_editor_ensure_cursor_visible(editor);
    return 1;
}
