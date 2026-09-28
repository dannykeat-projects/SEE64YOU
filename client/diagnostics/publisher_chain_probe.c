#include <conio.h>

#include "platform/chain.h"
#include "platform/reu.h"
#include "publisher/handoff.h"

static S64PublisherHandoff handoff;

int main(void)
{
    bgcolor(COLOR_BLUE);
    bordercolor(COLOR_BLUE);
    textcolor(COLOR_LIGHTBLUE);
    clrscr();
    cursor(0);

    if (!s64_reu_probe() || s64_reu_probe_banks() < 2u) {
        cputs("CHAIN TEST: REU REQUIRED\r\n");
        return 1;
    }

    s64_handoff_init(&handoff);
    handoff.flags = S64_HANDOFF_FLAG_INSERT_MODE
        | S64_HANDOFF_FLAG_WORD_WRAP;

    if (!s64_handoff_store(&handoff)) {
        cputs("CHAIN TEST: HANDOFF FAILED\r\n");
        return 1;
    }

    cputs("CHAIN TEST: STARTING PUBLISHER\r\n");
    s64_chain_publisher();

    cputs("CHAIN TEST: RETURNED UNEXPECTEDLY\r\n");
    return 1;
}
