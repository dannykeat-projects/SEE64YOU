#ifndef SEE64YOU_HISTORY_H
#define SEE64YOU_HISTORY_H

#include "buffer.h"

#define S64_HISTORY_CAPACITY 256u

typedef struct {
    unsigned int cursor;
    unsigned int selection_anchor;
    unsigned char selection_valid;
    unsigned char mark_mode;
} S64HistoryState;

#ifdef S64_REU_BUFFER
#define S64_HISTORY_LEVELS 16u

typedef struct {
    unsigned int position;
    unsigned int stored_length;
    unsigned int current_length;
    S64HistoryState before;
    S64HistoryState after;
    unsigned char slot;
    unsigned char coalesce_insert;
} S64HistoryEntry;

typedef struct {
    S64HistoryEntry entries[S64_HISTORY_LEVELS];
    unsigned char count;
    unsigned char cursor;
    unsigned char pending_valid;
    unsigned char saved_valid;
    unsigned char saved_cursor;
} S64History;
#else
typedef struct {
    unsigned char data[S64_HISTORY_CAPACITY];
    unsigned int position;
    unsigned int stored_length;
    unsigned int current_length;
    S64HistoryState before;
    S64HistoryState after;
    unsigned char valid;
    unsigned char applied;
    unsigned char saved_side;
    unsigned char coalesce_insert;
} S64History;
#endif

void s64_history_init(S64History* history);
void s64_history_reset(S64History* history);
int s64_history_capture(
    S64History* history,
    const S64Buffer* buffer,
    unsigned int start,
    unsigned int end,
    const S64HistoryState* before,
    unsigned char dirty
);
void s64_history_commit(
    S64History* history,
    unsigned int current_length,
    const S64HistoryState* after,
    unsigned char coalesce_insert
);
unsigned char s64_history_can_extend_insert(
    const S64History* history,
    unsigned int cursor
);
void s64_history_extend_insert(
    S64History* history,
    const S64HistoryState* after
);
void s64_history_break_coalesce(S64History* history);
void s64_history_mark_saved(S64History* history);
int s64_history_undo(
    S64History* history,
    S64Buffer* buffer,
    S64HistoryState* state,
    unsigned char* dirty
);
int s64_history_redo(
    S64History* history,
    S64Buffer* buffer,
    S64HistoryState* state,
    unsigned char* dirty
);

#endif
