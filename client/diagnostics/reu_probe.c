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

    DIAG_PROGRESS = 0x01u;
    clrscr();
    DIAG_PROGRESS = 0x02u;
    gotoxy(0u, 0u);
    cputs("SEE64YOU REU TEST");
    DIAG_PROGRESS = 0x03u;
    gotoxy(0u, 2u);

    DIAG_PROGRESS = 0x10u;
    dma_scratch = 0x5Au;
    cputs("STORE...");
    if (!s64_reu_store(0u, 0u, &dma_scratch, 1u)) {
        cputs("FAIL\r\n");
        for (;;) {
        }
    }
    DIAG_PROGRESS = 0x11u;
    cputs("OK");
    gotoxy(0u, 3u);

    dma_scratch = 0u;
    DIAG_PROGRESS = 0x12u;
    cputs("LOAD....");
    if (!s64_reu_load(0u, 0u, &dma_scratch, 1u)) {
        cputs("FAIL\r\n");
        for (;;) {
        }
    }
    DIAG_PROGRESS = 0x13u;
    cputs("OK");
    gotoxy(0u, 4u);

    if (dma_scratch != 0x5Au) {
        cputs("ROUNDTRIP FAIL");
        for (;;) {
        }
    }
    DIAG_PROGRESS = 0x14u;
    cputs("DMA ROUNDTRIP OK");
    gotoxy(0u, 5u);

    DIAG_PROGRESS = 0x20u;
    if (!s64_reu_probe()) {
        cputs("REU: NOT DETECTED");
        gotoxy(0u, 6u);
        cputs("FAIL");
        return 1;
    }

    DIAG_PROGRESS = 0x21u;
    DIAG_PROGRESS = 0x30u;
    banks = s64_reu_probe_banks();
    if (banks == 0u) {
        cputs("REU: SIZE UNKNOWN");
        gotoxy(0u, 6u);
        cputs("FAIL");
        return 2;
    }

    DIAG_PROGRESS = 0x31u;
    gotoxy(0u, 5u);
    cputs("BANKS: ");
    print_unsigned(banks);
    gotoxy(0u, 6u);
    cputs("SIZE: ");
    print_unsigned((unsigned int)(banks * 64u));
    cputs(" KIB");
    gotoxy(0u, 7u);
    cputs("PASS");

    for (;;) {
    }
}
