#include <conio.h>
#include <string.h>
#include <time.h>

#include "clipboard.h"
#include "document.h"
#include "editor.h"
#include "input/input.h"
#include "menu.h"
#include "platform/dos.h"
#include "platform/uci.h"
#ifdef S64_REU_BUFFER
#include "platform/chain.h"
#include "platform/reu.h"
#include "publisher/editor_handoff.h"
#endif
#include "storage/local.h"
#include "ui.h"

#define SEARCH_TEXT_CAPACITY 33u
#define LINE_TEXT_CAPACITY 6u
#define AUTOSAVE_SECONDS 30UL
#define AUTOSAVE_INTERVAL ((clock_t)(AUTOSAVE_SECONDS * (unsigned long)CLOCKS_PER_SEC))

static S64Editor editor;
static S64Clipboard clipboard;
static S64Document document;
static S64Dos dos;
static S64LocalDocument local_document;
static S64UciCapabilities uci_capabilities;
static char find_query[SEARCH_TEXT_CAPACITY];
static unsigned char find_query_valid;
static clock_t last_recovery_write;
static unsigned char recovery_error_shown;
static char* append_unsigned(char* output, unsigned int value)
{
    char digits[5];
    unsigned char count = 0u;

    do {
        digits[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    } while (value != 0u);

    while (count != 0u) {
        *output++ = digits[--count];
    }
    *output = '\0';
    return output;
}


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

#ifdef S64_REU_BUFFER
static unsigned char restore_publisher_handoff(void)
{
    if (!s64_editor_handoff_restore(&editor, &document, &local_document)) {
        return 0u;
    }

    s64_editor_recalculate_position(&editor);
    s64_editor_ensure_cursor_visible(&editor);
    return 1u;
}

static void launch_publisher(void)
{
    if (!s64_history_prepare_handoff(&editor.history)) {
        s64_ui_message("Publish handoff failed");
        return;
    }

    if (!s64_editor_handoff_store(&editor, &document, &local_document)) {
        s64_ui_render(&editor);
        s64_ui_message("Publish handoff failed");
        return;
    }

    s64_chain_publisher();
}
#endif

static void arm_recovery_autosave(void)
{
    last_recovery_write = (clock_t)(clock() - AUTOSAVE_INTERVAL);
    recovery_error_shown = 0u;
}

static void defer_recovery_autosave(void)
{
    last_recovery_write = clock();
    recovery_error_shown = 0u;
}

static void clear_recovery_snapshot(void)
{
    if (uci_capabilities.dos_target) {
        (void)s64_local_delete_recovery(&local_document);
    }
    arm_recovery_autosave();
}

static void maybe_autosave(void)
{
    clock_t now;
    S64LocalResult result;

    if (!editor.dirty || !uci_capabilities.dos_target) {
        return;
    }

    now = clock();
    if ((clock_t)(now - last_recovery_write) < AUTOSAVE_INTERVAL) {
        return;
    }

    result = s64_local_save_recovery(&local_document);
    last_recovery_write = now;
    if (result == S64_LOCAL_OK) {
        recovery_error_shown = 0u;
    } else if (!recovery_error_shown) {
        recovery_error_shown = 1u;
        s64_ui_message("Autosave unavailable");
    }
}

static void offer_recovery(void)
{
    S64LocalResult result;
    uint8_t exists = 0u;

    if (!uci_capabilities.dos_target) {
        arm_recovery_autosave();
        return;
    }

    result = s64_local_recovery_exists(&local_document, &exists);
    if (result != S64_LOCAL_OK || !exists) {
        arm_recovery_autosave();
        return;
    }

    if (!s64_ui_prompt_yes_no("Recover autosave? Y/N")) {
        (void)s64_local_delete_recovery(&local_document);
        arm_recovery_autosave();
        s64_ui_render(&editor);
        s64_ui_message("Recovery discarded");
        return;
    }

    result = s64_local_open_recovery(&local_document);
    if (result != S64_LOCAL_OK) {
        (void)s64_local_delete_recovery(&local_document);
        arm_recovery_autosave();
        show_local_result(result);
        return;
    }

    s64_editor_document_loaded(&editor);
    editor.dirty = 1u;
    defer_recovery_autosave();
    s64_ui_render(&editor);
    s64_ui_message("Recovered autosave");
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
    clear_recovery_snapshot();
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
        if (s64_local_document_replaced(&local_document)) {
            s64_editor_document_loaded(&editor);
            s64_ui_render(&editor);
        }
        show_local_result(result);
        return;
    }

    s64_editor_document_loaded(&editor);
    clear_recovery_snapshot();
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

    s64_editor_mark_saved(&editor);
    clear_recovery_snapshot();
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

    s64_editor_mark_saved(&editor);
    clear_recovery_snapshot();
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
#ifdef S64_REU_BUFFER
        if (value > 6553u || (value == 6553u && digit > 5u)) {
            return 0;
        }
#else
        if (value > 819u) {
            return 0;
        }
#endif

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
        strcpy(message, "Replaced ");
        strcat(append_unsigned(message + 9u, replaced), replaced == 1u ? " match" : " matches");
        s64_ui_message(message);
        return;
    }

    s64_ui_message("Choose R or A");
}

