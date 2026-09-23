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

void s64_editor_init(S64Editor* editor)
{
    s64_buffer_init(&editor->buffer);
    editor->view_start = 0u;
    editor->line = 1u;
    editor->column = 1u;
    editor->dirty = 0u;
    editor->insert_mode = 1u;
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

    if (cursor < editor->view_start) {
        editor->view_start = line_start(&editor->buffer, cursor);
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
            editor->view_start = line_start(&editor->buffer, cursor);
            return;
        }
    }
}

int s64_editor_insert(S64Editor* editor, unsigned char value)
{
    int changed;

    if (!editor->insert_mode && s64_buffer_cursor(&editor->buffer) < s64_buffer_length(&editor->buffer)) {
        s64_buffer_delete(&editor->buffer);
    }

    changed = s64_buffer_insert(&editor->buffer, value);
    if (changed) {
        editor->dirty = 1u;
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
    int changed = s64_buffer_backspace(&editor->buffer);
    if (changed) {
        editor->dirty = 1u;
        s64_editor_recalculate_position(editor);
        s64_editor_ensure_cursor_visible(editor);
    }
    return changed;
}

int s64_editor_delete(S64Editor* editor)
{
    int changed = s64_buffer_delete(&editor->buffer);
    if (changed) {
        editor->dirty = 1u;
        s64_editor_recalculate_position(editor);
    }
    return changed;
}

void s64_editor_left(S64Editor* editor)
{
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    if (cursor > 0u) {
        s64_buffer_move_cursor(&editor->buffer, cursor - 1u);
        s64_editor_recalculate_position(editor);
        s64_editor_ensure_cursor_visible(editor);
    }
}

void s64_editor_right(S64Editor* editor)
{
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    if (cursor < s64_buffer_length(&editor->buffer)) {
        s64_buffer_move_cursor(&editor->buffer, cursor + 1u);
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

    desired_column = cursor - current_start;
    previous_end = current_start - 1u;
    previous_start = line_start(&editor->buffer, previous_end);

    if (previous_start + desired_column > previous_end) {
        s64_buffer_move_cursor(&editor->buffer, previous_end);
    } else {
        s64_buffer_move_cursor(&editor->buffer, previous_start + desired_column);
    }

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

    desired_column = cursor - current_start;
    next_start = current_end + 1u;
    next_end = line_end(&editor->buffer, next_start);

    if (next_start + desired_column > next_end) {
        s64_buffer_move_cursor(&editor->buffer, next_end);
    } else {
        s64_buffer_move_cursor(&editor->buffer, next_start + desired_column);
    }

    s64_editor_recalculate_position(editor);
    s64_editor_ensure_cursor_visible(editor);
}
