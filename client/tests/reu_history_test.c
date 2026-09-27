#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "editor.h"
#include "platform/reu.h"

#define TEST_REU_BANKS 2u
#define TEST_REU_BANK_SIZE 65536u

static unsigned char fake_reu[TEST_REU_BANKS][TEST_REU_BANK_SIZE];

uint8_t s64_reu_probe(void)
{
    return 1u;
}

uint16_t s64_reu_probe_banks(void)
{
    return TEST_REU_BANKS;
}

uint8_t s64_reu_store(
    uint8_t bank,
    uint16_t address,
    const void* source,
    uint16_t length
)
{
    if (bank >= TEST_REU_BANKS || source == 0
        || (unsigned long)address + length > TEST_REU_BANK_SIZE) {
        return 0u;
    }

    memcpy(&fake_reu[bank][address], source, length);
    return 1u;
}

uint8_t s64_reu_load(
    uint8_t bank,
    uint16_t address,
    void* destination,
    uint16_t length
)
{
    if (bank >= TEST_REU_BANKS || destination == 0
        || (unsigned long)address + length > TEST_REU_BANK_SIZE) {
        return 0u;
    }

    memcpy(destination, &fake_reu[bank][address], length);
    return 1u;
}

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void require_text(
    const S64Editor* editor,
    const char* expected,
    const char* message
)
{
    unsigned int i;
    unsigned int length = (unsigned int)strlen(expected);

    require(s64_buffer_length(&editor->buffer) == length, message);
    for (i = 0u; i < length; ++i) {
        require(
            s64_buffer_char_at(&editor->buffer, i) == (unsigned char)expected[i],
            message
        );
    }
}

static void break_typing_transaction(S64Editor* editor)
{
    s64_editor_left(editor);
    s64_editor_right(editor);
}

int main(void)
{
    S64Editor editor;
    unsigned int i;

    memset(fake_reu, 0, sizeof(fake_reu));

    s64_editor_init(&editor);
    require(s64_editor_insert(&editor, 'a'), "insert a");
    require(s64_editor_insert(&editor, 'b'), "insert b");
    require(s64_editor_insert(&editor, 'c'), "insert c");
    s64_editor_left(&editor);
    require(s64_editor_insert(&editor, 'X'), "insert second transaction");
    s64_editor_end(&editor);
    require(s64_editor_insert(&editor, '!'), "insert third transaction");
    require_text(&editor, "abXc!", "three transaction fixture");

    require(s64_editor_undo(&editor), "undo third transaction");
    require_text(&editor, "abXc", "first undo");
    require(s64_editor_undo(&editor), "undo second transaction");
    require_text(&editor, "abc", "second undo");
    require(s64_editor_undo(&editor), "undo first transaction");
    require_text(&editor, "", "third undo");
    require(!s64_editor_undo(&editor), "no fourth undo");

    require(s64_editor_redo(&editor), "redo first transaction");
    require_text(&editor, "abc", "first redo");
    require(s64_editor_redo(&editor), "redo second transaction");
    require_text(&editor, "abXc", "second redo");
    require(s64_editor_redo(&editor), "redo third transaction");
    require_text(&editor, "abXc!", "third redo");
    require(!s64_editor_redo(&editor), "no fourth redo");

    s64_editor_init(&editor);
    require(s64_editor_insert(&editor, 'a'), "saved-state insert");
    s64_editor_mark_saved(&editor);
    require(!editor.dirty, "mark saved is clean");
    require(s64_editor_insert(&editor, 'b'), "edit after save");
    require(editor.dirty, "edit after save is dirty");
    require(s64_editor_undo(&editor), "undo to saved state");
    require_text(&editor, "a", "saved state content");
    require(!editor.dirty, "undo to saved state is clean");
    require(s64_editor_undo(&editor), "undo before saved state");
    require_text(&editor, "", "pre-save content");
    require(editor.dirty, "undo before saved state is dirty");
    require(s64_editor_redo(&editor), "redo to saved state");
    require_text(&editor, "a", "redo saved content");
    require(!editor.dirty, "redo to saved state is clean");
    require(s64_editor_redo(&editor), "redo after saved state");
    require_text(&editor, "ab", "redo edited content");
    require(editor.dirty, "redo after saved state is dirty");

    s64_editor_init(&editor);
    require(s64_editor_insert(&editor, 'a'), "branch insert a");
    break_typing_transaction(&editor);
    require(s64_editor_insert(&editor, 'b'), "branch insert b");
    require(s64_editor_undo(&editor), "branch undo b");
    require_text(&editor, "a", "branch undo content");
    require(s64_editor_insert(&editor, 'c'), "branch replacement edit");
    require_text(&editor, "ac", "branch replacement content");
    require(!s64_editor_redo(&editor), "new edit discards redo branch");

    s64_editor_init(&editor);
    for (i = 0u; i < 20u; ++i) {
        require(
            s64_editor_insert(&editor, (unsigned char)('a' + i)),
            "ring insert"
        );
        break_typing_transaction(&editor);
    }
    require(s64_buffer_length(&editor.buffer) == 20u, "ring fixture length");

    for (i = 0u; i < S64_HISTORY_LEVELS; ++i) {
        require(s64_editor_undo(&editor), "ring undo retained transaction");
    }
    require(
        s64_buffer_length(&editor.buffer) == 4u,
        "ring retains newest sixteen undo levels"
    );
    require(!s64_editor_undo(&editor), "ring drops oldest transactions");

    for (i = 0u; i < S64_HISTORY_LEVELS; ++i) {
        require(s64_editor_redo(&editor), "ring redo retained transaction");
    }
    require(s64_buffer_length(&editor.buffer) == 20u, "ring redo restores document");
    require(!s64_editor_redo(&editor), "ring redo exhausted");

    puts("REU multi-level history tests passed");
    return 0;
}
