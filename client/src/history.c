#include "history.h"

#define S64_HISTORY_SIDE_NONE 0u
#define S64_HISTORY_SIDE_BEFORE 1u
#define S64_HISTORY_SIDE_AFTER 2u

static unsigned char current_side(const S64History* history)
{
    return history->applied ? S64_HISTORY_SIDE_AFTER : S64_HISTORY_SIDE_BEFORE;
}

static void update_dirty(const S64History* history, unsigned char* dirty)
{
    if (dirty == 0) {
        return;
    }

    *dirty = history->saved_side == S64_HISTORY_SIDE_NONE
        || history->saved_side != current_side(history);
}

static int swap_range(S64History* history, S64Buffer* buffer)
{
    unsigned int i;
    unsigned int common;
    unsigned int old_current_length;
    unsigned int length = s64_buffer_length(buffer);
    unsigned long final_length;

    if (history->position > length
        || history->current_length > length - history->position
        || history->stored_length > S64_HISTORY_CAPACITY
        || history->current_length > S64_HISTORY_CAPACITY) {
        return 0;
    }

    final_length = (unsigned long)length
        - (unsigned long)history->current_length
        + (unsigned long)history->stored_length;
    if (final_length > (unsigned long)S64_BUFFER_CAPACITY
        || !s64_buffer_move_cursor(buffer, history->position)) {
        return 0;
    }

    common = history->current_length < history->stored_length
        ? history->current_length
        : history->stored_length;

    for (i = 0u; i < common; ++i) {
        unsigned char current = s64_buffer_char_at(
            buffer,
            history->position + i
        );
        unsigned char target = history->data[i];

        if (!s64_buffer_delete(buffer) || !s64_buffer_insert(buffer, target)) {
            return 0;
        }
        history->data[i] = current;
    }

    if (history->stored_length > history->current_length) {
        for (i = history->current_length; i < history->stored_length; ++i) {
            if (!s64_buffer_insert(buffer, history->data[i])) {
                return 0;
            }
        }
    } else {
        for (i = history->stored_length; i < history->current_length; ++i) {
            history->data[i] = s64_buffer_char_at(
                buffer,
                history->position + history->stored_length
            );
            if (!s64_buffer_delete(buffer)) {
                return 0;
            }
        }
    }

    old_current_length = history->current_length;
    history->current_length = history->stored_length;
    history->stored_length = old_current_length;
    history->applied = history->applied ? 0u : 1u;
    history->coalesce_insert = 0u;
    return 1;
}

void s64_history_init(S64History* history)
{
    s64_history_reset(history);
}

void s64_history_reset(S64History* history)
{
    history->position = 0u;
    history->stored_length = 0u;
    history->current_length = 0u;
    history->before.cursor = 0u;
    history->before.selection_anchor = 0u;
    history->before.selection_valid = 0u;
    history->before.mark_mode = 0u;
    history->after = history->before;
    history->valid = 0u;
    history->applied = 0u;
    history->saved_side = S64_HISTORY_SIDE_NONE;
    history->coalesce_insert = 0u;
}

int s64_history_capture(
    S64History* history,
    const S64Buffer* buffer,
    unsigned int start,
    unsigned int end,
    const S64HistoryState* before,
    unsigned char dirty
)
{
    unsigned int i;
    unsigned int length;

    if (history == 0 || buffer == 0 || before == 0) {
        return 0;
    }

    length = s64_buffer_length(buffer);
    if (start > end || end > length
        || end - start > S64_HISTORY_CAPACITY) {
        s64_history_reset(history);
        return 0;
    }

    history->position = start;
    history->stored_length = end - start;
    history->current_length = 0u;
    for (i = 0u; i < history->stored_length; ++i) {
        history->data[i] = s64_buffer_char_at(buffer, start + i);
    }

    history->before = *before;
    history->after = *before;
    history->valid = 0u;
    history->applied = 0u;
    history->saved_side = dirty
        ? S64_HISTORY_SIDE_NONE
        : S64_HISTORY_SIDE_BEFORE;
    history->coalesce_insert = 0u;
    return 1;
}

void s64_history_commit(
    S64History* history,
    unsigned int current_length,
    const S64HistoryState* after,
    unsigned char coalesce_insert
)
{
    if (history == 0 || after == 0
        || current_length > S64_HISTORY_CAPACITY) {
        if (history != 0) {
            s64_history_reset(history);
        }
        return;
    }

    history->current_length = current_length;
    history->after = *after;
    history->valid = 1u;
    history->applied = 1u;
    history->coalesce_insert = coalesce_insert ? 1u : 0u;
}

unsigned char s64_history_can_extend_insert(
    const S64History* history,
    unsigned int cursor
)
{
    return history != 0
        && history->valid
        && history->applied
        && history->coalesce_insert
        && history->stored_length == 0u
        && history->position + history->current_length == cursor
        && history->current_length < S64_HISTORY_CAPACITY;
}

void s64_history_extend_insert(
    S64History* history,
    const S64HistoryState* after
)
{
    if (history == 0 || after == 0
        || history->current_length >= S64_HISTORY_CAPACITY) {
        return;
    }

    ++history->current_length;
    history->after = *after;
}

void s64_history_break_coalesce(S64History* history)
{
    if (history != 0) {
        history->coalesce_insert = 0u;
    }
}

void s64_history_mark_saved(S64History* history)
{
    if (history == 0) {
        return;
    }

    if (history->valid) {
        history->saved_side = current_side(history);
    }
    history->coalesce_insert = 0u;
}

int s64_history_undo(
    S64History* history,
    S64Buffer* buffer,
    S64HistoryState* state,
    unsigned char* dirty
)
{
    if (history == 0 || buffer == 0 || state == 0
        || !history->valid || !history->applied
        || !swap_range(history, buffer)) {
        return 0;
    }

    *state = history->before;
    update_dirty(history, dirty);
    return 1;
}

int s64_history_redo(
    S64History* history,
    S64Buffer* buffer,
    S64HistoryState* state,
    unsigned char* dirty
)
{
    if (history == 0 || buffer == 0 || state == 0
        || !history->valid || history->applied
        || !swap_range(history, buffer)) {
        return 0;
    }

    *state = history->after;
    update_dirty(history, dirty);
    return 1;
}
