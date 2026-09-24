#include <conio.h>
#include <stdio.h>
#include <string.h>

#include "clipboard.h"
#include "document.h"
#include "editor.h"
#include "input/input.h"
#include "platform/dos.h"
#include "platform/uci.h"
#include "storage/local.h"
#include "ui.h"

#define SEARCH_TEXT_CAPACITY 33u
#define LINE_TEXT_CAPACITY 6u

static S64Editor editor;
static S64Clipboard clipboard;
static S64Document document;
static S64Dos dos;
static S64LocalDocument local_document;
static S64UciCapabilities uci_capabilities;
static char find_query[SEARCH_TEXT_CAPACITY];
static unsigned char find_query_valid;

static unsigned char local_storage_ready(void)
{
    if (!uci_capabilities.dos_target) {
        s64_ui_message("Ultimate DOS unavailable");
        return 0u;
    }
    return 1u;
}

static unsigned char allow_discard(void)
{
    return !editor.dirty || s64_ui_prompt_yes_no("Discard changes? Y/N");
}

static void show_local_result(S64LocalResult result)
{
    s64_ui_message(s64_local_result_message(&local_document, result));
}

static void new_document(void)
{
    S64LocalResult result;

    if (!allow_discard()) {
        s64_ui_render(&editor);
        return;
    }

    result = s64_local_new(&local_document, 1u);
    if (result != S64_LOCAL_OK) {
        show_local_result(result);
        return;
    }

    s64_editor_document_loaded(&editor);
    s64_ui_render(&editor);
    s64_ui_message("New document");
}

static void open_document(void)
{
    char filename[S64_LOCAL_FILENAME_CAPACITY];
    S64LocalResult result;

    if (!local_storage_ready()) return;
    if (!allow_discard()) {
        s64_ui_render(&editor);
        return;
    }
    if (!s64_ui_prompt_text("Open: ", filename, sizeof(filename))) {
        s64_ui_render(&editor);
        return;
    }

    result = s64_local_open(&local_document, filename, 1u);
    if (result != S64_LOCAL_OK) {
        show_local_result(result);
        return;
    }

    s64_editor_document_loaded(&editor);
    s64_ui_render(&editor);
    s64_ui_message("File opened");
}

static void save_as_document(void)
{
    char filename[S64_LOCAL_FILENAME_CAPACITY];
    S64LocalResult result;

    if (!local_storage_ready()) return;
    if (!s64_ui_prompt_text("Save as: ", filename, sizeof(filename))) {
        s64_ui_render(&editor);
        return;
    }

    result = s64_local_save_as(&local_document, filename, 0u);
    if (result == S64_LOCAL_EXISTS) {
        if (!s64_ui_prompt_yes_no("Overwrite file? Y/N")) {
            s64_ui_render(&editor);
            return;
        }
        result = s64_local_save_as(&local_document, filename, 1u);
    }

    if (result != S64_LOCAL_OK) {
        show_local_result(result);
        return;
    }

    editor.dirty = 0u;
    s64_ui_render(&editor);
    s64_ui_message("File saved");
}

static void save_document(void)
{
    S64LocalResult result;

    if (!local_storage_ready()) return;
    if (!s64_local_has_filename(&local_document)) {
        save_as_document();
        return;
    }

    result = s64_local_save(&local_document);
    if (result != S64_LOCAL_OK) {
        show_local_result(result);
        return;
    }

    editor.dirty = 0u;
    s64_ui_render(&editor);
    s64_ui_message("File saved");
}

static void handle_clipboard_result(unsigned char ok, const char* failure)
{
    if (!ok) {
        s64_ui_message(failure);
    } else {
        s64_ui_render(&editor);
    }
}

static void show_find_result(unsigned char previous)
{
    unsigned char found;

    if (!find_query_valid) {
        return;
    }

    found = previous
        ? (unsigned char)s64_editor_find_previous(&editor, find_query, 1u)
        : (unsigned char)s64_editor_find_next(&editor, find_query, 1u);

    if (found) {
        s64_ui_render(&editor);
    } else {
        s64_ui_message("Text not found");
    }
}

static void prompt_find(unsigned char previous)
{
    char value[SEARCH_TEXT_CAPACITY];

    if (!s64_ui_prompt_text("Find: ", value, SEARCH_TEXT_CAPACITY)) {
        s64_ui_render(&editor);
        return;
    }

    if (value[0] == '\0') {
        s64_ui_message("Find text is empty");
        return;
    }

    strcpy(find_query, value);
    find_query_valid = 1u;
    show_find_result(previous);
}

static void repeat_find(unsigned char previous)
{
    if (!find_query_valid) {
        prompt_find(previous);
        return;
    }

    show_find_result(previous);
}

