#ifndef SEE64YOU_UI_H
#define SEE64YOU_UI_H

#include "editor.h"

void s64_ui_init(void);
void s64_ui_set_http_ready(unsigned char ready);
void s64_ui_render(const S64Editor* editor);
void s64_ui_message(const char* message);

#endif
