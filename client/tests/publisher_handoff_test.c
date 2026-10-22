#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clipboard.h"
#include "document.h"
#include "editor.h"
#include "platform/reu.h"
#include "platform/reu_layout.h"
#include "publisher/handoff.h"

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

static void insert_text(S64Editor* editor, const char* text)
{
    while (*text != '\0') {
        require(
            s64_editor_insert(editor, (unsigned char)*text),
            "insert fixture text"
        );
        ++text;
    }
}

static void require_body(
    const S64Buffer* buffer,
    const char* expected,
    const char* message
)
{
    unsigned int i;
    unsigned int length = (unsigned int)strlen(expected);

    require(s64_buffer_length(buffer) == length, message);
    for (i = 0u; i < length; ++i) {
        require(
            s64_buffer_char_at(buffer, i) == (unsigned char)expected[i],
            message
        );
    }
}

int main(void)
{
    S64Editor editor;
    S64Document document;
    S64PublisherHandoff outgoing;
    S64PublisherHandoff incoming;
    S64Buffer restored_buffer;
    S64Document restored_document;
    char filename[S64_LOCAL_FILENAME_CAPACITY];
    unsigned char before_handoff;
    unsigned char after_handoff;

    memset(fake_reu, 0, sizeof(fake_reu));
    memset(filename, 0, sizeof(filename));
    strcpy(filename, "articles/draft.s64");

    s64_editor_init(&editor);
    s64_document_init(&document, &editor.buffer);
    insert_text(&editor, "hello from the editor\nsecond line");

    document.type = S64_DOCUMENT_POST;
    document.status = S64_DOCUMENT_DRAFT;
    document.order = -7;
    require(s64_document_set_slug(&document, "hello-editor"), "set slug");
    require(s64_document_set_title(&document, "Hello Editor"), "set title");
    require(s64_document_set_date(&document, "2026-09-28"), "set date");

    s64_editor_left(&editor);
    s64_editor_toggle_insert(&editor);
    s64_editor_toggle_word_wrap(&editor);

    require(editor.dirty, "fixture remains dirty");
    require(!editor.insert_mode, "fixture overwrite mode");
    require(!editor.word_wrap, "fixture horizontal mode");

    require(
        s64_history_prepare_handoff(&editor.history),
        "flush current history metadata"
    );

    fake_reu[S64_REU_HANDOFF_BANK][S64_REU_HANDOFF_BASE - 1u] = 0x5au;
    fake_reu[S64_REU_HANDOFF_BANK][65535u] = 0xa5u;
    before_handoff = fake_reu[S64_REU_HANDOFF_BANK][S64_REU_HANDOFF_BASE - 1u];
    after_handoff = fake_reu[S64_REU_HANDOFF_BANK][65535u];

    require(
        s64_handoff_capture(
            &outgoing,
            &editor.buffer,
            &document,
            &editor.history,
            filename,
            editor.dirty,
            editor.insert_mode,
            editor.word_wrap
        ),
        "capture handoff"
    );
    require(s64_handoff_store(&outgoing), "store handoff in REU");

    require(
        fake_reu[S64_REU_HANDOFF_BANK][S64_REU_HANDOFF_BASE - 1u]
            == before_handoff,
        "handoff does not overwrite history metadata region"
    );

    require(s64_handoff_load(&incoming), "load handoff from REU");
    require(
        s64_handoff_apply_document(
            &incoming,
            &restored_buffer,
            &restored_document
        ),
        "restore publisher document"
    );

    require_body(
        &restored_buffer,
        "hello from the editor\nsecond line",
        "restored body remains in REU"
    );
    require(
        restored_buffer.gap_start == editor.buffer.gap_start
            && restored_buffer.gap_end == editor.buffer.gap_end
            && restored_buffer.cursor == editor.buffer.cursor,
        "restore exact gap and cursor state"
    );
    require(restored_document.type == S64_DOCUMENT_POST, "restore document type");
    require(restored_document.status == S64_DOCUMENT_DRAFT, "restore status");
    require(restored_document.order == -7, "restore order");
    require(strcmp(restored_document.slug, "hello-editor") == 0, "restore slug");
    require(strcmp(restored_document.title, "Hello Editor") == 0, "restore title");
    require(strcmp(restored_document.date, "2026-09-28") == 0, "restore date");
    require(strcmp(incoming.filename, filename) == 0, "restore local filename");
    require(
        (incoming.flags & S64_HANDOFF_FLAG_DIRTY) != 0u,
        "restore dirty flag"
    );
    require(
        (incoming.flags & S64_HANDOFF_FLAG_INSERT_MODE) == 0u,
        "restore overwrite flag"
    );
    require(
        (incoming.flags & S64_HANDOFF_FLAG_WORD_WRAP) == 0u,
        "restore horizontal flag"
    );
    require(
        incoming.history.count == editor.history.count
            && incoming.history.cursor == editor.history.cursor,
        "restore history ring counters"
    );

    fake_reu[S64_REU_HANDOFF_BANK][S64_REU_HANDOFF_BASE + 20u] ^= 0x01u;
    require(!s64_handoff_load(&incoming), "checksum rejects corrupt handoff");

    require(s64_handoff_store(&outgoing), "restore valid handoff");
    require(s64_handoff_clear(), "clear handoff");
    require(!s64_handoff_load(&incoming), "cleared handoff is invalid");

    require(
        fake_reu[S64_REU_HANDOFF_BANK][65535u] == after_handoff,
        "handoff record stays within reserved slot"
    );

    puts("publisher handoff tests passed");
    return 0;
}
