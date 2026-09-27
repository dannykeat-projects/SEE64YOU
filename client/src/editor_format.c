#include "editor.h"
#include "editor_internal.h"

#include <stdint.h>

/*
 * Formatting lives in a separate translation unit so the conventional-C64
 * editor can omit dormant formatting code until the FORMAT UI is wired.
 * Host validation and cc65 module checks still compile and exercise it.
 */

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
    S64HistoryState state;
    unsigned int cursor = s64_buffer_cursor(&editor->buffer);

    state.cursor = preserve_selection ? selection_end : cursor;
    state.selection_anchor = preserve_selection ? selection_start : cursor;
    state.selection_valid = preserve_selection
        && selection_end > selection_start;
    state.mark_mode = 0u;

    s64_editor_internal_restore_history_state(editor, &state);
    editor->dirty = 1u;
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

        captured = s64_editor_internal_history_begin(editor, cursor, cursor);
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
        s64_editor_internal_history_commit(editor, marker_length * 2u, 0u, captured);
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

        captured = s64_editor_internal_history_begin(
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
        s64_editor_internal_history_commit(editor, end - start, 0u, captured);
        return 1;
    }

    if (s64_buffer_free(&editor->buffer) < marker_length * 2u) {
        return 0;
    }

    captured = s64_editor_internal_history_begin(editor, start, end);
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
    s64_editor_internal_history_commit(editor, (end - start) + marker_length * 2u, 0u, captured);
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
    unsigned int start = s64_editor_internal_line_start(&editor->buffer, cursor);
    unsigned int new_cursor;
    unsigned int relative = cursor - start;
    unsigned int extra = prefix_length > existing_length
        ? prefix_length - existing_length
        : 0u;
    unsigned char captured;

    if (extra > s64_buffer_free(&editor->buffer)) {
        return 0;
    }

    captured = s64_editor_internal_history_begin(editor, start, start + existing_length);

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
    s64_editor_internal_history_commit(editor, prefix_length, 0u, captured);
    return 1;
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
    start = s64_editor_internal_line_start(&editor->buffer, cursor);
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
    start = s64_editor_internal_line_start(&editor->buffer, cursor);

    if (bytes_match(&editor->buffer, start, "- ", 2u)) {
        existing = 2u;
        return replace_line_prefix(editor, "", 0u, existing);
    }

    return replace_line_prefix(editor, "- ", 2u, 0u);
}


