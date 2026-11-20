#ifndef SEE64YOU_UI_H
#define SEE64YOU_UI_H

#include "editor.h"
#include "document.h"

#define S64_UI_EDIT_CHARACTER 0x80u

void s64_ui_init(const S64Document* document);
S64DocumentKind s64_ui_select_document_kind(void);
int s64_ui_parse_line_number(const char* text, unsigned int* line_number);
void s64_ui_set_http_ready(unsigned char ready);
void s64_ui_set_dos_ready(unsigned char ready);
void s64_ui_render(const S64Editor* editor);
unsigned char s64_ui_render_append(const S64Editor* editor, unsigned char value);
void s64_ui_render_edit(const S64Editor* editor, unsigned char first_body_row);
void s64_ui_render_cursor(const S64Editor* editor);
void s64_ui_message(const char* message);
void s64_ui_begin_overlay(void);
void s64_ui_show_help(void);
unsigned char s64_ui_prompt_text(const char* label, char* value, unsigned char capacity);
unsigned char s64_ui_prompt_yes_no(const char* label);
unsigned char s64_ui_prompt_exit(void);

#endif
