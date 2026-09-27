#include "editor.h"
#include "editor_internal.h"

#include <stdint.h>

/*
 * Link formatting is isolated from the basic formatting object because its URL
 * validation and source construction are comparatively large on a 6502.
 */

static int link_insert_text_at(
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

static void link_finish_edit(
    S64Editor* editor,
    unsigned int selection_start,
    unsigned int selection_end
)
{
    S64HistoryState state;

    state.cursor = selection_end;
    state.selection_anchor = selection_start;
    state.selection_valid = selection_end > selection_start;
    state.mark_mode = 0u;

    s64_editor_internal_restore_history_state(editor, &state);
    editor->dirty = 1u;
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

    captured = s64_editor_internal_history_begin(editor, start, end);

    if (!link_insert_text_at(&editor->buffer, end, "](", 2u)) {
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
        || !link_insert_text_at(&editor->buffer, start, "[", 1u)) {
        if (captured) {
            s64_history_reset(&editor->history);
        }
        return 0;
    }

    link_finish_edit(editor, start + 1u, end + 1u);
    s64_editor_internal_history_commit(editor, (end - start) + url_length + 4u, 0u, captured);
    return 1;
}

