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

    puts("REU editor clipboard tests passed");
    return 0;
}
