#ifndef SEE64YOU_EDITOR_INTERNAL_H
#define SEE64YOU_EDITOR_INTERNAL_H

#include "editor.h"

unsigned int s64_editor_internal_line_start(
    const S64Buffer* buffer,
    unsigned int position
);
void s64_editor_internal_reset_preferred_column(S64Editor* editor);
void s64_editor_internal_history_state(
    const S64Editor* editor,
    S64HistoryState* state
);
unsigned char s64_editor_internal_history_begin(
    S64Editor* editor,
    unsigned int start,
    unsigned int end
);
void s64_editor_internal_history_commit(
    S64Editor* editor,
    unsigned int current_length,
    unsigned char coalesce_insert,
    unsigned char captured
);
void s64_editor_internal_restore_history_state(
    S64Editor* editor,
    const S64HistoryState* state
);

#endif
