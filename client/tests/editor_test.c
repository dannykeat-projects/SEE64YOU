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

    s64_editor_init(&editor);
    insert_text(&editor, "12345\nx\nABCDE");
    s64_editor_home(&editor);
    s64_editor_right(&editor);
    s64_editor_right(&editor);
    s64_editor_right(&editor);
    s64_editor_right(&editor);
    require(editor.column == 5u, "fixture column");
    s64_editor_up(&editor);
    require(editor.line == 2u && editor.column == 2u, "short line clamps vertical column");
    s64_editor_up(&editor);
    require(editor.line == 1u && editor.column == 5u, "vertical movement restores preferred column");
    s64_editor_down(&editor);
    s64_editor_down(&editor);
    require(editor.line == 3u && editor.column == 5u, "preferred column survives repeated vertical movement");
    s64_editor_end(&editor);
    require(editor.column == 6u, "end moves to line end");

    s64_editor_init(&editor);
    insert_text(&editor, "ab\ncd");
    require(editor.line == 2u && editor.column == 3u, "multiline typing position");
    s64_editor_left(&editor);
    require(editor.line == 2u && editor.column == 2u, "left updates position locally");
    require(s64_editor_insert(&editor, 'X'), "mid-line insert succeeds");
    require_text(&editor, "ab\ncXd", "mid-line insert content");
    require(editor.line == 2u && editor.column == 3u, "mid-line insert updates position locally");
    require(s64_editor_backspace(&editor), "mid-line backspace succeeds");
    require_text(&editor, "ab\ncd", "mid-line backspace content");
    require(editor.line == 2u && editor.column == 2u, "backspace updates position locally");
    s64_editor_home(&editor);
    require(editor.line == 2u && editor.column == 1u, "home reaches second-line start");
    require(s64_editor_backspace(&editor), "backspace joins logical lines");
    require_text(&editor, "abcd", "newline backspace joins lines");
    require(editor.line == 1u && editor.column == 3u, "newline backspace restores previous line position");

    s64_editor_init(&editor);
    insert_text(&editor, "abcd");
    s64_editor_home(&editor);
    s64_editor_right(&editor);
    require(editor.line == 1u && editor.column == 2u, "forward delete fixture position");
    require(s64_editor_delete(&editor), "forward delete character succeeds");
    require_text(&editor, "acd", "forward delete removes character at cursor");
    require(editor.line == 1u && editor.column == 2u, "forward delete preserves logical position");
    require(editor.view_cursor_row == 0u && editor.view_cursor_column == 1u, "forward delete preserves view cursor");
    require(s64_editor_undo(&editor), "undo forward delete");
    require_text(&editor, "abcd", "undo restores forward-deleted character");
    require(s64_editor_redo(&editor), "redo forward delete");
    require_text(&editor, "acd", "redo removes forward-deleted character again");

    s64_editor_init(&editor);
    insert_text(&editor, "ab\ncd");
    require(s64_editor_go_to_line(&editor, 1u), "go to first line for newline movement");
    s64_editor_end(&editor);
    require(editor.line == 1u && editor.column == 3u, "first line end position");
    s64_editor_right(&editor);
    require(editor.line == 2u && editor.column == 1u, "right across newline updates position locally");
    s64_editor_left(&editor);
    require(editor.line == 1u && editor.column == 3u, "left across newline updates position locally");
    require(s64_editor_delete(&editor), "delete newline succeeds");
    require_text(&editor, "abcd", "delete newline joins lines");
    require(editor.line == 1u && editor.column == 3u, "delete keeps cursor position");

    s64_editor_init(&editor);
    insert_text(&editor, "ab\ncd");
    require(s64_editor_go_to_line(&editor, 1u), "go to first line for overwrite");
    s64_editor_end(&editor);
    s64_editor_toggle_insert(&editor);
    require(s64_editor_insert(&editor, 'X'), "overwrite newline with character");
    require_text(&editor, "abXcd", "overwrite newline content");
    require(editor.line == 1u && editor.column == 4u, "overwrite character updates position locally");

    s64_editor_init(&editor);
    insert_text(&editor, "abcd");
    s64_editor_home(&editor);
    s64_editor_right(&editor);
    s64_editor_right(&editor);
    s64_editor_toggle_insert(&editor);
    require(s64_editor_newline(&editor), "overwrite character with newline");
    require_text(&editor, "ab\nd", "overwrite with newline content");
    require(editor.line == 2u && editor.column == 1u, "overwrite newline updates position locally");

    s64_editor_init(&editor);
    insert_text(&editor, "abcdef\nx\nabcdef");
    s64_editor_up(&editor);
    require(editor.column == 2u, "vertical fixture reaches short line");
    require(s64_editor_backspace(&editor), "backspace edits short line");
    s64_editor_up(&editor);
    require(editor.column == 1u, "backspace resets preferred vertical column");

    s64_editor_init(&editor);
    insert_text(&editor, "abcdef\nx\nabcdef");
    s64_editor_up(&editor);
    clipboard.data[0] = 'Z';
    clipboard.length = 1u;
    require(s64_editor_paste(&editor, &clipboard), "paste into short line");
    s64_editor_up(&editor);
    require(editor.column == 3u, "paste resets preferred vertical column");

    s64_editor_init(&editor);
    insert_text(&editor, "one two  three");
    s64_editor_home(&editor);
    s64_editor_word_right(&editor);
    require(s64_buffer_cursor(&editor.buffer) == 4u, "word right skips first word and separator");
    s64_editor_word_right(&editor);
    require(s64_buffer_cursor(&editor.buffer) == 9u, "word right skips repeated separators");
    s64_editor_word_left(&editor);
    require(s64_buffer_cursor(&editor.buffer) == 4u, "word left returns to previous word");

    s64_editor_init(&editor);
    insert_text(&editor, "one\ntwo");
    s64_editor_word_left(&editor);
    require(editor.line == 2u && editor.column == 1u, "word left reaches current line start");
    s64_editor_word_left(&editor);
    require(editor.line == 1u && editor.column == 1u, "word left crosses newline locally");
    s64_editor_word_right(&editor);
    require(editor.line == 2u && editor.column == 1u, "word right crosses newline locally");

    s64_editor_init(&editor);
    insert_text(&editor, "alpha beta alpha");
    s64_editor_home(&editor);
    require(s64_editor_find_next(&editor, "alpha", 1u), "find first match");
    require(s64_editor_selection_start(&editor) == 0u, "first match start");
    require(s64_editor_selection_end(&editor) == 5u, "first match end");
    require(s64_editor_find_next(&editor, "alpha", 1u), "find next match");
    require(s64_editor_selection_start(&editor) == 11u, "second match start");
    require(s64_editor_find_next(&editor, "alpha", 1u), "find next wraps");
    require(s64_editor_selection_start(&editor) == 0u, "wrapped match start");
    require(s64_editor_find_previous(&editor, "alpha", 1u), "find previous wraps");
    require(s64_editor_selection_start(&editor) == 11u, "previous wrapped match start");

    require(s64_editor_find_next(&editor, "beta", 1u), "find replacement target");
    require(s64_editor_replace_selection(&editor, "B"), "replace selected match");
    require_text(&editor, "alpha B alpha", "replace selection content");

    s64_editor_init(&editor);
    insert_text(&editor, "aa aa aa");
    {
        unsigned int replaced = 0u;
        require(s64_editor_replace_all(&editor, "aa", "b", &replaced), "replace all succeeds");
        require(replaced == 3u, "replace all count");
        require_text(&editor, "b b b", "replace all content");
    }

    s64_editor_init(&editor);
    insert_text(&editor, "aba");
    {
        unsigned int replaced = 0u;
        require(s64_editor_replace_all(&editor, "a", "aa", &replaced), "replace all growth succeeds");
        require(replaced == 2u, "replace all growth count");
        require_text(&editor, "aabaa", "replace all skips replacement text");
    }

    s64_editor_init(&editor);
    require(editor.word_wrap, "word wrap enabled by default");
    require(editor.view_column == 0u, "wrapped viewport starts at first column");
    insert_text(&editor, "123456789012345678901234567890123456789012345");
    require(editor.view_column == 0u, "wrapped long line does not scroll horizontally");
    s64_editor_toggle_word_wrap(&editor);
    require(!editor.word_wrap, "word wrap toggles off");
    require(editor.view_column == 6u, "no-wrap follows cursor horizontally");
    s64_editor_home(&editor);
    require(editor.view_column == 0u, "home scrolls no-wrap viewport left");
    s64_editor_end(&editor);
    require(editor.view_column == 6u, "end restores horizontal cursor visibility");
    s64_editor_toggle_word_wrap(&editor);
    require(editor.word_wrap, "word wrap toggles on");
    require(editor.view_column == 0u, "re-enabling wrap clears horizontal offset");

    s64_editor_init(&editor);
    insert_text(&editor, "123456789012345678901234567890123456789012345");
    s64_editor_up(&editor);
    require(s64_buffer_cursor(&editor.buffer) == 5u, "wrapped up moves one visual row");
    require(editor.line == 1u && editor.column == 6u, "wrapped up preserves logical line position");
    s64_editor_down(&editor);
    require(s64_buffer_cursor(&editor.buffer) == 45u, "wrapped down restores visual column");
    require(editor.line == 1u && editor.column == 46u, "wrapped down preserves logical line position");

    s64_editor_init(&editor);
    {
        unsigned int i;
        for (i = 0u; i < 960u; ++i) {
            require(s64_editor_insert(&editor, 'x'), "fill wrapped viewport");
        }
    }
    require(editor.view_start == 80u, "wrapped viewport scrolls by visual rows");
    s64_editor_page_up(&editor);
    require(s64_buffer_cursor(&editor.buffer) == 80u, "page up moves through wrapped rows");
    require(editor.line == 1u && editor.column == 81u, "page up preserves logical position");
    require(editor.view_start <= 80u, "page up reveals previous wrapped page");

    s64_editor_init(&editor);
    insert_text(&editor, "one\ntwo\nthree");
    require(s64_editor_go_to_line(&editor, 2u), "go to existing line");
    require(editor.line == 2u && editor.column == 1u, "go to line position");
    s64_editor_right(&editor);
    require(editor.line == 2u && editor.column == 2u, "go to line keeps incremental position synchronized");
    require(s64_editor_go_to_line(&editor, 2u), "go to current line");
    require(
        s64_buffer_cursor(&editor.buffer) == 4u
            && editor.line == 2u
            && editor.column == 1u,
        "go to current line returns to line start"
    );
    require(s64_editor_go_to_line(&editor, 3u), "go forward from current line");
    require(
        s64_buffer_cursor(&editor.buffer) == 8u
            && editor.line == 3u
            && editor.column == 1u,
        "forward go to line position"
    );
    require(s64_editor_go_to_line(&editor, 2u), "go backward from current line");
    require(
        s64_buffer_cursor(&editor.buffer) == 4u
            && editor.line == 2u
            && editor.column == 1u,
        "backward go to line position"
    );
    require(s64_editor_go_to_line(&editor, 1u), "go directly to first line");
    require(s64_buffer_cursor(&editor.buffer) == 0u, "first line starts at zero");
    require(s64_editor_go_to_line(&editor, 2u), "return to second line");
    s64_editor_right(&editor);
    require(!s64_editor_go_to_line(&editor, 4u), "reject missing line");
    require(editor.line == 2u && editor.column == 2u, "missing line preserves position");

    s64_buffer_init(&editor.buffer);
    require(s64_buffer_insert(&editor.buffer, 'n'), "load fixture first character");
    require(s64_buffer_insert(&editor.buffer, 'e'), "load fixture second character");
    require(s64_buffer_insert(&editor.buffer, 'w'), "load fixture third character");
    require(s64_buffer_move_cursor(&editor.buffer, 0u), "load fixture starts at document beginning");
    s64_editor_document_loaded(&editor);
    s64_editor_right(&editor);
    require(editor.line == 1u && editor.column == 2u, "document load resets incremental position tracking");

    s64_editor_init(&editor);
    insert_text(&editor, "hello");
    s64_editor_home(&editor);
    s64_editor_toggle_mark(&editor);
    s64_editor_end(&editor);
    s64_editor_toggle_mark(&editor);
    require(s64_editor_format_bold(&editor), "bold wraps selection");
    require_text(&editor, "**hello**", "bold source markers");
    require(s64_editor_selection_start(&editor) == 2u, "bold keeps label selection start");
    require(s64_editor_selection_end(&editor) == 7u, "bold keeps label selection end");

    require(s64_editor_format_italic(&editor), "italic nests with bold");
    require_text(&editor, "***hello***", "bold italic nesting");
    require(s64_editor_format_italic(&editor), "italic toggle removes nested italic");
    require_text(&editor, "**hello**", "italic toggle preserves bold");
    require(s64_editor_format_bold(&editor), "bold toggle removes bold");
    require_text(&editor, "hello", "bold toggle restores source");

    s64_editor_clear_selection(&editor);
    s64_editor_end(&editor);
    require(s64_editor_format_bold(&editor), "bold inserts paired markers without selection");
    require_text(&editor, "hello****", "bold empty marker pair");
    require(s64_buffer_cursor(&editor.buffer) == 7u, "cursor placed inside bold markers");

    s64_editor_init(&editor);
    insert_text(&editor, "Title");
    require(s64_editor_format_heading(&editor, 1u), "heading one adds prefix");
    require_text(&editor, "# Title", "heading one source");
    require(s64_editor_format_heading(&editor, 2u), "heading level replaces existing heading");
    require_text(&editor, "## Title", "heading two source");
    s64_editor_home(&editor);
    s64_editor_right(&editor);
    require(s64_editor_format_heading(&editor, 1u), "heading replacement safe with cursor in prefix");
    require_text(&editor, "# Title", "heading replacement from prefix");
    require(s64_editor_format_heading(&editor, 1u), "same heading toggles off");
    require_text(&editor, "Title", "heading toggle restores line");
    require(!s64_editor_format_heading(&editor, 3u), "unsupported heading level rejected");
    require_text(&editor, "Title", "unsupported heading leaves source unchanged");

    s64_editor_init(&editor);
    insert_text(&editor, "item");
    s64_editor_home(&editor);
    require(s64_editor_format_list_item(&editor), "list item adds prefix");
    require_text(&editor, "- item", "list item source");
    require(s64_editor_format_list_item(&editor), "list item toggles off");
    require_text(&editor, "item", "list toggle restores line");

    s64_editor_init(&editor);
    insert_text(&editor, "OpenAI");
    s64_editor_home(&editor);
    s64_editor_toggle_mark(&editor);
    s64_editor_end(&editor);
    s64_editor_toggle_mark(&editor);
    require(
        s64_editor_format_link(&editor, "https://openai.com"),
        "link wraps selected label"
    );
    require_text(
        &editor,
        "[OpenAI](https://openai.com)",
        "link source syntax"
    );
    require(s64_editor_selection_start(&editor) == 1u, "link keeps label selected");
    require(s64_editor_selection_end(&editor) == 7u, "link selection end");

    s64_editor_init(&editor);
    insert_text(&editor, "bad]label");
    s64_editor_select_all(&editor);
    require(
        !s64_editor_format_link(&editor, "https://example.com"),
        "unsafe link label rejected"
    );
    require_text(&editor, "bad]label", "rejected link leaves source unchanged");
    require(
        !s64_editor_format_link(&editor, "https://example.com/a)b"),
        "unsafe link URL rejected"
    );
    require_text(&editor, "bad]label", "rejected URL leaves source unchanged");
    require(
        !s64_editor_format_link(&editor, "ftp://example.com/file"),
        "unsupported link scheme rejected"
    );
    require_text(&editor, "bad]label", "unsupported scheme leaves source unchanged");

    s64_editor_init(&editor);
    {
        unsigned int i;
        for (i = 0u; i < S64_BUFFER_CAPACITY - 1u; ++i) {
            require(s64_editor_insert(&editor, 'x'), "fill buffer for formatting capacity test");
        }
    }
    require(
        !s64_editor_format_bold(&editor),
        "bold preflight rejects insufficient free space"
    );
    require(
        s64_buffer_length(&editor.buffer) == S64_BUFFER_CAPACITY - 1u,
        "failed formatting makes no partial edit"
    );

    s64_editor_init(&editor);
    insert_text(&editor, "abc");
    require(s64_editor_undo(&editor), "undo coalesced typing");
    require_text(&editor, "", "typing undo restores empty document");
    require(!editor.dirty, "undo first edit restores clean state");
    require(s64_editor_redo(&editor), "redo coalesced typing");
    require_text(&editor, "abc", "typing redo restores text");
    require(editor.dirty, "redo edit restores dirty state");

    s64_editor_mark_saved(&editor);
    require(!editor.dirty, "mark saved clears dirty state");
    require(s64_editor_undo(&editor), "undo after save");
    require_text(&editor, "", "undo after save restores previous text");
    require(editor.dirty, "undo away from saved state becomes dirty");
    require(s64_editor_redo(&editor), "redo back to saved state");
    require_text(&editor, "abc", "redo returns saved text");
    require(!editor.dirty, "redo to saved state becomes clean");

    s64_editor_init(&editor);
    insert_text(&editor, "ab");
    s64_editor_left(&editor);
    require(s64_editor_insert(&editor, 'X'), "insert after cursor move");
    require_text(&editor, "aXb", "cursor edit content");
    require(s64_editor_undo(&editor), "cursor movement breaks typing transaction");
    require_text(&editor, "ab", "undo removes only post-move edit");
    require(s64_editor_redo(&editor), "redo post-move edit");
    require_text(&editor, "aXb", "redo restores post-move edit");

    s64_editor_init(&editor);
    insert_text(&editor, "hello");
    s64_editor_mark_saved(&editor);
    require(s64_editor_backspace(&editor), "backspace for undo");
    require_text(&editor, "hell", "backspace content");
    require(s64_editor_undo(&editor), "undo backspace");
    require_text(&editor, "hello", "undo restores backspaced character");
    require(!editor.dirty, "undo backspace restores saved state");
    require(s64_editor_redo(&editor), "redo backspace");
    require_text(&editor, "hell", "redo removes character again");

    s64_editor_init(&editor);
    insert_text(&editor, "hello world");
    s64_editor_mark_saved(&editor);
    s64_editor_home(&editor);
    s64_editor_toggle_mark(&editor);
    s64_editor_right(&editor);
    s64_editor_right(&editor);
    s64_editor_right(&editor);
    s64_editor_right(&editor);
    s64_editor_right(&editor);
    s64_editor_toggle_mark(&editor);
    require(s64_editor_cut(&editor, &clipboard), "cut for undo");
    require_text(&editor, " world", "cut edit content");
    require(s64_editor_undo(&editor), "undo cut");
    require_text(&editor, "hello world", "undo cut restores selection text");
    require(s64_editor_redo(&editor), "redo cut");
    require_text(&editor, " world", "redo cut removes selection again");

    require(s64_editor_paste(&editor, &clipboard), "paste for undo");
    require_text(&editor, "hello world", "paste edit content");
    require(s64_editor_undo(&editor), "undo paste");
    require_text(&editor, " world", "undo paste removes inserted text");
    require(s64_editor_redo(&editor), "redo paste");
    require_text(&editor, "hello world", "redo paste restores inserted text");

    s64_editor_init(&editor);
    insert_text(&editor, "aa aa aa");
    s64_editor_mark_saved(&editor);
    {
        unsigned int replaced = 0u;
        require(s64_editor_replace_all(&editor, "aa", "b", &replaced), "replace all undo fixture");
        require(replaced == 3u, "replace all undo count");
    }
    require_text(&editor, "b b b", "replace all undo changed content");
    require(s64_editor_undo(&editor), "undo replace all as one transaction");
    require_text(&editor, "aa aa aa", "undo replace all restores source");
    require(!editor.dirty, "undo replace all restores saved state");
    require(s64_editor_redo(&editor), "redo replace all as one transaction");
    require_text(&editor, "b b b", "redo replace all restores replacements");

    s64_editor_init(&editor);
    insert_text(&editor, "hello");
    s64_editor_mark_saved(&editor);
    s64_editor_select_all(&editor);
    require(s64_editor_format_bold(&editor), "bold for undo");
    require_text(&editor, "**hello**", "bold undo fixture");
    require(s64_editor_undo(&editor), "undo formatting");
    require_text(&editor, "hello", "undo formatting restores source");
    require(s64_editor_redo(&editor), "redo formatting");
    require_text(&editor, "**hello**", "redo formatting restores markers");

    puts("editor tests passed");
    return 0;
}
