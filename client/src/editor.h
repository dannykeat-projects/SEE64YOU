#ifndef SEE64YOU_EDITOR_H
#define SEE64YOU_EDITOR_H

#include "buffer.h"
#include "clipboard.h"

typedef struct {
    S64Buffer buffer;
    unsigned int view_start;
    unsigned int view_column;
    unsigned int line;
    unsigned int column;
    unsigned int selection_anchor;
    unsigned int preferred_column;
    unsigned char selection_valid;
    unsigned char preferred_column_valid;
    unsigned char mark_mode;
    unsigned char dirty;
    unsigned char insert_mode;
    unsigned char word_wrap;
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
void s64_editor_home(S64Editor* editor);
void s64_editor_end(S64Editor* editor);
void s64_editor_page_up(S64Editor* editor);
void s64_editor_page_down(S64Editor* editor);
void s64_editor_word_left(S64Editor* editor);
void s64_editor_word_right(S64Editor* editor);
int s64_editor_go_to_line(S64Editor* editor, unsigned int line_number);
int s64_editor_find_next(S64Editor* editor, const char* query, unsigned char wrap);
int s64_editor_find_previous(S64Editor* editor, const char* query, unsigned char wrap);
int s64_editor_replace_selection(S64Editor* editor, const char* replacement);
int s64_editor_replace_all(
    S64Editor* editor,
    const char* query,
    const char* replacement,
    unsigned int* replacements
);
void s64_editor_toggle_insert(S64Editor* editor);
void s64_editor_toggle_word_wrap(S64Editor* editor);

void s64_editor_toggle_mark(S64Editor* editor);
void s64_editor_select_all(S64Editor* editor);
void s64_editor_clear_selection(S64Editor* editor);
unsigned char s64_editor_has_selection(const S64Editor* editor);
unsigned int s64_editor_selection_start(const S64Editor* editor);
unsigned int s64_editor_selection_end(const S64Editor* editor);
int s64_editor_copy(const S64Editor* editor, S64Clipboard* clipboard);
int s64_editor_cut(S64Editor* editor, S64Clipboard* clipboard);
int s64_editor_paste(S64Editor* editor, const S64Clipboard* clipboard);

void s64_editor_recalculate_position(S64Editor* editor);
void s64_editor_ensure_cursor_visible(S64Editor* editor);

#endif
