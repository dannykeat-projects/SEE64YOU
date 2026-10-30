#include <6502.h>
#include <stdint.h>
#include "input/input.h"

#define BYTE(address) (*(volatile uint8_t*)(address))

static void check(uint8_t key, uint8_t modifiers, S64InputType expected,
    uint8_t detail)
{
    S64InputEvent event;
    BYTE(0x028D) = modifiers;
    BYTE(0x00CB) = 0x40u;
    BYTE(0x0277) = key;
    BYTE(0x00C6) = 1u;
    if (!s64_input_read(&event, 1u) || event.type != expected) {
        BYTE(0xC062) = detail;
        BYTE(0xC061) = 0x5Au;
        for (;;) {}
    }
    BYTE(0xC060) = detail;
}

int main(void)
{
    /* Freeze the scanner while testing the production decoder with KERNAL
     * output bytes. Physical modifier timing remains a hardware gate. */
    SEI();
    BYTE(0xC060) = BYTE(0xC061) = BYTE(0xC062) = 0u;
    check(0xA7u, 2u, S64_INPUT_MENU, 1u); /* Commodore+M */
    check(0x89u, 2u, S64_INPUT_NEW, 2u);
    check(0x8Au, 2u, S64_INPUT_OPEN, 3u);
    check(0x8Bu, 2u, S64_INPUT_SAVE, 4u);
    check(0x8Cu, 2u, S64_INPUT_SAVE_AS, 5u);
    check(0x89u, 0u, S64_INPUT_FIND_NEXT, 6u);
    check(0x8Au, 0u, S64_INPUT_FIND_PREVIOUS, 7u);
    check(0x8Bu, 0u, S64_INPUT_REPLACE, 8u);
    check(0x8Cu, 0u, S64_INPUT_HELP, 9u);
    check(0x85u, 0u, S64_INPUT_TOGGLE_MARK, 10u);
    check(0x86u, 0u, S64_INPUT_COPY, 11u);
    check(0x87u, 0u, S64_INPUT_CUT, 12u);
    check(0x88u, 0u, S64_INPUT_PASTE, 13u);
    check(0x4Du, 2u, S64_INPUT_MENU, 14u);
    check(0x85u, 2u, S64_INPUT_NEW, 15u);
    check(0x86u, 2u, S64_INPUT_OPEN, 16u);
    check(0x87u, 2u, S64_INPUT_SAVE, 17u);
    check(0x88u, 2u, S64_INPUT_SAVE_AS, 18u);
    BYTE(0xC060) = 0x80u;
    BYTE(0xC061) = 0xA5u;
    for (;;) {}
    return 0;
}
