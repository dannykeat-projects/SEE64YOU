#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "editor.h"
#include "platform/reu.h"

/*
 * Aligned REU read window: how many transfers a scan needs, plus the safety
 * invariants that make the reuse correct.  The fake-REU harness matches
 * tests/reu_buffer_test.c; the counters let this fixture speak about window
 * reuse instead of only about bytes.  Baseline behaviour (an unaligned window
 * that starts at the requested byte) reloads on every step of a backward scan,
 * so the two "one transfer covers" assertions fail there and pass here.
 */

static unsigned char fake_reu[S64_BUFFER_CAPACITY];
static unsigned char text[200];
static unsigned long reu_loads;
static unsigned long reu_stores;
static unsigned long fail_load;

uint8_t s64_reu_probe(void)
{
    return 1u;
}

uint16_t s64_reu_probe_banks(void)
{
    return 2u;
}

uint8_t s64_reu_store(
    uint8_t bank,
    uint16_t address,
    const void* source,
    uint16_t length
)
{
    ++reu_stores;
    if (bank != 0u || source == 0
        || (unsigned long)address + length > S64_BUFFER_CAPACITY) {
        return 0u;
    }

    memcpy(&fake_reu[address], source, length);
    return 1u;
}

uint8_t s64_reu_load(
    uint8_t bank,
    uint16_t address,
    void* destination,
    uint16_t length
)
{
    ++reu_loads;
    if (reu_loads == fail_load) {
        return 0u;
    }
    if (bank != 0u || destination == 0
        || (unsigned long)address + length > S64_BUFFER_CAPACITY) {
        return 0u;
    }

    memcpy(destination, &fake_reu[address], length);
    return 1u;
}

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void expect_document(const unsigned char* bytes, unsigned int length)
{
    unsigned int i;

    for (i = 0u; i < length; ++i) {
        if (i == 100u) {
            require(bytes[i] == 'X', "saved fixture keeps the inserted byte");
        } else if (i < 100u) {
            require(bytes[i] == text[i], "saved fixture keeps the appended text");
        } else {
            require(bytes[i] == text[i - 1u],
                "saved fixture shifts text past the gap");
        }
    }
}