static int parse_line_number(const char* text, unsigned int* line_number)
{
    unsigned int value = 0u;
    unsigned char digit;

    if (*text == '\0') {
        return 0;
    }

    while (*text != '\0') {
        if (*text < '0' || *text > '9') {
            return 0;
        }

        digit = (unsigned char)(*text - '0');
        if (value > 819u) {
            return 0;
        }

        value = (unsigned int)(value * 10u + digit);
        if (value > S64_BUFFER_CAPACITY + 1u) {
            return 0;
        }
        ++text;
    }

    if (value == 0u) {
        return 0;
    }

    *line_number = value;
    return 1;
}

static void prompt_go_to_line(void)
{
    char value[LINE_TEXT_CAPACITY];
    unsigned int line_number;

    if (!s64_ui_prompt_text("Go line: ", value, LINE_TEXT_CAPACITY)) {
        s64_ui_render(&editor);
        return;
    }

    if (!parse_line_number(value, &line_number)) {
        s64_ui_message("Invalid line number");
        return;
    }

    if (!s64_editor_go_to_line(&editor, line_number)) {
        s64_ui_message("Line does not exist");
        return;
    }

    s64_ui_render(&editor);
}

static void prompt_replace(void)
{
    char query[SEARCH_TEXT_CAPACITY];
    char replacement[SEARCH_TEXT_CAPACITY];
    char choice[2];
    char message[40];
    unsigned int replaced = 0u;

    if (!s64_ui_prompt_text("Find: ", query, SEARCH_TEXT_CAPACITY)) {
        s64_ui_render(&editor);
        return;
    }

    if (query[0] == '\0') {
        s64_ui_message("Find text is empty");
        return;
    }

    if (!s64_ui_prompt_text("With: ", replacement, SEARCH_TEXT_CAPACITY)) {
        s64_ui_render(&editor);
        return;
    }

    if (!s64_ui_prompt_text("R=one A=all: ", choice, sizeof(choice))) {
        s64_ui_render(&editor);
        return;
    }

    strcpy(find_query, query);
    find_query_valid = 1u;

    if (choice[0] == 'R' || choice[0] == 'r') {
        if (!s64_editor_find_next(&editor, query, 1u)) {
            s64_ui_message("Text not found");
            return;
        }

        if (!s64_editor_replace_selection(&editor, replacement)) {
            s64_ui_message("Replacement too large");
            return;
        }

        s64_ui_render(&editor);
        s64_ui_message("Replaced one match");
        return;
    }

    if (choice[0] == 'A' || choice[0] == 'a') {
        if (!s64_editor_replace_all(&editor, query, replacement, &replaced)) {
            s64_ui_message("Replacement too large");
            return;
        }

        if (replaced == 0u) {
            s64_ui_message("Text not found");
            return;
        }

        s64_ui_render(&editor);
        sprintf(message, "Replaced %u match%s", replaced, replaced == 1u ? "" : "es");
        s64_ui_message(message);
        return;
    }

    s64_ui_message("Choose R or A");
}

int main(void)
{
    S64InputEvent input;
    unsigned char running = 1u;

    s64_editor_init(&editor);
    s64_clipboard_init(&clipboard);
    s64_document_init(&document, &editor.buffer);
    s64_dos_init(&dos, s64_uci_execute, S64_DOS_TARGET_PRIMARY);
    s64_local_init(&local_document, &dos, &document);
    s64_ui_init();
    s64_uci_probe_capabilities(&uci_capabilities);
    s64_ui_set_http_ready(uci_capabilities.http_target);
    s64_ui_set_dos_ready(uci_capabilities.dos_target);
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
            case S64_INPUT_TOGGLE_WRAP:
                s64_editor_toggle_word_wrap(&editor);
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

            case S64_INPUT_NEW:
                new_document();
                continue;
            case S64_INPUT_OPEN:
                open_document();
                continue;
            case S64_INPUT_SAVE:
                save_document();
                continue;
            case S64_INPUT_SAVE_AS:
                save_as_document();
                continue;
            case S64_INPUT_FIND:
                prompt_find(0u);
                continue;
            case S64_INPUT_FIND_NEXT:
                repeat_find(0u);
                continue;
            case S64_INPUT_FIND_PREVIOUS:
                repeat_find(1u);
                continue;
            case S64_INPUT_GOTO_LINE:
                prompt_go_to_line();
                continue;
            case S64_INPUT_REPLACE:
                prompt_replace();
                continue;
            case S64_INPUT_UNDO:
                s64_ui_message("Undo: history layer next");
                continue;
            case S64_INPUT_REDO:
                s64_ui_message("Redo: history layer next");
                continue;

            case S64_INPUT_EXIT:
                if (editor.dirty && !s64_ui_prompt_yes_no("Discard and exit? Y/N")) {
                    s64_ui_render(&editor);
                    continue;
                }
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
