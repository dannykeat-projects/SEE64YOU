#include "history.h"
#include "platform/reu.h"

#ifndef S64_REU_BUFFER
#error "history_reu.c requires S64_REU_BUFFER"
#endif

#define S64_REU_HISTORY_PAYLOAD_BANK 0u
#define S64_REU_HISTORY_PAYLOAD_BASE 61440u
#define S64_REU_HISTORY_META_BANK 1u
#define S64_REU_HISTORY_META_BASE 61440u
#define S64_HISTORY_TRANSFER_SIZE 64u
#define S64_HISTORY_LEVEL_MASK 0x0fu

typedef struct {
    unsigned int position;
    unsigned int stored_length;
    unsigned int current_length;
    S64HistoryState before;
    S64HistoryState after;
    unsigned char coalesce_insert;
} S64HistoryEntry;

static unsigned char history_target[S64_HISTORY_TRANSFER_SIZE];
static unsigned char history_current[S64_HISTORY_TRANSFER_SIZE];
static S64HistoryEntry history_entry;
static unsigned char history_entry_slot;
static unsigned char history_entry_valid;
static unsigned char history_entry_dirty;

static unsigned char physical_slot(
    const S64History* history,
    unsigned char logical_index
)
{
    return (unsigned char)(
        (history->start + logical_index) & S64_HISTORY_LEVEL_MASK
    );
}

static unsigned int payload_address(
    unsigned char slot,
    unsigned int offset
)
{
    return (unsigned int)(
        S64_REU_HISTORY_PAYLOAD_BASE
        + ((unsigned int)slot * S64_HISTORY_CAPACITY)
        + offset
    );
}

static unsigned int metadata_address(unsigned char slot)
{
    return (unsigned int)(
        S64_REU_HISTORY_META_BASE
        + ((unsigned int)slot * (unsigned int)sizeof(S64HistoryEntry))
    );
}

static unsigned char metadata_load(
    unsigned char slot,
    S64HistoryEntry* entry
)
{
    return s64_reu_load(
        S64_REU_HISTORY_META_BANK,
        (uint16_t)metadata_address(slot),
        entry,
        (uint16_t)sizeof(S64HistoryEntry)
    );
}

static unsigned char metadata_store(
    unsigned char slot,
    const S64HistoryEntry* entry
)
{
    return s64_reu_store(
        S64_REU_HISTORY_META_BANK,
        (uint16_t)metadata_address(slot),
        entry,
        (uint16_t)sizeof(S64HistoryEntry)
    );
}

static unsigned char flush_entry(void)
{
    if (!history_entry_valid || !history_entry_dirty) {
        return 1u;
    }

    if (!metadata_store(history_entry_slot, &history_entry)) {
        return 0u;
    }

    history_entry_dirty = 0u;
    return 1u;
}

static void discard_entry_cache(void)
{
    history_entry_valid = 0u;
    history_entry_dirty = 0u;
}

static S64HistoryEntry* load_entry(
    const S64History* history,
    unsigned char logical_index
)
{
    unsigned char slot;

    if (history == 0 || logical_index >= history->count) {
        return 0;
    }

    slot = physical_slot(history, logical_index);
    if (history_entry_valid && history_entry_slot == slot) {
        return &history_entry;
    }

    if (!flush_entry() || !metadata_load(slot, &history_entry)) {
        discard_entry_cache();
        return 0;
    }

    history_entry_slot = slot;
    history_entry_valid = 1u;
    history_entry_dirty = 0u;
    return &history_entry;
}

static S64HistoryEntry* create_entry(
    const S64History* history,
    unsigned char logical_index
)
{
    if (!flush_entry()) {
        discard_entry_cache();
        return 0;
    }

    history_entry_slot = physical_slot(history, logical_index);
    history_entry_valid = 1u;
    history_entry_dirty = 1u;
    return &history_entry;
}

