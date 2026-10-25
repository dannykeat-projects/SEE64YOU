#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clipboard.h"
#include "editor.h"
#include "platform/reu.h"

#define TEST_REU_BANKS 2u
#define TEST_REU_BANK_SIZE 65536u
#define TEST_TEXT_LENGTH 1024u

static unsigned char fake_reu[TEST_REU_BANKS][TEST_REU_BANK_SIZE];
static unsigned int fail_clipboard_read;
static unsigned int fail_document_store;

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
    if (bank == 0u && address < S64_BUFFER_CAPACITY && fail_document_store
        && --fail_document_store == 0u) {
        return 0u;
    }
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
    if (bank == 1u && address < S64_CLIPBOARD_CAPACITY && fail_clipboard_read
        && --fail_clipboard_read == 0u) {
        return 0u;
    }
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

static unsigned char fixture_value(unsigned int index)
{
    return (unsigned char)('A' + (index % 26u));
}

static void require_document(const S64Editor* editor, unsigned int length)
{
    unsigned int i;

    require(s64_buffer_length(&editor->buffer) == length, "document length");
    for (i = 0u; i < length; ++i) {
        require(
            s64_buffer_char_at(&editor->buffer, i) == fixture_value(i),
            "document content"
        );
    }
}

int main(void)
{
    S64Editor editor;
    S64Clipboard clipboard;
    unsigned char readback[64];
    unsigned int i;

    memset(fake_reu, 0, sizeof(fake_reu));
    s64_editor_init(&editor);
    s64_clipboard_init(&clipboard);

    for (i = 0u; i < TEST_TEXT_LENGTH; ++i) {
        require(
            s64_editor_insert(&editor, fixture_value(i)),
            "insert REU editor fixture"
        );
    }

    s64_editor_select_all(&editor);
    require(s64_editor_copy(&editor, &clipboard), "copy >256 bytes to REU clipboard");
    require(clipboard.length == TEST_TEXT_LENGTH, "large clipboard length");
    require(
        s64_clipboard_read(&clipboard, 500u, readback, sizeof(readback)) == sizeof(readback),
        "read middle of REU clipboard"
    );
    for (i = 0u; i < sizeof(readback); ++i) {
        require(
            readback[i] == fixture_value(500u + i),
            "REU clipboard content"
        );
    }

    s64_editor_init(&editor);
    require(s64_editor_paste(&editor, &clipboard), "paste >256 bytes from REU clipboard");
    require_document(&editor, TEST_TEXT_LENGTH);

    s64_editor_select_all(&editor);
    require(s64_editor_cut(&editor, &clipboard), "cut >256 bytes to REU clipboard");
    require(s64_buffer_length(&editor.buffer) == 0u, "large cut clears document");
    require(clipboard.length == TEST_TEXT_LENGTH, "large cut preserves clipboard");

    /* One-shot failures leave recoverable partial text, never a clean document. */
    s64_editor_init(&editor);
    fail_clipboard_read = 1u;
    require(!s64_editor_paste(&editor, &clipboard), "first clipboard read fails");
    require(!editor.dirty, "no-op failed paste stays clean");
    require(s64_buffer_length(&editor.buffer) == 0u, "first read leaves bytes intact");

    fail_clipboard_read = 2u;
    require(!s64_editor_paste(&editor, &clipboard), "second clipboard read fails");
    require(editor.dirty, "partial clipboard paste is dirty");
    require_document(&editor, 64u);
    require(editor.column == 65u, "partial paste cursor is recalculated");
    require(!s64_editor_undo(&editor), "partial paste discards unreliable history");

    s64_editor_init(&editor);
    fail_document_store = 8u;
    require(!s64_editor_paste(&editor, &clipboard), "midpaste document write fails");
    require(editor.dirty, "partial failed write is dirty");
    require_document(&editor, 7u);
    require(editor.column == 8u, "failed write cursor is recalculated");
    require(!s64_editor_undo(&editor), "failed write discards unreliable history");

    s64_editor_init(&editor);
    require(s64_editor_insert(&editor, 'x'), "selection failure fixture");
    s64_editor_mark_saved(&editor);
    s64_editor_select_all(&editor);
    fail_clipboard_read = 1u;
    require(!s64_editor_paste(&editor, &clipboard), "replacement clipboard read fails");
    require(editor.dirty, "deleted selection is dirty after failed paste");
    require(s64_buffer_length(&editor.buffer) == 0u, "failed replacement preserves current bytes");
    require(!s64_editor_undo(&editor), "failed replacement discards unreliable history");

    puts("REU editor clipboard tests passed");
    return 0;
}
