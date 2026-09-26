#ifndef SEE64YOU_UI_H
#define SEE64YOU_UI_H

#include "editor.h"

void s64_ui_init(void);
void s64_ui_set_http_ready(unsigned char ready);
void s64_ui_set_dos_ready(unsigned char ready);
void s64_ui_render(const S64Editor* editor);
void s64_ui_render_append(const S64Editor* editor, unsigned char value);
void s64_ui_render_edit(const S64Editor* editor, unsigned char first_body_row);
void s64_ui_render_char_edit(const S64Editor* editor);
void s64_ui_render_backspace(const S64Editor* editor);
void s64_ui_render_cursor(const S64Editor* editor);
void s64_ui_message(const char* message);
unsigned char s64_ui_prompt_text(const char* label, char* value, unsigned char capacity);
unsigned char s64_ui_prompt_yes_no(const char* label);

#endif
