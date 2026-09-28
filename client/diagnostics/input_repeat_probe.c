#include <stdint.h>

#include "input/input.h"

#define SHFLAG (*(volatile uint8_t*)0x028Du)
#define RPTFLG (*(volatile uint8_t*)0x028Au)
#define DIAG_PROGRESS (*(volatile uint8_t*)0xC060u)
#define DIAG_RESULT (*(volatile uint8_t*)0xC061u)
#define DIAG_DETAIL (*(volatile uint8_t*)0xC062u)

#define DIAG_PASS 0xA5u
#define DIAG_FAIL 0x5Au

static void fail(uint8_t detail)
{
    DIAG_DETAIL = detail;
    DIAG_RESULT = DIAG_FAIL;
    for (;;) {
    }
}

static void require_repeat(uint8_t modifiers, uint8_t expected, uint8_t detail)
{
    SHFLAG = modifiers;
    s64_input_sync_repeat();
    if (RPTFLG != expected) {
        fail(detail);
    }
    DIAG_PROGRESS = detail;
}

int main(void)
{
    DIAG_PROGRESS = 0u;
    DIAG_RESULT = 0u;
    DIAG_DETAIL = 0u;

    require_repeat(0x00u, 0x80u, 1u);
    require_repeat(0x01u, 0x80u, 2u);
    require_repeat(0x02u, 0x40u, 3u);
    require_repeat(0x04u, 0x40u, 4u);
    require_repeat(0x06u, 0x40u, 5u);

    s64_input_restore_repeat();
    if (RPTFLG != 0x00u) {
        fail(6u);
    }

    SHFLAG = 0u;
    DIAG_PROGRESS = 0x80u;
    DIAG_RESULT = DIAG_PASS;
    for (;;) {
    }
}
