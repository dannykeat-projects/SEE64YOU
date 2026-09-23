#include <conio.h>

#include "clipboard.h"
#include "editor.h"
#include "input/input.h"
#include "platform/uci.h"
#include "ui.h"

static S64Editor editor;
static S64Clipboard clipboard;
static S64UciCapabilities uci_capabilities;

static void handle_clipboard_result(unsigned char ok, const char* failure)
{
    if (!ok) {
        s64_ui_message(failure);
    } else {
        s64_ui_render(&editor);
    }
}

int main(void)
{
    S64InputEvent input;
    unsigned char running = 1u;

    s64_editor_init(&editor);
    s64_clipboard_init(&clipboard);
    s64_ui_init();
    s64_uci_probe_capabilities(&uci_capabilities);
    s64_ui_set_http_ready(uci_capabilities.http_target);
    s64_ui_render(&editor);

    while (running) {
        s64_input_read(&input);

        switch (input.type) {
            case S64_INPUT_CHARACTER:
                if (!s64_editor_insert(&editor, input.character)) {
                    s64_ui_message("Document buffer full");
                    continue;
                }
                break;

            case S64_INPUT_LEFT:
                s64_editor_left(&editor);
                break;
            case S64_INPUT_RIGHT:
                s64_editor_right(&editor);
                break;
            case S64_INPUT_UP:
                s64_editor_up(&editor);
                break;
            case S64_INPUT_DOWN:
                s64_editor_down(&editor);
                break;
            case S64_INPUT_HOME:
                s64_editor_home(&editor);
                break;
            case S64_INPUT_END:
                s64_editor_end(&editor);
                break;
            case S64_INPUT_PAGE_UP:
                s64_editor_page_up(&editor);
                break;
            case S64_INPUT_PAGE_DOWN:
                s64_editor_page_down(&editor);
                break;
            case S64_INPUT_WORD_LEFT:
                s64_editor_word_left(&editor);
                break;
            case S64_INPUT_WORD_RIGHT:
                s64_editor_word_right(&editor);
                break;

            case S64_INPUT_BACKSPACE:
                s64_editor_backspace(&editor);
                break;
            case S64_INPUT_NEWLINE:
                if (!s64_editor_newline(&editor)) {
                    s64_ui_message("Document buffer full");
                    continue;
                }
                break;
            case S64_INPUT_TOGGLE_INSERT:
                s64_editor_toggle_insert(&editor);
                break;

            case S64_INPUT_TOGGLE_MARK:
                s64_editor_toggle_mark(&editor);
                break;
            case S64_INPUT_SELECT_ALL:
                s64_editor_select_all(&editor);
                break;
            case S64_INPUT_COPY:
                handle_clipboard_result(
                    (unsigned char)s64_editor_copy(&editor, &clipboard),
                    "Nothing selected"
                );
                continue;
            case S64_INPUT_CUT:
                handle_clipboard_result(
                    (unsigned char)s64_editor_cut(&editor, &clipboard),
                    "Cannot cut selection"
                );
                continue;
            case S64_INPUT_PASTE:
                handle_clipboard_result(
                    (unsigned char)s64_editor_paste(&editor, &clipboard),
                    clipboard.length == 0u ? "Clipboard empty" : "Paste too large"
                );
                continue;

            case S64_INPUT_SAVE:
                s64_ui_message("Save: storage layer next");
                continue;
            case S64_INPUT_FIND:
                s64_ui_message("Find: editor command next");
                continue;
            case S64_INPUT_REPLACE:
                s64_ui_message("Replace: editor command next");
                continue;
            case S64_INPUT_UNDO:
                s64_ui_message("Undo: history layer next");
                continue;
            case S64_INPUT_REDO:
                s64_ui_message("Redo: history layer next");
                continue;

            case S64_INPUT_EXIT:
                running = 0u;
                continue;

            case S64_INPUT_NONE:
            default:
                continue;
        }

        s64_ui_render(&editor);
    }

    clrscr();
    cputs("SEE64YOU EXITED\r\n");
    return 0;
}
