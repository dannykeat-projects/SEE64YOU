#include "editor.h"

#include <stdint.h>

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

static unsigned int wrapped_row_start(const S64Buffer* buffer, unsigned int position)
{
    unsigned int start = line_start(buffer, position);

    return start + ((position - start) / VIEW_COLUMNS) * VIEW_COLUMNS;
}

static unsigned int wrapped_row_end(const S64Buffer* buffer, unsigned int start)
{
    unsigned int position = start;
    unsigned int length = s64_buffer_length(buffer);
    unsigned int limit = start + VIEW_COLUMNS;

    while (position < length
        && position < limit
        && s64_buffer_char_at(buffer, position) != '\n') {
        ++position;
    }
    return position;
}

static unsigned char next_wrapped_row_start(
    const S64Buffer* buffer,
    unsigned int start,
    unsigned int* next
)
{
    unsigned int end = wrapped_row_end(buffer, start);
    unsigned int length = s64_buffer_length(buffer);

    if (end < length && s64_buffer_char_at(buffer, end) == '\n') {
        *next = end + 1u;
        return 1u;
    }
    if (end - start == VIEW_COLUMNS) {
        *next = end;
        return 1u;
    }
    return 0u;
}

static unsigned int previous_wrapped_row_start(
    const S64Buffer* buffer,
    unsigned int start
)
{
    if (start == 0u) {
        return 0u;
    }
    return wrapped_row_start(buffer, start - 1u);
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

static void history_state(const S64Editor* editor, S64HistoryState* state)
{
    state->cursor = s64_buffer_cursor(&editor->buffer);
    state->selection_anchor = editor->selection_anchor;
    state->selection_valid = editor->selection_valid;
    state->mark_mode = editor->mark_mode;
}

static unsigned char history_begin(
    S64Editor* editor,
    unsigned int start,
    unsigned int end
)
{
    S64HistoryState before;

    history_state(editor, &before);
    return (unsigned char)s64_history_capture(
        &editor->history,
        &editor->buffer,
        start,
        end,
        &before,
        editor->dirty
    );
}

static void history_commit(
    S64Editor* editor,
    unsigned int current_length,
    unsigned char coalesce_insert,
    unsigned char captured
)
{
    S64HistoryState after;

    if (!captured) {
        return;
    }

    history_state(editor, &after);
    s64_history_commit(
        &editor->history,
        current_length,
        &after,
        coalesce_insert
    );
}

static void restore_history_state(
    S64Editor* editor,
    const S64HistoryState* state
)
{
    s64_buffer_move_cursor(&editor->buffer, state->cursor);
    editor->selection_anchor = state->selection_anchor;
    editor->selection_valid = state->selection_valid;
    editor->mark_mode = state->mark_mode;
    reset_preferred_column(editor);
    s64_editor_recalculate_position(editor);
    s64_editor_ensure_cursor_visible(editor);
}

static void finish_content_edit(S64Editor* editor)
{
    editor->dirty = 1u;
    editor->selection_anchor = s64_buffer_cursor(&editor->buffer);
    editor->selection_valid = 0u;
    editor->mark_mode = 0u;
    reset_preferred_column(editor);
    s64_editor_recalculate_position(editor);
    s64_editor_ensure_cursor_visible(editor);
}

static void advance_position(S64Editor* editor, unsigned char value)
{
    if (value == '\n') {
        ++editor->line;
        editor->column = 1u;
    } else {
        ++editor->column;
    }
}

static void retreat_position(S64Editor* editor, unsigned char value)
{
    unsigned int cursor;

    if (value != '\n') {
        if (editor->column > 1u) {
            --editor->column;
        }
        return;
    }

    if (editor->line > 1u) {
        --editor->line;
    }
    cursor = s64_buffer_cursor(&editor->buffer);
    editor->column = cursor - line_start(&editor->buffer, cursor) + 1u;
}

static void finish_known_position_edit(S64Editor* editor)
{
    editor->dirty = 1u;
    editor->selection_anchor = s64_buffer_cursor(&editor->buffer);
    editor->selection_valid = 0u;
    editor->mark_mode = 0u;
    reset_preferred_column(editor);
    s64_editor_ensure_cursor_visible(editor);
}

static void finish_appended_value(S64Editor* editor, unsigned char value)
{
    unsigned int next_view_start;

    editor->dirty = 1u;
    editor->selection_anchor = s64_buffer_cursor(&editor->buffer);
    editor->selection_valid = 0u;
    editor->mark_mode = 0u;
    reset_preferred_column(editor);

    if (value == '\n') {
        ++editor->line;
        editor->column = 1u;
    } else {
        ++editor->column;
    }

    if (!editor->word_wrap) {
        s64_editor_ensure_cursor_visible(editor);
        return;
    }

    if (value == '\n') {
        ++editor->view_cursor_row;
        editor->view_cursor_column = 0u;
    } else {
        ++editor->view_cursor_column;
        if (editor->view_cursor_column >= VIEW_COLUMNS) {
            ++editor->view_cursor_row;
            editor->view_cursor_column = 0u;
        }
    }

    if (editor->view_cursor_row >= VIEW_ROWS
        && next_wrapped_row_start(
            &editor->buffer,
            editor->view_start,
            &next_view_start
        )) {
        editor->view_start = next_view_start;
        editor->view_cursor_row = VIEW_ROWS - 1u;
    }
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
    s64_history_break_coalesce(&editor->history);
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

static uint8_t bytes_match(
    const S64Buffer* buffer,
    unsigned int position,
    const char* text,
    unsigned int length
)
{
    unsigned int i;
    unsigned int buffer_length = s64_buffer_length(buffer);

    if (position > buffer_length || length > buffer_length - position) {
        return 0u;
    }

    for (i = 0u; i < length; ++i) {
        if (s64_buffer_char_at(buffer, position + i) != (unsigned char)text[i]) {
            return 0u;
        }
    }

    return 1u;
}

static int insert_text_at(
    S64Buffer* buffer,
    unsigned int position,
    const char* text,
    unsigned int length
)
{
    unsigned int i;

    if (!s64_buffer_move_cursor(buffer, position)) {
        return 0;
    }

    for (i = 0u; i < length; ++i) {
        if (!s64_buffer_insert(buffer, (unsigned char)text[i])) {
            return 0;
        }
    }

    return 1;
}

static void finish_format_edit(
    S64Editor* editor,
    unsigned int selection_start,
    unsigned int selection_end,
    uint8_t preserve_selection
)
{
    if (preserve_selection && selection_end > selection_start) {
        editor->selection_anchor = selection_start;
        s64_buffer_move_cursor(&editor->buffer, selection_end);
        editor->selection_valid = 1u;
    } else {
        editor->selection_anchor = s64_buffer_cursor(&editor->buffer);
        editor->selection_valid = 0u;
    }

    editor->mark_mode = 0u;
    editor->dirty = 1u;
    reset_preferred_column(editor);
    s64_editor_recalculate_position(editor);
    s64_editor_ensure_cursor_visible(editor);
}

static int format_wrapper(
    S64Editor* editor,
    const char* marker,
    unsigned int marker_length,
    uint8_t italic_marker
)
{
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    unsigned int start;
    unsigned int end;
    unsigned int left_stars = 0u;
    unsigned int right_stars = 0u;
    uint8_t remove_existing = 0u;
    unsigned char captured;

    if (!s64_editor_has_selection(editor)) {
        if (s64_buffer_free(&editor->buffer) < marker_length * 2u) {
            return 0;
        }

        captured = history_begin(editor, cursor, cursor);
        if (!insert_text_at(&editor->buffer, cursor, marker, marker_length)
            || !insert_text_at(
                &editor->buffer,
                cursor + marker_length,
                marker,
                marker_length
            )) {
            if (captured) {
                s64_history_reset(&editor->history);
            }
            return 0;
        }

        s64_buffer_move_cursor(&editor->buffer, cursor + marker_length);
        finish_format_edit(editor, 0u, 0u, 0u);
        history_commit(editor, marker_length * 2u, 0u, captured);
        return 1;
    }

    start = s64_editor_selection_start(editor);
    end = s64_editor_selection_end(editor);

    if (italic_marker) {
        while (start > left_stars
            && s64_buffer_char_at(&editor->buffer, start - left_stars - 1u) == '*') {
            ++left_stars;
        }
        while (end + right_stars < s64_buffer_length(&editor->buffer)
            && s64_buffer_char_at(&editor->buffer, end + right_stars) == '*') {
            ++right_stars;
        }

        remove_existing = (left_stars & 1u) != 0u
            && (right_stars & 1u) != 0u;
    } else {
        remove_existing = start >= marker_length
            && bytes_match(
                &editor->buffer,
                start - marker_length,
                marker,
                marker_length
            )
            && bytes_match(&editor->buffer, end, marker, marker_length);
    }

    if (remove_existing) {
        unsigned int remove_length = italic_marker ? 1u : marker_length;

        captured = history_begin(
            editor,
            start - remove_length,
            end + remove_length
        );
        if (!s64_buffer_delete_range(
                &editor->buffer,
                end,
                end + remove_length
            )
            || !s64_buffer_delete_range(
                &editor->buffer,
                start - remove_length,
                start
            )) {
            if (captured) {
                s64_history_reset(&editor->history);
            }
            return 0;
        }

        finish_format_edit(
            editor,
            start - remove_length,
            end - remove_length,
            1u
        );
        history_commit(editor, end - start, 0u, captured);
        return 1;
    }

    if (s64_buffer_free(&editor->buffer) < marker_length * 2u) {
        return 0;
    }

    captured = history_begin(editor, start, end);
    if (!insert_text_at(&editor->buffer, end, marker, marker_length)
        || !insert_text_at(&editor->buffer, start, marker, marker_length)) {
        if (captured) {
            s64_history_reset(&editor->history);
        }
        return 0;
    }

    finish_format_edit(
        editor,
        start + marker_length,
        end + marker_length,
        1u
    );
    history_commit(
        editor,
        (end - start) + marker_length * 2u,
        0u,
        captured
    );
    return 1;
}

static unsigned int heading_prefix_length(
    const S64Buffer* buffer,
    unsigned int start
)
{
    unsigned int length = s64_buffer_length(buffer);
    unsigned int count = 0u;

    while (start + count < length
        && count < 6u
        && s64_buffer_char_at(buffer, start + count) == '#') {
        ++count;
    }

    if (count != 0u
        && start + count < length
        && s64_buffer_char_at(buffer, start + count) == ' ') {
        return count + 1u;
    }

    return 0u;
}

static int replace_line_prefix(
    S64Editor* editor,
    const char* prefix,
    unsigned int prefix_length,
    unsigned int existing_length
)
{
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    unsigned int start = line_start(&editor->buffer, cursor);
    unsigned int new_cursor;
    unsigned int relative = cursor - start;
    unsigned int extra = prefix_length > existing_length
        ? prefix_length - existing_length
        : 0u;
    unsigned char captured;

    if (extra > s64_buffer_free(&editor->buffer)) {
        return 0;
    }

    captured = history_begin(editor, start, start + existing_length);

    if (existing_length != 0u
        && !s64_buffer_delete_range(
            &editor->buffer,
            start,
            start + existing_length
        )) {
        if (captured) {
            s64_history_reset(&editor->history);
        }
        return 0;
    }

    if (prefix_length != 0u
        && !insert_text_at(&editor->buffer, start, prefix, prefix_length)) {
        if (captured) {
            s64_history_reset(&editor->history);
        }
        return 0;
    }

    if (relative <= existing_length) {
        new_cursor = start + (relative < prefix_length ? relative : prefix_length);
    } else {
        new_cursor = cursor - existing_length + prefix_length;
    }
    s64_buffer_move_cursor(&editor->buffer, new_cursor);
    finish_format_edit(editor, 0u, 0u, 0u);
    history_commit(editor, prefix_length, 0u, captured);
    return 1;
}

static uint8_t ascii_equal_ci(char left, char right)
{
    if (left >= 'A' && left <= 'Z') left = (char)(left + ('a' - 'A'));
    if (right >= 'A' && right <= 'Z') right = (char)(right + ('a' - 'A'));
    return left == right;
}

static uint8_t url_has_supported_scheme(const char* url)
{
    static const char http_scheme[] = "http://";
    static const char https_scheme[] = "https://";
    unsigned int i;

    if (url == 0) return 0u;

    for (i = 0u; i < sizeof(http_scheme) - 1u; ++i) {
        if (url[i] == '\0' || !ascii_equal_ci(url[i], http_scheme[i])) break;
    }
    if (i == sizeof(http_scheme) - 1u) return 1u;

    for (i = 0u; i < sizeof(https_scheme) - 1u; ++i) {
        if (url[i] == '\0' || !ascii_equal_ci(url[i], https_scheme[i])) return 0u;
    }
    return 1u;
}

static uint8_t link_url_valid(const char* url, unsigned int* length)
{
    unsigned int i = 0u;
    unsigned char value;

    if (url == 0 || url[0] == '\0' || length == 0
        || !url_has_supported_scheme(url)) {
        return 0u;
    }

    while (url[i] != '\0') {
        if (i >= 255u) {
            return 0u;
        }

        value = (unsigned char)url[i];
        if (value <= 0x20u || value > 0x7eu || value == ')') {
            return 0u;
        }
        ++i;
    }

    *length = i;
    return 1u;
}

static uint8_t selection_link_label_valid(
    const S64Editor* editor,
    unsigned int start,
    unsigned int end
)
{
    unsigned int i;

    for (i = start; i < end; ++i) {
        unsigned char value = s64_buffer_char_at(&editor->buffer, i);
        if (value == ']' || value == '\n' || value == '\r') {
            return 0u;
        }
    }

    return 1u;
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
    s64_history_break_coalesce(&editor->history);
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
    s64_history_init(&editor->history);
    editor->view_start = 0u;
    editor->view_column = 0u;
    editor->line = 1u;
    editor->column = 1u;
    editor->selection_anchor = 0u;
    editor->preferred_column = 0u;
    editor->view_cursor_row = 0u;
    editor->view_cursor_column = 0u;
    editor->selection_valid = 0u;
    editor->preferred_column_valid = 0u;
    editor->mark_mode = 0u;
    editor->dirty = 0u;
    editor->insert_mode = 1u;
    editor->word_wrap = 1u;
}

void s64_editor_document_loaded(S64Editor* editor)
{
    s64_history_reset(&editor->history);
    editor->view_start = 0u;
    editor->view_column = 0u;
    editor->line = 1u;
    editor->column = 1u;
    editor->selection_anchor = s64_buffer_cursor(&editor->buffer);
    editor->preferred_column = 0u;
    editor->view_cursor_row = 0u;
    editor->view_cursor_column = 0u;
    editor->selection_valid = 0u;
    editor->preferred_column_valid = 0u;
    editor->mark_mode = 0u;
    editor->dirty = 0u;
}

void s64_editor_mark_saved(S64Editor* editor)
{
    editor->dirty = 0u;
    s64_history_mark_saved(&editor->history);
}

int s64_editor_undo(S64Editor* editor)
{
    S64HistoryState state;

    if (!s64_history_undo(
            &editor->history,
            &editor->buffer,
            &state,
            &editor->dirty
        )) {
        return 0;
    }

    restore_history_state(editor, &state);
    return 1;
}

int s64_editor_redo(S64Editor* editor)
{
    S64HistoryState state;

    if (!s64_history_redo(
            &editor->history,
            &editor->buffer,
            &state,
            &editor->dirty
        )) {
        return 0;
    }

    restore_history_state(editor, &state);
    return 1;
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
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    unsigned int cursor_start = line_start(&editor->buffer, cursor);
    unsigned int cursor_column = cursor - cursor_start;

    if (editor->word_wrap) {
        unsigned int cursor_row_start = wrapped_row_start(&editor->buffer, cursor);
        unsigned int row_start = editor->view_start;
        unsigned int previous;

        editor->view_column = 0u;

        if (cursor_row_start < editor->view_start) {
            editor->view_start = cursor_row_start;
            editor->view_cursor_row = 0u;
            editor->view_cursor_column = (unsigned char)(cursor - cursor_row_start);
            return;
        }

        for (row = 0u; row < VIEW_ROWS; ++row) {
            if (row_start == cursor_row_start) {
                editor->view_cursor_row = (unsigned char)row;
                editor->view_cursor_column = (unsigned char)(cursor - cursor_row_start);
                return;
            }
            if (!next_wrapped_row_start(&editor->buffer, row_start, &row_start)) {
                break;
            }
        }

        editor->view_start = cursor_row_start;
        editor->view_cursor_row = 0u;
        for (row = 1u; row < VIEW_ROWS; ++row) {
            previous = previous_wrapped_row_start(
                &editor->buffer,
                editor->view_start
            );
            if (previous == editor->view_start) {
                break;
            }
            editor->view_start = previous;
            ++editor->view_cursor_row;
        }
        editor->view_cursor_column = (unsigned char)(cursor - cursor_row_start);
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
    editor->view_cursor_row = (unsigned char)row;
    editor->view_cursor_column = (unsigned char)(cursor_column - editor->view_column);
}

static int editor_insert_value(
    S64Editor* editor,
    unsigned char value,
    unsigned char coalesce_insert
)
{
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    unsigned int length = s64_buffer_length(&editor->buffer);
    unsigned int start = cursor;
    unsigned int end = cursor;
    unsigned char had_selection = s64_editor_has_selection(editor);
    unsigned char appended = !had_selection && cursor == length;
    unsigned char captured;
    S64HistoryState after;

    if (editor->insert_mode
        && !had_selection
        && s64_buffer_free(&editor->buffer) == 0u) {
        return 0;
    }

    if (coalesce_insert
        && editor->insert_mode
        && !had_selection
        && s64_history_can_extend_insert(&editor->history, cursor)) {
        if (!s64_buffer_insert(&editor->buffer, value)) {
            return 0;
        }

        if (appended) {
            finish_appended_value(editor, value);
        } else {
            advance_position(editor, value);
            finish_known_position_edit(editor);
        }
        history_state(editor, &after);
        s64_history_extend_insert(&editor->history, &after);
        return 1;
    }

    if (had_selection) {
        start = s64_editor_selection_start(editor);
        end = s64_editor_selection_end(editor);
        if (!editor->insert_mode && end < length) {
            ++end;
        }
    } else if (!editor->insert_mode && cursor < length) {
        end = cursor + 1u;
    }

    captured = history_begin(editor, start, end);

    if (had_selection) {
        delete_selection(editor);
    }

    if (!editor->insert_mode
        && s64_buffer_cursor(&editor->buffer) < s64_buffer_length(&editor->buffer)) {
        s64_buffer_delete(&editor->buffer);
    }

    if (!s64_buffer_insert(&editor->buffer, value)) {
        if (captured) {
            s64_history_reset(&editor->history);
        }
        return 0;
    }

    if (appended) {
        finish_appended_value(editor, value);
    } else if (!had_selection) {
        advance_position(editor, value);
        finish_known_position_edit(editor);
    } else {
        finish_content_edit(editor);
    }
    history_commit(
        editor,
        1u,
        (unsigned char)(coalesce_insert && editor->insert_mode && !had_selection),
        captured
    );
    return 1;
}

int s64_editor_insert(S64Editor* editor, unsigned char value)
{
    return editor_insert_value(editor, value, 1u);
}

int s64_editor_newline(S64Editor* editor)
{
    return editor_insert_value(editor, '\n', 0u);
}

int s64_editor_backspace(S64Editor* editor)
{
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    unsigned int start;
    unsigned int end;
    unsigned char captured;
    unsigned char removed;

    if (s64_editor_has_selection(editor)) {
        start = s64_editor_selection_start(editor);
        end = s64_editor_selection_end(editor);
        captured = history_begin(editor, start, end);
        if (!delete_selection(editor)) {
            return 0;
        }
        history_commit(editor, 0u, 0u, captured);
        return 1;
    }

    if (cursor == 0u) {
        return 0;
    }

    removed = s64_buffer_char_at(&editor->buffer, cursor - 1u);
    captured = history_begin(editor, cursor - 1u, cursor);
    if (!s64_buffer_backspace(&editor->buffer)) {
        if (captured) {
            s64_history_reset(&editor->history);
        }
        return 0;
    }

    retreat_position(editor, removed);
    finish_known_position_edit(editor);
    history_commit(editor, 0u, 0u, captured);
    return 1;
}

int s64_editor_delete(S64Editor* editor)
{
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    unsigned int start;
    unsigned int end;
    unsigned char captured;

    if (s64_editor_has_selection(editor)) {
        start = s64_editor_selection_start(editor);
        end = s64_editor_selection_end(editor);
        captured = history_begin(editor, start, end);
        if (!delete_selection(editor)) {
            return 0;
        }
        history_commit(editor, 0u, 0u, captured);
        return 1;
    }

    if (cursor >= s64_buffer_length(&editor->buffer)) {
        return 0;
    }

    captured = history_begin(editor, cursor, cursor + 1u);
    if (!s64_buffer_delete(&editor->buffer)) {
        if (captured) {
            s64_history_reset(&editor->history);
        }
        return 0;
    }

    finish_known_position_edit(editor);
    history_commit(editor, 0u, 0u, captured);
    return 1;
}

void s64_editor_left(S64Editor* editor)
{
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    unsigned char value;

    s64_history_break_coalesce(&editor->history);

    if (cursor > 0u) {
        value = s64_buffer_char_at(&editor->buffer, cursor - 1u);
        s64_buffer_move_cursor(&editor->buffer, cursor - 1u);
        retreat_position(editor, value);
        reset_preferred_column(editor);
        update_selection_after_move(editor);
        s64_editor_ensure_cursor_visible(editor);
    }
}

void s64_editor_right(S64Editor* editor)
{
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);
    unsigned char value;

    s64_history_break_coalesce(&editor->history);

    if (cursor < s64_buffer_length(&editor->buffer)) {
        value = s64_buffer_char_at(&editor->buffer, cursor);
        s64_buffer_move_cursor(&editor->buffer, cursor + 1u);
        advance_position(editor, value);
        reset_preferred_column(editor);
        update_selection_after_move(editor);
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

    s64_history_break_coalesce(&editor->history);

    if (editor->word_wrap) {
        unsigned int current_row = wrapped_row_start(&editor->buffer, cursor);
        unsigned int previous_row;
        unsigned int previous_end;

        if (current_row == 0u) {
            return;
        }

        desired_column = preferred_column(editor, cursor, current_row);
        previous_row = previous_wrapped_row_start(&editor->buffer, current_row);
        previous_end = wrapped_row_end(&editor->buffer, previous_row);
        s64_buffer_move_cursor(
            &editor->buffer,
            previous_row + desired_column > previous_end
                ? previous_end
                : previous_row + desired_column
        );
        update_selection_after_move(editor);
        s64_editor_recalculate_position(editor);
        s64_editor_ensure_cursor_visible(editor);
        return;
    }

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

    s64_history_break_coalesce(&editor->history);

    if (editor->word_wrap) {
        unsigned int current_row = wrapped_row_start(&editor->buffer, cursor);
        unsigned int next_row;
        unsigned int next_end;

        if (!next_wrapped_row_start(&editor->buffer, current_row, &next_row)) {
            return;
        }

        desired_column = preferred_column(editor, cursor, current_row);
        next_end = wrapped_row_end(&editor->buffer, next_row);
        s64_buffer_move_cursor(
            &editor->buffer,
            next_row + desired_column > next_end
                ? next_end
                : next_row + desired_column
        );
        update_selection_after_move(editor);
        s64_editor_recalculate_position(editor);
        s64_editor_ensure_cursor_visible(editor);
        return;
    }

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

    s64_history_break_coalesce(&editor->history);
    s64_buffer_move_cursor(&editor->buffer, line_start(&editor->buffer, cursor));
    reset_preferred_column(editor);
    update_selection_after_move(editor);
    s64_editor_recalculate_position(editor);
    s64_editor_ensure_cursor_visible(editor);
}

void s64_editor_end(S64Editor* editor)
{
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);

    s64_history_break_coalesce(&editor->history);
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

    s64_history_break_coalesce(&editor->history);

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

    s64_history_break_coalesce(&editor->history);

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

    s64_history_break_coalesce(&editor->history);

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

static int replace_selection_core(
    S64Editor* editor,
    const char* replacement,
    unsigned int replacement_length,
    unsigned char record_history
)
{
    unsigned int i;
    unsigned int start;
    unsigned int end;
    unsigned int selected;
    unsigned char captured = 0u;

    if (!s64_editor_has_selection(editor)) {
        return 0;
    }

    start = s64_editor_selection_start(editor);
    end = s64_editor_selection_end(editor);
    selected = end - start;
    if (replacement_length > (unsigned int)(s64_buffer_free(&editor->buffer) + selected)) {
        return 0;
    }

    if (record_history) {
        captured = history_begin(editor, start, end);
    }

    if (!delete_selection(editor)) {
        if (captured) {
            s64_history_reset(&editor->history);
        }
        return 0;
    }

    for (i = 0u; i < replacement_length; ++i) {
        if (!s64_buffer_insert(&editor->buffer, (unsigned char)replacement[i])) {
            if (captured) {
                s64_history_reset(&editor->history);
            }
            return 0;
        }
    }

    finish_content_edit(editor);
    if (record_history) {
        history_commit(editor, replacement_length, 0u, captured);
    }
    return 1;
}

int s64_editor_replace_selection(S64Editor* editor, const char* replacement)
{
    unsigned int replacement_length;

    if (!text_length(replacement, &replacement_length)) {
        return 0;
    }

    return replace_selection_core(
        editor,
        replacement,
        replacement_length,
        1u
    );
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
    unsigned char captured = 0u;

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

    if (count == 0u) {
        return 1;
    }

    captured = history_begin(editor, 0u, length);
    position = 0u;
    while (find_forward_position(&editor->buffer, query, query_length, position, &found)) {
        select_range(editor, found, query_length);
        if (!replace_selection_core(
                editor,
                replacement,
                replacement_length,
                0u
            )) {
            if (captured) {
                s64_history_reset(&editor->history);
            }
            return 0;
        }
        position = s64_buffer_cursor(&editor->buffer);
    }

    history_commit(
        editor,
        s64_buffer_length(&editor->buffer),
        0u,
        captured
    );

    if (replacements != 0) {
        *replacements = count;
    }
    return 1;
}

void s64_editor_toggle_insert(S64Editor* editor)
{
    s64_history_break_coalesce(&editor->history);
    editor->insert_mode = editor->insert_mode ? 0u : 1u;
}

void s64_editor_toggle_word_wrap(S64Editor* editor)
{
    editor->word_wrap = editor->word_wrap ? 0u : 1u;
    editor->view_column = 0u;
    s64_editor_ensure_cursor_visible(editor);
}

int s64_editor_format_bold(S64Editor* editor)
{
    return format_wrapper(editor, "**", 2u, 0u);
}

int s64_editor_format_italic(S64Editor* editor)
{
    return format_wrapper(editor, "*", 1u, 1u);
}

int s64_editor_format_heading(S64Editor* editor, unsigned char level)
{
    unsigned int cursor;
    unsigned int start;
    unsigned int existing;
    char prefix[8];
    unsigned int i;

    if (editor == 0 || level < 1u || level > 2u) {
        return 0;
    }

    cursor = s64_buffer_cursor(&editor->buffer);
    start = line_start(&editor->buffer, cursor);
    existing = heading_prefix_length(&editor->buffer, start);

    if (existing == (unsigned int)level + 1u) {
        return replace_line_prefix(editor, "", 0u, existing);
    }

    for (i = 0u; i < level; ++i) {
        prefix[i] = '#';
    }
    prefix[level] = ' ';
    prefix[level + 1u] = '\0';

    return replace_line_prefix(
        editor,
        prefix,
        (unsigned int)level + 1u,
        existing
    );
}

int s64_editor_format_list_item(S64Editor* editor)
{
    unsigned int cursor;
    unsigned int start;
    unsigned int existing = 0u;

    if (editor == 0) {
        return 0;
    }

    cursor = s64_buffer_cursor(&editor->buffer);
    start = line_start(&editor->buffer, cursor);

    if (bytes_match(&editor->buffer, start, "- ", 2u)) {
        existing = 2u;
        return replace_line_prefix(editor, "", 0u, existing);
    }

    return replace_line_prefix(editor, "- ", 2u, 0u);
}

int s64_editor_format_link(S64Editor* editor, const char* url)
{
    unsigned int start;
    unsigned int end;
    unsigned int url_length;
    unsigned int suffix_length;
    unsigned int i;
    unsigned char captured;

    if (editor == 0 || !s64_editor_has_selection(editor)
        || !link_url_valid(url, &url_length)) {
        return 0;
    }

    start = s64_editor_selection_start(editor);
    end = s64_editor_selection_end(editor);
    if (!selection_link_label_valid(editor, start, end)) {
        return 0;
    }

    suffix_length = url_length + 3u;

    if (s64_buffer_free(&editor->buffer) < suffix_length + 1u) {
        return 0;
    }

    captured = history_begin(editor, start, end);

    if (!insert_text_at(&editor->buffer, end, "](", 2u)) {
        if (captured) {
            s64_history_reset(&editor->history);
        }
        return 0;
    }
    for (i = 0u; i < url_length; ++i) {
        if (!s64_buffer_insert(&editor->buffer, (unsigned char)url[i])) {
            if (captured) {
                s64_history_reset(&editor->history);
            }
            return 0;
        }
    }
    if (!s64_buffer_insert(&editor->buffer, ')')
        || !insert_text_at(&editor->buffer, start, "[", 1u)) {
        if (captured) {
            s64_history_reset(&editor->history);
        }
        return 0;
    }

    finish_format_edit(editor, start + 1u, end + 1u, 1u);
    history_commit(
        editor,
        (end - start) + url_length + 4u,
        0u,
        captured
    );
    return 1;
}

void s64_editor_toggle_mark(S64Editor* editor)
{
    s64_history_break_coalesce(&editor->history);
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
    s64_history_break_coalesce(&editor->history);
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
    unsigned int start;
    unsigned int end;
    unsigned char captured;

    if (!s64_editor_copy(editor, clipboard)) {
        return 0;
    }

    start = s64_editor_selection_start(editor);
    end = s64_editor_selection_end(editor);
    captured = history_begin(editor, start, end);

    if (!delete_selection(editor)) {
        if (captured) {
            s64_history_reset(&editor->history);
        }
        return 0;
    }

    history_commit(editor, 0u, 0u, captured);
    return 1;
}

int s64_editor_paste(S64Editor* editor, const S64Clipboard* clipboard)
{
    unsigned int i;
    unsigned int selected = 0u;
    unsigned int start = s64_buffer_cursor(&editor->buffer);
    unsigned int end = start;
    unsigned char captured;

    if (clipboard->length == 0u) {
        return 0;
    }

    if (s64_editor_has_selection(editor)) {
        start = s64_editor_selection_start(editor);
        end = s64_editor_selection_end(editor);
        selected = end - start;
    }

    if (clipboard->length > (unsigned int)(s64_buffer_free(&editor->buffer) + selected)) {
        return 0;
    }

    captured = history_begin(editor, start, end);
    if (selected != 0u && !delete_selection(editor)) {
        if (captured) {
            s64_history_reset(&editor->history);
        }
        return 0;
    }

    for (i = 0u; i < clipboard->length; ++i) {
        if (!s64_buffer_insert(&editor->buffer, clipboard->data[i])) {
            if (captured) {
                s64_history_reset(&editor->history);
            }
            return 0;
        }
    }

    finish_content_edit(editor);
    history_commit(editor, clipboard->length, 0u, captured);
    return 1;
}