static void mark_entry_dirty(void)
{
    if (history_entry_valid) {
        history_entry_dirty = 1u;
    }
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
        S64_REU_HISTORY_PAYLOAD_BANK,
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
        S64_REU_HISTORY_PAYLOAD_BANK,
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

static unsigned char prepare_capture(S64History* history)
{
    if (!flush_entry()) {
        return 0u;
    }

    if (history->cursor < history->count) {
        if (history->saved_valid
            && history->saved_cursor > history->cursor) {
            history->saved_valid = 0u;
        }
        history->count = history->cursor;
    }

    if (history->count >= S64_HISTORY_LEVELS) {
        history->start = (unsigned char)(
            (history->start + 1u) & S64_HISTORY_LEVEL_MASK
        );
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
    }

    discard_entry_cache();
    return 1u;
}

static int capture_payload(
    const S64HistoryEntry* entry,
    unsigned char slot,
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
                slot,
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

static int swap_range(
    S64HistoryEntry* entry,
    unsigned char slot,
    S64Buffer* buffer
)
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
                slot,
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
                slot,
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
                    slot,
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
                    slot,
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
    mark_entry_dirty();
    return 1;
}

void s64_history_init(S64History* history)
{
    s64_history_reset(history);
}

void s64_history_reset(S64History* history)
{
    if (history == 0) {
        return;
    }

    history->start = 0u;
    history->count = 0u;
    history->cursor = 0u;
    history->pending_valid = 0u;
    history->saved_valid = 0u;
    history->saved_cursor = 0u;
    discard_entry_cache();
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
    if (!prepare_capture(history)) {
        s64_history_reset(history);
        return 0;
    }

    slot = physical_slot(history, history->count);
    entry = create_entry(history, history->count);
    if (entry == 0) {
        s64_history_reset(history);
        return 0;
    }

    entry->position = start;
    entry->stored_length = end - start;
    entry->current_length = 0u;
    entry->before = *before;
    entry->after = *before;
    entry->coalesce_insert = 0u;

    if (!capture_payload(entry, slot, buffer)) {
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
    unsigned char slot;

    if (history == 0 || after == 0 || !history->pending_valid
        || current_length > S64_HISTORY_CAPACITY) {
        if (history != 0 && current_length > S64_HISTORY_CAPACITY) {
            s64_history_reset(history);
        }
        return;
    }

    slot = physical_slot(history, history->count);
    if (!history_entry_valid || history_entry_slot != slot) {
        s64_history_reset(history);
        return;
    }

    history_entry.current_length = current_length;
    history_entry.after = *after;
    history_entry.coalesce_insert = coalesce_insert ? 1u : 0u;
    mark_entry_dirty();

    ++history->count;
    history->cursor = history->count;
    history->pending_valid = 0u;
}

unsigned char s64_history_can_extend_insert(
    const S64History* history,
    unsigned int cursor
)
{
    S64HistoryEntry* entry;

    if (history == 0 || history->pending_valid
        || history->cursor == 0u
        || history->cursor != history->count) {
        return 0u;
    }

    entry = load_entry(history, (unsigned char)(history->cursor - 1u));
    if (entry == 0) {
        return 0u;
    }

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

    entry = load_entry(history, (unsigned char)(history->cursor - 1u));
    if (entry == 0 || entry->current_length >= S64_HISTORY_CAPACITY) {
        if (entry == 0) {
            s64_history_reset(history);
        }
        return;
    }

    ++entry->current_length;
    entry->after = *after;
    mark_entry_dirty();
}

void s64_history_break_coalesce(S64History* history)
{
    S64HistoryEntry* entry;

    if (history == 0 || history->cursor == 0u) {
        return;
    }

    entry = load_entry(history, (unsigned char)(history->cursor - 1u));
    if (entry == 0) {
        s64_history_reset(history);
        return;
    }

    entry->coalesce_insert = 0u;
    mark_entry_dirty();
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
    unsigned char logical_index;
    unsigned char slot;

    if (history == 0 || buffer == 0 || state == 0
        || history->pending_valid || history->cursor == 0u) {
        return 0;
    }

    logical_index = (unsigned char)(history->cursor - 1u);
    slot = physical_slot(history, logical_index);
    entry = load_entry(history, logical_index);
    if (entry == 0 || !swap_range(entry, slot, buffer)) {
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
    unsigned char logical_index;
    unsigned char slot;

    if (history == 0 || buffer == 0 || state == 0
        || history->pending_valid || history->cursor >= history->count) {
        return 0;
    }

    logical_index = history->cursor;
    slot = physical_slot(history, logical_index);
    entry = load_entry(history, logical_index);
    if (entry == 0 || !swap_range(entry, slot, buffer)) {
        return 0;
    }

    ++history->cursor;
    *state = entry->after;
    update_dirty(history, dirty);
    return 1;
}
