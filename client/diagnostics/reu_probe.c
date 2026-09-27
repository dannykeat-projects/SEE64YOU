#include <conio.h>
#include <stdint.h>

#include "platform/reu.h"

static uint8_t dma_scratch;
#define DIAG_PROGRESS (*(volatile uint8_t*)0xC000u)

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

    DIAG_PROGRESS = 0x10u;
    dma_scratch = 0x5Au;
    cputs("STORE...");
    if (!s64_reu_store(0u, 0u, &dma_scratch, 1u)) {
        cputs("FAIL\r\n");
        for (;;) {
        }
    }
    DIAG_PROGRESS = 0x11u;
    cputs("OK\r\n");

    dma_scratch = 0u;
    DIAG_PROGRESS = 0x12u;
    cputs("LOAD....");
    if (!s64_reu_load(0u, 0u, &dma_scratch, 1u)) {
        cputs("FAIL\r\n");
        for (;;) {
        }
    }
    DIAG_PROGRESS = 0x13u;
    cputs("OK\r\n");

    if (dma_scratch != 0x5Au) {
        cputs("ROUNDTRIP FAIL\r\n");
        for (;;) {
        }
    }
    DIAG_PROGRESS = 0x14u;
    cputs("DMA ROUNDTRIP OK\r\n");

    DIAG_PROGRESS = 0x20u;
    if (!s64_reu_probe()) {
        cputs("REU: NOT DETECTED\r\nFAIL\r\n");
        return 1;
    }

    DIAG_PROGRESS = 0x21u;
    DIAG_PROGRESS = 0x30u;
    banks = s64_reu_probe_banks();
    if (banks == 0u) {
        cputs("REU: SIZE UNKNOWN\r\nFAIL\r\n");
        return 2;
    }

    DIAG_PROGRESS = 0x31u;
    cputs("BANKS: ");
    print_unsigned(banks);
    cputs("\r\nSIZE: ");
    print_unsigned((unsigned int)(banks * 64u));
    cputs(" KIB\r\nPASS\r\n");

    for (;;) {
    }
}
