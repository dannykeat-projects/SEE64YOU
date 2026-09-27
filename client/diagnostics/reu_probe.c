#include <conio.h>
#include <stdint.h>

#include "platform/reu.h"

#ifdef S64_VICE_DEBUG_EXIT
#define S64_VICE_DEBUG_PORT (*(volatile uint8_t*)0xD7FFu)
#endif

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

static uint8_t bank_result_code(uint16_t banks)
{
    switch (banks) {
        case 2u: return 1u;   /* 128 KiB */
        case 4u: return 2u;   /* 256 KiB */
        case 8u: return 3u;   /* 512 KiB */
        case 16u: return 4u;  /* 1 MiB */
        case 32u: return 5u;  /* 2 MiB */
        case 64u: return 6u;  /* 4 MiB */
        case 128u: return 7u; /* 8 MiB */
        case 256u: return 8u; /* 16 MiB */
        default: return 9u;
    }
}

static void finish_diagnostic(uint8_t code)
{
#ifdef S64_VICE_DEBUG_EXIT
    S64_VICE_DEBUG_PORT = code;
    for (;;) {
    }
#else
    (void)code;
#endif
}

int main(void)
{
    uint16_t banks;
    uint8_t result;

#ifdef S64_VICE_DEBUG_SMOKE
    finish_diagnostic(11u);
#endif

    clrscr();
    cputs("SEE64YOU REU DIAGNOSTIC\r\n\r\n");

    if (!s64_reu_probe()) {
        cputs("REU: NOT DETECTED\r\n");
        finish_diagnostic(10u);
        return 1;
    }

    banks = s64_reu_probe_banks();
    result = bank_result_code(banks);
    if (result == 9u) {
        cputs("REU: DMA OK, SIZE UNKNOWN\r\n");
        finish_diagnostic(result);
        return 2;
    }

    cputs("REU: OK\r\nSIZE: ");
    print_unsigned((unsigned int)(banks * 64u));
    cputs(" KIB\r\n");
    cputs("PROBE DATA RESTORED\r\n");

    finish_diagnostic(result);
    return 0;
}
