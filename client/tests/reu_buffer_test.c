#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "platform/reu.h"

static unsigned char fake_reu[S64_BUFFER_CAPACITY];
static unsigned long reu_loads;
static unsigned long reu_stores;

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

int main(void)
{
    S64Buffer buffer;
    unsigned char readback[4];
    unsigned int i;

    memset(fake_reu, 0, sizeof(fake_reu));
    s64_buffer_init(&buffer);

    require(s64_buffer_length(&buffer) == 0u, "new REU buffer is empty");
    require(s64_buffer_free(&buffer) == S64_BUFFER_CAPACITY, "new REU buffer is fully free");
    require(s64_buffer_cursor(&buffer) == 0u, "new REU logical cursor starts at zero");

    require(s64_buffer_insert(&buffer, 'A'), "insert A");
    require(s64_buffer_insert(&buffer, 'B'), "insert B");
    require(s64_buffer_insert(&buffer, 'C'), "insert C");
    require(s64_buffer_length(&buffer) == 3u, "length after insert");
    require(s64_buffer_char_at(&buffer, 1u) == 'B', "cached char lookup");

    require(s64_buffer_move_cursor(&buffer, 1u), "move cursor into REU text");
    require(s64_buffer_insert(&buffer, 'X'), "insert in REU middle");
    require(s64_buffer_read(&buffer, 0u, readback, sizeof(readback)) == 4u, "read across REU gap");
    require(readback[0] == 'A' && readback[1] == 'X'
        && readback[2] == 'B' && readback[3] == 'C', "REU read preserves logical order");
    require(s64_buffer_find_byte(&buffer, 0u, 'B') == 2u, "find byte across REU gap");
    require(s64_buffer_find_byte(&buffer, 0u, 'Z') == 4u, "missing REU byte returns length");

    require(s64_buffer_backspace(&buffer), "REU backspace");
    require(s64_buffer_delete(&buffer), "REU forward delete");
    require(s64_buffer_length(&buffer) == 2u, "REU delete length");
    require(s64_buffer_char_at(&buffer, 0u) == 'A', "REU delete keeps prefix");
    require(s64_buffer_char_at(&buffer, 1u) == 'C', "REU delete keeps suffix");

    s64_buffer_init(&buffer);
    for (i = 0u; i < 300u; ++i) {
        require(
            s64_buffer_insert(&buffer, (unsigned char)('A' + (i % 26u))),
            "fill long REU fixture"
        );
    }

    {
        unsigned long loads_before = reu_loads;
        unsigned long stores_before = reu_stores;

        require(s64_buffer_move_cursor(&buffer, 0u), "move REU logical cursor to start");
        require(reu_loads == loads_before && reu_stores == stores_before,
            "logical cursor move performs no REU transfer");
    }
    require(s64_buffer_cursor(&buffer) == 0u, "logical cursor moves independently");
    require(buffer.gap_start == 300u, "navigation leaves REU gap in place");
    require(s64_buffer_char_at(&buffer, 0u) == 'A', "lazy move keeps first byte");
    require(s64_buffer_char_at(&buffer, 299u) == (unsigned char)('A' + (299u % 26u)), "lazy move keeps last byte");
    require(s64_buffer_move_cursor(&buffer, 300u), "move REU logical cursor to end");
    require(buffer.gap_start == 300u, "second navigation still leaves REU gap in place");
    require(s64_buffer_char_at(&buffer, 129u) == (unsigned char)('A' + (129u % 26u)), "lazy cursor round trip keeps middle byte");

    require(s64_buffer_delete_range(&buffer, 100u, 200u), "REU delete range");
    require(s64_buffer_length(&buffer) == 200u, "REU range deletion length");
    require(s64_buffer_char_at(&buffer, 99u) == (unsigned char)('A' + (99u % 26u)), "REU range keeps prefix");
    require(s64_buffer_char_at(&buffer, 100u) == (unsigned char)('A' + (200u % 26u)), "REU range keeps suffix");

    /*
     * Exercise overlapping DMA moves with an almost-full buffer. The physical
     * source and destination ranges overlap when the gap is smaller than the
     * transfer window, so load-before-store ordering must preserve the text.
     */
    s64_buffer_init(&buffer);
    buffer.gap_start = 100u;
    buffer.gap_end = 108u;
    buffer.cursor = 100u;
    for (i = 0u; i < 100u; ++i) {
        fake_reu[i] = (unsigned char)((i % 251u) + 1u);
    }
    for (i = 108u; i < S64_BUFFER_CAPACITY; ++i) {
        fake_reu[i] = (unsigned char)(((i - 8u) % 251u) + 1u);
    }

    require(s64_buffer_move_cursor(&buffer, 260u), "move logical cursor across small REU gap");
    require(buffer.gap_start == 100u && buffer.gap_end == 108u, "logical move does not relocate small gap");
    require(s64_buffer_insert(&buffer, 0xfeu), "edit forces overlapping REU gap move right");
    require(buffer.gap_start == 261u && buffer.gap_end == 268u, "right edit relocates gap and consumes one byte");
    require(s64_buffer_char_at(&buffer, 259u) == (unsigned char)((259u % 251u) + 1u), "right overlap keeps moved tail");
    require(s64_buffer_char_at(&buffer, 260u) == 0xfeu, "right overlap inserts at logical cursor");
    require(s64_buffer_char_at(&buffer, 261u) == (unsigned char)((260u % 251u) + 1u), "right overlap keeps suffix after insertion");

    require(s64_buffer_move_cursor(&buffer, 40u), "move logical cursor left without relocating gap");
    require(buffer.gap_start == 261u && buffer.gap_end == 268u, "left logical move leaves gap in place");
    require(s64_buffer_insert(&buffer, 0xfdu), "edit forces overlapping REU gap move left");
    require(buffer.gap_start == 41u && buffer.gap_end == 47u, "left edit relocates gap and consumes one byte");
    require(s64_buffer_char_at(&buffer, 39u) == (unsigned char)((39u % 251u) + 1u), "left overlap keeps prefix");
    require(s64_buffer_char_at(&buffer, 40u) == 0xfdu, "left overlap inserts at logical cursor");
    require(s64_buffer_char_at(&buffer, 41u) == (unsigned char)((40u % 251u) + 1u), "left overlap keeps following byte");
    require(s64_buffer_char_at(&buffer, 261u) == 0xfeu, "left overlap preserves earlier inserted marker");

    puts("REU buffer tests passed");
    return 0;
}
