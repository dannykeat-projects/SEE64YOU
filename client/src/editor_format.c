#include "editor.h"

#include <stdint.h>

#define POSITION_INVALID (S64_BUFFER_CAPACITY + 1u)

/*
 * Formatting lives in a separate translation unit so the conventional-C64
 * editor can omit dormant formatting code until the FORMAT UI is wired.
 * Host validation and cc65 module checks still compile and exercise it.
 */

static unsigned int format_line_start(
    const S64Buffer* buffer,
    unsigned int position
)
{
    while (position > 0u) {
        if (s64_buffer_char_at(buffer, position - 1u) == '\n') {
            break;
        }
        --position;
    }
    return position;
}

static void format_reset_preferred_column(S64Editor* editor)
{
    editor->preferred_column_valid = 0u;
}

static void format_history_state(
    const S64Editor* editor,
    S64HistoryState* state
)
{
    state->cursor = s64_buffer_cursor(&editor->buffer);
    state->selection_anchor = editor->selection_anchor;
    state->selection_valid = editor->selection_valid;
    state->mark_mode = editor->mark_mode;
}

static unsigned char format_history_begin(
    S64Editor* editor,
    unsigned int start,
    unsigned int end
)
{
    S64HistoryState before;

    format_history_state(editor, &before);
    return (unsigned char)s64_history_capture(
        &editor->history,
        &editor->buffer,
        start,
        end,
        &before,
        editor->dirty
    );
}

static void format_history_commit(
    S64Editor* editor,
    unsigned int current_length,
    unsigned char captured
)
{
    S64HistoryState after;

    if (!captured) {
        return;
    }

    format_history_state(editor, &after);
    s64_history_commit(
        &editor->history,
        current_length,
        &after,
        0u
    );
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
    format_reset_preferred_column(editor);
    editor->position_cursor = POSITION_INVALID;
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

        captured = format_history_begin(editor, cursor, cursor);
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
        format_history_commit(editor, marker_length * 2u, captured);
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

        captured = format_history_begin(
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
        format_history_commit(editor, end - start, captured);
        return 1;
    }

    if (s64_buffer_free(&editor->buffer) < marker_length * 2u) {
        return 0;
    }

    captured = format_history_begin(editor, start, end);
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
    format_history_commit(editor, (end - start) + marker_length * 2u, captured);
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
    unsigned int start = format_line_start(&editor->buffer, cursor);
    unsigned int new_cursor;
    unsigned int relative = cursor - start;
    unsigned int extra = prefix_length > existing_length
        ? prefix_length - existing_length
        : 0u;
    unsigned char captured;

    if (extra > s64_buffer_free(&editor->buffer)) {
        return 0;
    }

    captured = format_history_begin(editor, start, start + existing_length);

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
    format_history_commit(editor, prefix_length, captured);
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
    start = format_line_start(&editor->buffer, cursor);
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
    start = format_line_start(&editor->buffer, cursor);

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

    captured = format_history_begin(editor, start, end);

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
    format_history_commit(editor, (end - start) + url_length + 4u, captured);
    return 1;
}

