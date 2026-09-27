#include "history.h"
#include "platform/reu.h"

#ifndef S64_REU_BUFFER
#error "history_reu.c requires S64_REU_BUFFER"
#endif

#define S64_REU_HISTORY_BANK 0u
#define S64_REU_HISTORY_BASE 61440u
#define S64_HISTORY_TRANSFER_SIZE 64u
#define S64_HISTORY_SLOT_NONE 0xffu

static unsigned char history_target[S64_HISTORY_TRANSFER_SIZE];
static unsigned char history_current[S64_HISTORY_TRANSFER_SIZE];

static unsigned int payload_address(unsigned char slot, unsigned int offset)
{
    return (unsigned int)(
        S64_REU_HISTORY_BASE
        + ((unsigned int)slot * S64_HISTORY_CAPACITY)
        + offset
    );
}

static unsigned char payload_load(
    unsigned char slot,
    unsigned int offset,
    unsigned char* data,
    unsigned int length
)
{
    if (length == 0u) {
        return 1u;
    }

    return s64_reu_load(
        S64_REU_HISTORY_BANK,
        (uint16_t)payload_address(slot, offset),
        data,
        (uint16_t)length
    );
}

static unsigned char payload_store(
    unsigned char slot,
    unsigned int offset,
    const unsigned char* data,
    unsigned int length
)
{
    if (length == 0u) {
        return 1u;
    }

    return s64_reu_store(
        S64_REU_HISTORY_BANK,
        (uint16_t)payload_address(slot, offset),
        data,
        (uint16_t)length
    );
}

static void update_dirty(const S64History* history, unsigned char* dirty)
{
    if (dirty == 0) {
        return;
    }

    *dirty = !history->saved_valid
        || history->cursor != history->saved_cursor;
}

static void discard_redo(S64History* history)
{
    if (history->cursor >= history->count) {
        return;
    }

    if (history->saved_valid && history->saved_cursor > history->cursor) {
        history->saved_valid = 0u;
    }
    history->count = history->cursor;
}

static unsigned char slot_in_use(
    const S64History* history,
    unsigned char slot
)
{
    unsigned char i;

    for (i = 0u; i < history->count; ++i) {
        if (history->entries[i].slot == slot) {
            return 1u;
        }
    }
    return 0u;
}

static unsigned char allocate_slot(S64History* history)
{
    unsigned char i;
    unsigned char slot;

    discard_redo(history);

    if (history->count >= S64_HISTORY_LEVELS) {
        slot = history->entries[0].slot;
        for (i = 1u; i < history->count; ++i) {
            history->entries[i - 1u] = history->entries[i];
        }
        --history->count;
        if (history->cursor > 0u) {
            --history->cursor;
        }

        if (history->saved_valid) {
            if (history->saved_cursor == 0u) {
                history->saved_valid = 0u;
            } else {
                --history->saved_cursor;
            }
        }
        return slot;
    }

    for (slot = 0u; slot < S64_HISTORY_LEVELS; ++slot) {
        if (!slot_in_use(history, slot)) {
            return slot;
        }
    }

    return S64_HISTORY_SLOT_NONE;
}

static int capture_payload(
    S64HistoryEntry* entry,
    const S64Buffer* buffer
)
{
    unsigned int offset = 0u;

    while (offset < entry->stored_length) {
        unsigned int count = entry->stored_length - offset;

        if (count > S64_HISTORY_TRANSFER_SIZE) {
            count = S64_HISTORY_TRANSFER_SIZE;
        }
        if (s64_buffer_read(
                buffer,
                entry->position + offset,
                history_current,
                count
            ) != count
            || !payload_store(
                entry->slot,
                offset,
                history_current,
                count
            )) {
            return 0;
        }
        offset += count;
    }

    return 1;
}

static int swap_range(S64HistoryEntry* entry, S64Buffer* buffer)
{
    unsigned int offset;
    unsigned int common;
    unsigned int old_current_length;
    unsigned int length = s64_buffer_length(buffer);
    unsigned long final_length;

    if (entry->position > length
        || entry->current_length > length - entry->position
        || entry->stored_length > S64_HISTORY_CAPACITY
        || entry->current_length > S64_HISTORY_CAPACITY) {
        return 0;
    }

    final_length = (unsigned long)length
        - (unsigned long)entry->current_length
        + (unsigned long)entry->stored_length;
    if (final_length > (unsigned long)S64_BUFFER_CAPACITY
        || !s64_buffer_move_cursor(buffer, entry->position)) {
        return 0;
    }

    common = entry->current_length < entry->stored_length
        ? entry->current_length
        : entry->stored_length;

    offset = 0u;
    while (offset < common) {
        unsigned int i;
        unsigned int count = common - offset;

        if (count > S64_HISTORY_TRANSFER_SIZE) {
            count = S64_HISTORY_TRANSFER_SIZE;
        }

        if (s64_buffer_read(
                buffer,
                entry->position + offset,
                history_current,
                count
            ) != count
            || !payload_load(
                entry->slot,
                offset,
                history_target,
                count
            )) {
            return 0;
        }

        for (i = 0u; i < count; ++i) {
            if (!s64_buffer_delete(buffer)
                || !s64_buffer_insert(buffer, history_target[i])) {
                return 0;
            }
        }

        if (!payload_store(
                entry->slot,
                offset,
                history_current,
                count
            )) {
            return 0;
        }
        offset += count;
    }

    if (entry->stored_length > entry->current_length) {
        offset = entry->current_length;
        while (offset < entry->stored_length) {
            unsigned int i;
            unsigned int count = entry->stored_length - offset;

            if (count > S64_HISTORY_TRANSFER_SIZE) {
                count = S64_HISTORY_TRANSFER_SIZE;
            }
            if (!payload_load(
                    entry->slot,
                    offset,
                    history_target,
                    count
                )) {
                return 0;
            }

            for (i = 0u; i < count; ++i) {
                if (!s64_buffer_insert(buffer, history_target[i])) {
                    return 0;
                }
            }
            offset += count;
        }
    } else if (entry->current_length > entry->stored_length) {
        offset = entry->stored_length;
        while (offset < entry->current_length) {
            unsigned int count = entry->current_length - offset;

            if (count > S64_HISTORY_TRANSFER_SIZE) {
                count = S64_HISTORY_TRANSFER_SIZE;
            }
            if (s64_buffer_read(
                    buffer,
                    entry->position + offset,
                    history_current,
                    count
                ) != count
                || !payload_store(
                    entry->slot,
                    offset,
                    history_current,
                    count
                )) {
                return 0;
            }
            offset += count;
        }

        if (!s64_buffer_delete_range(
                buffer,
                entry->position + entry->stored_length,
                entry->position + entry->current_length
            )) {
            return 0;
        }
    }

    old_current_length = entry->current_length;
    entry->current_length = entry->stored_length;
    entry->stored_length = old_current_length;
    entry->coalesce_insert = 0u;
    return 1;
}