static S64InputType run_menu(unsigned char initial_group)
{
    S64InputEvent menu_input;
    unsigned char group = initial_group < S64_MENU_GROUPS ? initial_group : 0u;
    unsigned char item = 0u;
    unsigned char count;
    unsigned char clicked;

    for (;;) {
        s64_ui_render(&editor);
        s64_menu_render(group, item);
        s64_input_read(&menu_input);

        switch (menu_input.type) {
            case S64_INPUT_LEFT:
                group = group == 0u ? S64_MENU_GROUPS - 1u : (unsigned char)(group - 1u);
                item = 0u;
                break;
            case S64_INPUT_RIGHT:
                group = (unsigned char)((group + 1u) % S64_MENU_GROUPS);
                item = 0u;
                break;
            case S64_INPUT_UP:
                count = s64_menu_item_count(group);
                item = item == 0u ? (unsigned char)(count - 1u) : (unsigned char)(item - 1u);
                break;
            case S64_INPUT_DOWN:
                count = s64_menu_item_count(group);
                item = (unsigned char)((item + 1u) % count);
                break;
            case S64_INPUT_NEWLINE:
            case S64_INPUT_ACTIVATE:
                return s64_menu_command(group, item);
            case S64_INPUT_HELP:
                return S64_INPUT_HELP;
            case S64_INPUT_POINTER_CLICK:
                if (menu_input.row == 0u) {
                    clicked = s64_menu_group_at(menu_input.column);
                    if (clicked != S64_MENU_NONE) {
                        group = clicked;
                        item = 0u;
                        break;
                    }
                    return S64_INPUT_NONE;
                }
                clicked = s64_menu_item_at(group, menu_input.column, menu_input.row);
                if (clicked != S64_MENU_NONE) {
                    return s64_menu_command(group, clicked);
                }
                return S64_INPUT_NONE;
            case S64_INPUT_MENU:
            case S64_INPUT_EXIT:
                return S64_INPUT_NONE;
            default:
                break;
        }
    }
}

