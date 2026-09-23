#include <conio.h>
#include "editor.h"
#include "ui.h"

#define KEY_RUNSTOP     0x03u
#define KEY_ENTER       0x0du
#define KEY_DELETE      0x14u
#define KEY_CURSOR_DOWN 0x11u
#define KEY_CURSOR_RIGHT 0x1du
#define KEY_CURSOR_UP   0x91u
#define KEY_CURSOR_LEFT 0x9du
#define KEY_F8          0x8cu

static S64Editor editor;

static unsigned char is_printable(unsigned char key)
{
    return key >= 0x20u && key <= 0x7eu;
}

int main(void)
{
    unsigned char key;
    unsigned char running = 1u;

    s64_editor_init(&editor);
    s64_ui_init();
    s64_ui_render(&editor);

    while (running) {
        key = cgetc();

        switch (key) {
            case KEY_CURSOR_LEFT:
                s64_editor_left(&editor);
                break;

            case KEY_CURSOR_RIGHT:
                s64_editor_right(&editor);
                break;

            case KEY_CURSOR_UP:
                s64_editor_up(&editor);
                break;

            case KEY_CURSOR_DOWN:
                s64_editor_down(&editor);
                break;

            case KEY_DELETE:
                s64_editor_backspace(&editor);
                break;

            case KEY_ENTER:
                s64_editor_newline(&editor);
                break;

            case KEY_F8:
                running = 0u;
                continue;

            case KEY_RUNSTOP:
                s64_ui_message("RUN/STOP ignored - F8 exits");
                continue;

            default:
                if (is_printable(key)) {
                    if (!s64_editor_insert(&editor, key)) {
                        s64_ui_message("Document buffer full");
                        continue;
                    }
                } else {
                    continue;
                }
                break;
        }

        s64_ui_render(&editor);
    }

    clrscr();
    cputs("SEE64YOU EXITED\r\n");
    return 0;
}