int main(void)
{
    S64Buffer buffer;
    S64Buffer fresh;
    static unsigned char saved[S64_BUFFER_CAPACITY];
    static unsigned char reopened[S64_BUFFER_CAPACITY];
    unsigned int i;
    unsigned int length;
    unsigned long loads_before;
    unsigned long stores_before;

    memset(fake_reu, 0, sizeof fake_reu);
    for (i = 0u; i < 200u; ++i) {
        text[i] = (unsigned char)('a' + (i % 26u));
    }

    /* ---------- one window on each side of a gap that is not at EOF ---------- */
    s64_buffer_init(&buffer);
    require(s64_buffer_append(&buffer, text, 200u) == 1,
        "append 200 bytes into the EOF gap");
    require(buffer.gap_start == 200u
        && buffer.gap_end == S64_BUFFER_CAPACITY,
        "append leaves the gap at EOF");
    require(s64_buffer_move_cursor(&buffer, 100u) == 1,
        "logical cursor moves to 100");
    require(s64_buffer_insert(&buffer, 'X') == 1,
        "insert moves the gap to the cursor");
    require(buffer.gap_start == 101u
        && buffer.gap_end == (unsigned int)(S64_BUFFER_CAPACITY - 100u),
        "the gap sits inside the document");
    length = s64_buffer_length(&buffer);
    require(length == 201u, "the document is 201 bytes after the insert");
    require(s64_buffer_char_at(&buffer, 100u) == 'X',
        "the inserted byte reads at the gap edge");

    /* The left half of one aligned block already covers the gap edge, so the
     * whole backward walk to the block start must need no further transfer. */
    loads_before = reu_loads;
    for (i = 99u; i >= 64u; --i) {
        require(s64_buffer_char_at(&buffer, i) == text[i],
            "left-side backward scan stays byte exact");
    }
    require(reu_loads == loads_before,
        "one aligned transfer serves the left-side scan to the block start");

    /* The right half of the gap must not reuse the left-side window, whose
     * upper half sits inside the gap and holds no document bytes. */
    loads_before = reu_loads;
    require(s64_buffer_char_at(&buffer, 101u) == text[100],
        "the first byte right of the gap reads through the gap offset");
    require(reu_loads == loads_before + 1u,
        "the right side of the gap reloads rather than reusing stale bytes");

    /* The document tail sits in the last aligned block of REU memory, so the
     * final byte and a 64-position backward scan share a single transfer. */
    require(s64_buffer_char_at(&buffer, 200u) == text[199],
        "the final byte reads at the last aligned block");
    loads_before = reu_loads;
    for (i = 200u; i >= 137u; --i) {
        require(s64_buffer_char_at(&buffer, i) == text[i - 1u],
            "right-side backward scan stays byte exact");
    }
    require(reu_loads == loads_before,
        "no extra transfer covers the 64-position backward scan on the document tail");

    /* ---------- edits invalidate the cached window ---------- */
    require(s64_buffer_move_cursor(&buffer, 90u) == 1,
        "cursor moves to 90");
    require(s64_buffer_insert(&buffer, 'Y') == 1,
        "insert inside the cached block");
    require(s64_buffer_char_at(&buffer, 90u) == 'Y',
        "an insert invalidates the cached window");
    require(s64_buffer_move_cursor(&buffer, 91u) == 1,
        "cursor moves after the inserted byte");
    require(s64_buffer_backspace(&buffer) == 1,
        "backspace removes the inserted byte");
    require(s64_buffer_length(&buffer) == 201u,
        "backspace plus insert leaves the document the same length");
    require(s64_buffer_char_at(&buffer, 90u) == text[90],
        "a deletion invalidates the cached window");

    /* ---------- a failed transfer latches and blocks further use ---------- */
    stores_before = reu_stores;
    fail_load = reu_loads + 1u;
    require(s64_buffer_char_at(&buffer, 20u) == 0u,
        "a failed REU read yields no byte");
    require(s64_buffer_read_error(&buffer) != 0u,
        "the failed REU read is latched");
    require(s64_buffer_insert(&buffer, 'Z') == 0,
        "editing is refused while the document read failed");
    require(s64_buffer_append(&buffer, text, 1u) == 0,
        "appending is refused while the document read failed");
    require(reu_stores == stores_before,
        "the latched failure performs no further REU write");
    fail_load = 0ul;
    s64_buffer_init(&buffer);
    require(s64_buffer_read_error(&buffer) == 0u,
        "initializing a document clears the latch");

    /* ---------- byte-exact save and reopen through the gap ---------- */
    s64_buffer_init(&buffer);
    require(s64_buffer_append(&buffer, text, 200u) == 1,
        "rebuild the document for the save round trip");
    require(s64_buffer_move_cursor(&buffer, 100u) == 1
        && s64_buffer_insert(&buffer, 'X') == 1,
        "save fixture keeps a gap that is not at EOF");
    require(s64_buffer_read(&buffer, 0u, saved, S64_BUFFER_CAPACITY) == 201u,
        "serialization reads the whole document across the gap");
    expect_document(saved, 201u);
    s64_buffer_init(&fresh);
    require(s64_buffer_append(&fresh, saved, 201u) == 1,
        "reopen the saved bytes");
    require(s64_buffer_read(&fresh, 0u, reopened, S64_BUFFER_CAPACITY) == 201u,
        "the reopened document serializes at the same length");
    require(memcmp(saved, reopened, 201u) == 0,
        "the reopened document is byte exact");

    /* ---------- Editor-level typing after a deletion ----------
     * The reported post-delete lag was not reproduced by the host models, so
     * this fixture only pins that typing after a deletion still yields the
     * same document as a fresh typing pass, and that the transfers stay few. */
    {
        static const unsigned char typed[] = "alpha beta gamma delta";
        S64Editor editor;
        S64Editor fresh_editor;
        static unsigned char first[sizeof typed];
        static unsigned char second[sizeof typed];
        unsigned int size_a;
        unsigned int size_b;

        s64_editor_init(&editor);
        for (i = 0u; i + 1u < sizeof typed; ++i) {
            require(s64_editor_insert(&editor, typed[i]) == 1,
                "type the sentence");
        }
        for (i = 0u; i < 3u; ++i) {
            require(s64_editor_backspace(&editor) == 1,
                "delete the last three characters");
        }
        for (i = (unsigned int)(sizeof typed - 4u); i + 1u < sizeof typed; ++i) {
            require(s64_editor_insert(&editor, typed[i]) == 1,
                "retype the deleted characters");
        }

        loads_before = reu_loads;
        stores_before = reu_stores;
        size_a = s64_buffer_read(&editor.buffer, 0u, first, sizeof first);
        require(size_a == (unsigned int)(sizeof typed - 1u),
            "post-delete document keeps its length");
        require(reu_loads - loads_before <= 64u
            && reu_stores - stores_before <= 8u,
            "serializing the document stays within bounded transfers");

        s64_editor_init(&fresh_editor);
        for (i = 0u; i + 1u < sizeof typed; ++i) {
            require(s64_editor_insert(&fresh_editor, typed[i]) == 1,
                "type the same sentence again in a fresh document");
        }
        size_b = s64_buffer_read(&fresh_editor.buffer, 0u, second, sizeof second);
        require(size_b == size_a, "the fresh document has the same length");
        require(memcmp(first, second, size_a) == 0,
            "the post-delete document matches a freshly typed document byte for byte");
    }

    puts("REU aligned-window invariants and post-delete typing passed");
    return 0;
}
