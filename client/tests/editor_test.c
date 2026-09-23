#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clipboard.h"
#include "editor.h"

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void insert_text(S64Editor* editor, const char* text)
{
    while (*text != '\0') {
        require(s64_editor_insert(editor, (unsigned char)*text), "insert fixture text");
        ++text;
    }
}

static void require_text(const S64Editor* editor, const char* expected, const char* message)
{
    unsigned int i;
    size_t length = strlen(expected);

    require(s64_buffer_length(&editor->buffer) == length, message);
    for (i = 0u; i < length; ++i) {
        require(s64_buffer_char_at(&editor->buffer, i) == (unsigned char)expected[i], message);
    }
}

int main(void)
{
    S64Editor editor;
    S64Clipboard clipboard;

    s64_editor_init(&editor);
    s64_clipboard_init(&clipboard);
    insert_text(&editor, "hello world");

    s64_editor_home(&editor);
    require(s64_buffer_cursor(&editor.buffer) == 0u, "home moves to line start");

    s64_editor_toggle_mark(&editor);
    s64_editor_right(&editor);
    s64_editor_right(&editor);
    s64_editor_right(&editor);
    s64_editor_right(&editor);
    s64_editor_right(&editor);

    require(editor.mark_mode, "mark mode remains active");
    require(s64_editor_has_selection(&editor), "movement creates selection");
    require(s64_editor_selection_start(&editor) == 0u, "selection start");
    require(s64_editor_selection_end(&editor) == 5u, "selection end");

    s64_editor_toggle_mark(&editor);
    require(!editor.mark_mode, "mark mode can stop while selection remains");
    require(s64_editor_copy(&editor, &clipboard), "copy selection");
    require(clipboard.length == 5u, "clipboard length");
    require(memcmp(clipboard.data, "hello", 5u) == 0, "clipboard content");

    require(s64_editor_cut(&editor, &clipboard), "cut selection");
    require_text(&editor, " world", "cut removes selected text");
    require(s64_buffer_cursor(&editor.buffer) == 0u, "cut leaves cursor at selection start");

    require(s64_editor_paste(&editor, &clipboard), "paste clipboard");
    require_text(&editor, "hello world", "paste restores text");

    s64_editor_select_all(&editor);
    require(s64_editor_has_selection(&editor), "select all selects document");
    require(s64_editor_insert(&editor, 'X'), "typing replaces selection");
    require_text(&editor, "X", "selection replacement");

    s64_editor_select_all(&editor);
    require(s64_editor_backspace(&editor), "backspace removes selection");
    require_text(&editor, "", "selection deletion");

    puts("editor tests passed");
    return 0;
}