void s64_history_init(S64History* history)
{
    s64_history_reset(history);
}

void s64_history_reset(S64History* history)
{
    history->count = 0u;
    history->cursor = 0u;
    history->pending_valid = 0u;
    history->saved_valid = 0u;
    history->saved_cursor = 0u;
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
    S64HistoryEntry* entry;
    unsigned int length;
    unsigned char slot;

    if (history == 0 || buffer == 0 || before == 0) {
        return 0;
    }

    length = s64_buffer_length(buffer);
    if (start > end || end > length
        || end - start > S64_HISTORY_CAPACITY) {
        s64_history_reset(history);
        return 0;
    }

    if (!dirty) {
        history->saved_valid = 1u;
        history->saved_cursor = history->cursor;
    }

    history->pending_valid = 0u;
    slot = allocate_slot(history);
    if (slot == S64_HISTORY_SLOT_NONE) {
        s64_history_reset(history);
        return 0;
    }

    entry = &history->entries[history->count];
    entry->position = start;
    entry->stored_length = end - start;
    entry->current_length = 0u;
    entry->before = *before;
    entry->after = *before;
    entry->slot = slot;
    entry->coalesce_insert = 0u;

    if (!capture_payload(entry, buffer)) {
        s64_history_reset(history);
        return 0;
    }

    history->pending_valid = 1u;
    return 1;
}

void s64_history_commit(
    S64History* history,
    unsigned int current_length,
    const S64HistoryState* after,
    unsigned char coalesce_insert
)
{
    S64HistoryEntry* entry;

    if (history == 0 || after == 0 || !history->pending_valid
        || current_length > S64_HISTORY_CAPACITY) {
        if (history != 0 && current_length > S64_HISTORY_CAPACITY) {
            s64_history_reset(history);
        }
        return;
    }

    entry = &history->entries[history->count];
    entry->current_length = current_length;
    entry->after = *after;
    entry->coalesce_insert = coalesce_insert ? 1u : 0u;
    ++history->count;
    history->cursor = history->count;
    history->pending_valid = 0u;
}

unsigned char s64_history_can_extend_insert(
    const S64History* history,
    unsigned int cursor
)
{
    const S64HistoryEntry* entry;

    if (history == 0 || history->pending_valid
        || history->cursor == 0u
        || history->cursor != history->count) {
        return 0u;
    }

    entry = &history->entries[history->cursor - 1u];
    return entry->coalesce_insert
        && entry->stored_length == 0u
        && entry->position + entry->current_length == cursor
        && entry->current_length < S64_HISTORY_CAPACITY;
}

void s64_history_extend_insert(
    S64History* history,
    const S64HistoryState* after
)
{
    S64HistoryEntry* entry;

    if (history == 0 || after == 0
        || history->cursor == 0u
        || history->cursor != history->count) {
        return;
    }

    entry = &history->entries[history->cursor - 1u];
    if (entry->current_length >= S64_HISTORY_CAPACITY) {
        return;
    }

    ++entry->current_length;
    entry->after = *after;
}

void s64_history_break_coalesce(S64History* history)
{
    if (history != 0 && history->cursor > 0u) {
        history->entries[history->cursor - 1u].coalesce_insert = 0u;
    }
}

void s64_history_mark_saved(S64History* history)
{
    if (history == 0) {
        return;
    }

    history->saved_valid = 1u;
    history->saved_cursor = history->cursor;
    s64_history_break_coalesce(history);
}

int s64_history_undo(
    S64History* history,
    S64Buffer* buffer,
    S64HistoryState* state,
    unsigned char* dirty
)
{
    S64HistoryEntry* entry;

    if (history == 0 || buffer == 0 || state == 0
        || history->pending_valid || history->cursor == 0u) {
        return 0;
    }

    entry = &history->entries[history->cursor - 1u];
    if (!swap_range(entry, buffer)) {
        return 0;
    }

    --history->cursor;
    *state = entry->before;
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
    S64HistoryEntry* entry;

    if (history == 0 || buffer == 0 || state == 0
        || history->pending_valid || history->cursor >= history->count) {
        return 0;
    }

    entry = &history->entries[history->cursor];
    if (!swap_range(entry, buffer)) {
        return 0;
    }

    ++history->cursor;
    *state = entry->after;
    update_dirty(history, dirty);
    return 1;
}
