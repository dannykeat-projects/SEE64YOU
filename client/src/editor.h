#ifndef SEE64YOU_EDITOR_H
#define SEE64YOU_EDITOR_H

#include "buffer.h"

typedef struct {
    S64Buffer buffer;
    unsigned int view_start;
    unsigned int line;
    unsigned int column;
    unsigned char dirty;
    unsigned char insert_mode;
} S64Editor;

void s64_editor_init(S64Editor* editor);
int s64_editor_insert(S64Editor* editor, unsigned char value);
int s64_editor_newline(S64Editor* editor);
int s64_editor_backspace(S64Editor* editor);
int s64_editor_delete(S64Editor* editor);
void s64_editor_left(S64Editor* editor);
void s64_editor_right(S64Editor* editor);
void s64_editor_up(S64Editor* editor);
void s64_editor_down(S64Editor* editor);
void s64_editor_recalculate_position(S64Editor* editor);
void s64_editor_ensure_cursor_visible(S64Editor* editor);

#endif