int main(void)
{
    S64InputEvent input;
    unsigned char running = 1u;
    unsigned int previous_cursor;
    unsigned int previous_length;
    unsigned int previous_view_start;
    unsigned char previous_selection;
    unsigned char previous_mark_mode;
    unsigned char previous_cursor_row;
    unsigned char cursor_only;
    unsigned char inserted_value;
    unsigned char removed_value;
    unsigned char mouse_selecting = 0u;
#ifdef S64_REU_BUFFER
    unsigned char restored_handoff;

    if (!s64_reu_probe() || s64_reu_probe_banks() != 256u) {
        clrscr();
        cputs("SEE64YOU C64U\r\n16 MB REU REQUIRED\r\n");
        return 1;
    }
#endif

    s64_editor_init(&editor);
    s64_clipboard_init(&clipboard);
    s64_document_init(&document, &editor.buffer);
    s64_dos_init(&dos, s64_uci_execute, S64_DOS_TARGET_PRIMARY);
    s64_local_init(&local_document, &dos, &document);
#ifdef S64_REU_BUFFER
    restored_handoff = restore_publisher_handoff();
#endif
    s64_ui_init();
    s64_input_init();
    s64_uci_probe_capabilities(&uci_capabilities);
    s64_ui_set_http_ready(uci_capabilities.http_target);
    s64_ui_set_dos_ready(uci_capabilities.dos_target);
    s64_ui_render(&editor);
#ifdef S64_REU_BUFFER
    if (restored_handoff) {
        defer_recovery_autosave();
    } else {
        offer_recovery();
    }
#else
    offer_recovery();
#endif

    while (running) {
        maybe_autosave();
        s64_input_read(&input);
        cursor_only = 0u;

        if (input.type == S64_INPUT_MENU || input.type == S64_INPUT_ACTIVATE) {
            input.type = run_menu(0u);
            if (input.type == S64_INPUT_NONE) {
                s64_ui_render(&editor);
                continue;
            }
        } else if (input.type == S64_INPUT_POINTER_CLICK) {
            mouse_selecting = 0u;
            if (input.row == 0u) {
                unsigned char group = s64_menu_group_at(input.column);
                if (group != S64_MENU_NONE) {
                    input.type = run_menu(group);
                    if (input.type == S64_INPUT_NONE) {
                        s64_ui_render(&editor);
                        continue;
                    }
                } else {
                    continue;
                }
            } else if (input.row < 24u) {
                if (s64_editor_point(&editor, (unsigned char)(input.row - 1u),
                    input.column, 0u)) {
                    mouse_selecting = 1u;
                    s64_ui_render(&editor);
                }
                continue;
            } else {
                continue;
            }
        } else if (input.type == S64_INPUT_POINTER_DRAG) {
            if (mouse_selecting) {
                unsigned char row = input.row;
                if (row < 1u) row = 1u;
                if (row > 23u) row = 23u;
                if (s64_editor_point(&editor, (unsigned char)(row - 1u),
                    input.column, 1u)) {
                    s64_ui_render(&editor);
                }
            }
            continue;
        }

        switch (input.type) {
            case S64_INPUT_CHARACTER:
            case S64_INPUT_NEWLINE:
                inserted_value = input.type == S64_INPUT_NEWLINE
                    ? (unsigned char)'\n'
                    : input.character;
                previous_cursor = s64_buffer_cursor(&editor.buffer);
                previous_length = s64_buffer_length(&editor.buffer);
                previous_view_start = editor.view_start;
                previous_selection = s64_editor_has_selection(&editor);
                previous_mark_mode = editor.mark_mode;
                previous_cursor_row = editor.view_cursor_row;
                if (!(input.type == S64_INPUT_NEWLINE
                    ? s64_editor_newline(&editor)
                    : s64_editor_insert(&editor, inserted_value))) {
                    s64_ui_message("Document buffer full");
                    continue;
                }
                if (previous_cursor == previous_length
                    && !previous_selection
                    && !previous_mark_mode
                    && editor.word_wrap
                    && editor.view_start == previous_view_start) {
                    s64_ui_render_append(&editor, inserted_value);
                    continue;
                }
                if (!previous_selection
                    && !previous_mark_mode
                    && editor.word_wrap
                    && editor.view_start == previous_view_start) {
                    s64_ui_render_edit(
                        &editor,
                        (unsigned char)(
                            (input.type == S64_INPUT_CHARACTER
                                && previous_cursor_row == editor.view_cursor_row
                                ? S64_UI_EDIT_CHARACTER
                                : 0u)
                            | (previous_cursor_row < editor.view_cursor_row
                                ? previous_cursor_row
                                : editor.view_cursor_row)
                        )
                    );
                    continue;
                }
                break;

            case S64_INPUT_LEFT:
                s64_editor_left(&editor);
                cursor_only = 1u;
                break;
            case S64_INPUT_RIGHT:
                s64_editor_right(&editor);
                cursor_only = 1u;
                break;
            case S64_INPUT_UP:
                s64_editor_up(&editor);
                cursor_only = 1u;
                break;
            case S64_INPUT_DOWN:
                s64_editor_down(&editor);
                cursor_only = 1u;
                break;
            case S64_INPUT_HOME:
                s64_editor_home(&editor);
                cursor_only = 1u;
                break;
            case S64_INPUT_END:
                s64_editor_end(&editor);
                cursor_only = 1u;
                break;
            case S64_INPUT_PAGE_UP:
                s64_editor_page_up(&editor);
                cursor_only = 1u;
                break;
            case S64_INPUT_PAGE_DOWN:
                s64_editor_page_down(&editor);
                cursor_only = 1u;
                break;
            case S64_INPUT_WORD_LEFT:
                s64_editor_word_left(&editor);
                cursor_only = 1u;
                break;
            case S64_INPUT_WORD_RIGHT:
                s64_editor_word_right(&editor);
                cursor_only = 1u;
                break;

            case S64_INPUT_DELETE:
                if (!s64_editor_delete(&editor)) {
                    continue;
                }
                break;

            case S64_INPUT_BACKSPACE:
                previous_cursor = s64_buffer_cursor(&editor.buffer);
                previous_length = s64_buffer_length(&editor.buffer);
                previous_view_start = editor.view_start;
                previous_selection = s64_editor_has_selection(&editor);
                previous_mark_mode = editor.mark_mode;
                previous_cursor_row = editor.view_cursor_row;
                removed_value = previous_cursor > 0u
                    ? s64_buffer_char_at(&editor.buffer, previous_cursor - 1u)
                    : 0u;
                if (!s64_editor_backspace(&editor)) {
                    continue;
                }
                if (!previous_selection
                    && !previous_mark_mode
                    && editor.word_wrap
                    && editor.view_start == previous_view_start) {
                    if (previous_cursor == previous_length
                        && removed_value != '\n') {
                        gotoxy(
                            editor.view_cursor_column,
                            (unsigned char)(1u + editor.view_cursor_row)
                        );
                        cputc(' ');
                        s64_ui_render_cursor(&editor);
                    } else {
                        s64_ui_render_edit(
                            &editor,
                            (unsigned char)(
                                (removed_value != '\n'
                                    && previous_cursor_row == editor.view_cursor_row
                                    ? S64_UI_EDIT_CHARACTER
                                    : 0u)
                                | (previous_cursor_row < editor.view_cursor_row
                                    ? previous_cursor_row
                                    : editor.view_cursor_row)
                            )
                        );
                    }
                    continue;
                }
                break;
            case S64_INPUT_TOGGLE_INSERT:
                s64_editor_toggle_insert(&editor);
                cursor_only = 1u;
                break;
            case S64_INPUT_TOGGLE_WRAP:
                s64_editor_toggle_word_wrap(&editor);
                break;
            case S64_INPUT_HELP:
                s64_ui_show_help();
                s64_input_read(&input);
                s64_ui_render(&editor);
                continue;

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
#ifdef S64_REU_BUFFER
            case S64_INPUT_PUBLISH:
                launch_publisher();
                continue;
#endif
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
                if (!s64_editor_undo(&editor)) {
                    s64_ui_message("Nothing to undo");
                    continue;
                }
                if (!editor.dirty) {
                    clear_recovery_snapshot();
                }
                break;
            case S64_INPUT_REDO:
                if (!s64_editor_redo(&editor)) {
                    s64_ui_message("Nothing to redo");
                    continue;
                }
                if (!editor.dirty) {
                    clear_recovery_snapshot();
                }
                break;

            case S64_INPUT_FORMAT_BOLD:
                if (!s64_editor_format_bold(&editor)) {
                    s64_ui_message("Cannot format bold");
                    continue;
                }
                break;
            case S64_INPUT_FORMAT_ITALIC:
                if (!s64_editor_format_italic(&editor)) {
                    s64_ui_message("Cannot format italic");
                    continue;
                }
                break;
            case S64_INPUT_FORMAT_LIST:
                if (!s64_editor_format_list_item(&editor)) {
                    s64_ui_message("Cannot format list");
                    continue;
                }
                break;
            case S64_INPUT_EXIT:
                if (editor.dirty && !s64_ui_prompt_yes_no("Discard and exit? Y/N")) {
                    s64_ui_render(&editor);
                    continue;
                }
                if (editor.dirty) {
                    clear_recovery_snapshot();
                }
                running = 0u;
                continue;

            case S64_INPUT_NONE:
            default:
                continue;
        }

        if (cursor_only
            && !editor.mark_mode
            && !s64_editor_has_selection(&editor)) {
            s64_ui_render_cursor(&editor);
        } else {
            s64_ui_render(&editor);
        }
    }

    s64_input_restore_repeat();
    s64_input_shutdown();
    clrscr();
    return 0;
}
