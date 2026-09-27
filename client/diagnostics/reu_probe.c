#include <conio.h>
#include <stdint.h>

#include "platform/reu.h"

static void print_unsigned(unsigned int value)
{
    char digits[6];
    unsigned char length = 0u;

    do {
        digits[length++] = (char)('0' + (value % 10u));
        value /= 10u;
    } while (value != 0u);

    while (length != 0u) {
        cputc(digits[--length]);
    }
}

int main(void)
{
    uint16_t banks;

    clrscr();
    cputs("SEE64YOU REU TEST\r\n\r\n");

    if (!s64_reu_probe()) {
        cputs("REU: NOT DETECTED\r\nFAIL\r\n");
        return 1;
    }

    banks = s64_reu_probe_banks();
    if (banks == 0u) {
        cputs("REU: SIZE UNKNOWN\r\nFAIL\r\n");
        return 2;
    }

    cputs("BANKS: ");
    print_unsigned(banks);
    cputs("\r\nSIZE: ");
    print_unsigned((unsigned int)(banks * 64u));
    cputs(" KIB\r\nPASS\r\n");

    for (;;) {
    }
}
