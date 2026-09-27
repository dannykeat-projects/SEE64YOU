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
    cputs("SEE64YOU REU DIAGNOSTIC\r\n\r\n");

    if (!s64_reu_probe()) {
        cputs("REU: NOT DETECTED\r\n");
        return 1;
    }

    banks = s64_reu_probe_banks();
    if (banks == 0u) {
        cputs("REU: DMA OK, SIZE UNKNOWN\r\n");
        return 2;
    }

    cputs("REU: OK\r\nSIZE: ");
    print_unsigned((unsigned int)(banks * 64u));
    cputs(" KIB\r\n");
    cputs("PROBE DATA RESTORED\r\n");
    return 0;
}
