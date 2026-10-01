#include <stdint.h>

#define SCREEN ((volatile uint8_t*)0x0400u)
#define COLOR ((volatile uint8_t*)0xd800u)
#define BORDER (*(volatile uint8_t*)0xd020u)
#define BACKGROUND (*(volatile uint8_t*)0xd021u)
#define TARGET_MARKER (*(volatile uint8_t*)0x02a7u)

int main(void)
{
    static const uint8_t message[] = {
        0x14u, 0x01u, 0x12u, 0x07u, 0x05u, 0x14u,
        0x20u, 0x12u, 0x01u, 0x0eu
    };
    uint8_t i;

    for (i = 0u; i < (uint8_t)sizeof(message); ++i) {
        SCREEN[i] = message[i];
        COLOR[i] = 1u;
    }
    BORDER = 6u;
    BACKGROUND = 0u;
    TARGET_MARKER = 0x5au;
    return 0;
}
