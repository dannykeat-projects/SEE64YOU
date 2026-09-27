#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "platform/reu.h"

static unsigned char fake_reu[S64_BUFFER_CAPACITY];

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

    require(s64_buffer_move_cursor(&buffer, 0u), "move REU gap left across multiple DMA chunks");
    require(s64_buffer_char_at(&buffer, 0u) == 'A', "long move keeps first byte");
    require(s64_buffer_char_at(&buffer, 299u) == (unsigned char)('A' + (299u % 26u)), "long move keeps last byte");
    require(s64_buffer_move_cursor(&buffer, 300u), "move REU gap right across multiple DMA chunks");
    require(s64_buffer_char_at(&buffer, 129u) == (unsigned char)('A' + (129u % 26u)), "round-trip move keeps middle byte");

    require(s64_buffer_delete_range(&buffer, 100u, 200u), "REU delete range");
    require(s64_buffer_length(&buffer) == 200u, "REU range deletion length");
    require(s64_buffer_char_at(&buffer, 99u) == (unsigned char)('A' + (99u % 26u)), "REU range keeps prefix");
    require(s64_buffer_char_at(&buffer, 100u) == (unsigned char)('A' + (200u % 26u)), "REU range keeps suffix");

    puts("REU buffer tests passed");
    return 0;
}
