#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "charset.h"

static void test_printable_ascii_round_trip(void)
{
    uint16_t value;

    for (value = 0x20u; value <= 0x7Eu; ++value) {
        uint8_t petscii = 0u;
        uint8_t ascii = 0u;

        assert(s64_charset_ascii_to_petscii((uint8_t)value, &petscii));
        assert(s64_charset_petscii_to_ascii(petscii, &ascii));
        assert(ascii == (uint8_t)value);
    }
}

static void test_letter_ranges(void)
{
    uint8_t value;

    assert(s64_charset_ascii_to_petscii(0x41u, &value) && value == 0xC1u);
    assert(s64_charset_ascii_to_petscii(0x5Au, &value) && value == 0xDAu);
    assert(s64_charset_ascii_to_petscii(0x61u, &value) && value == 0x41u);
    assert(s64_charset_ascii_to_petscii(0x7Au, &value) && value == 0x5Au);

    assert(s64_charset_petscii_to_ascii(0xC1u, &value) && value == 0x41u);
    assert(s64_charset_petscii_to_ascii(0xDAu, &value) && value == 0x5Au);
    assert(s64_charset_petscii_to_ascii(0x41u, &value) && value == 0x61u);
    assert(s64_charset_petscii_to_ascii(0x5Au, &value) && value == 0x7Au);
}

static void test_shifted_punctuation(void)
{
    static const uint8_t ascii_values[] = {
        0x5Cu, 0x5Fu, 0x60u, 0x7Bu, 0x7Cu, 0x7Du, 0x7Eu
    };
    static const uint8_t petscii_values[] = {
        0xBFu, 0xA4u, 0xADu, 0xB3u, 0xDDu, 0xABu, 0xB1u
    };
    uint8_t i;

    for (i = 0u; i < sizeof(ascii_values); ++i) {
        uint8_t value = 0u;

        assert(s64_charset_ascii_to_petscii(ascii_values[i], &value));
        assert(value == petscii_values[i]);
        assert(s64_charset_petscii_to_ascii(petscii_values[i], &value));
        assert(value == ascii_values[i]);
    }
}

static void test_newline_and_rejections(void)
{
    uint8_t value = 0u;

    assert(s64_charset_ascii_to_petscii(0x0Au, &value) && value == 0x0Du);
    assert(s64_charset_petscii_to_ascii(0x0Du, &value) && value == 0x0Au);
    assert(!s64_charset_petscii_to_ascii(0x5Cu, &value));
    assert(!s64_charset_petscii_to_ascii(0x5Fu, &value));
    assert(!s64_charset_petscii_to_ascii(0x60u, &value));
    assert(!s64_charset_petscii_to_ascii(0x7Fu, &value));
    assert(!s64_charset_ascii_to_petscii(0x80u, &value));
}

int main(void)
{
    test_printable_ascii_round_trip();
    test_letter_ranges();
    test_shifted_punctuation();
    test_newline_and_rejections();
    puts("charset tests passed");
    return 0;
}
