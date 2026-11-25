#include <conio.h>
#include "ui.h"

/* The managed build puts this blocking prompt in MODEL to preserve resident
 * headroom; FILE has no room for another prompt. Document actions stay resident. */
unsigned char s64_ui_prompt_exit(void)
{
    unsigned char key;

    for (;;) {
        key = (unsigned char)cgetc();
        if (key == CH_STOP) return key;
        if (key == 'S' || key == 'D') key += 'a' - 'A';
        if (key == 's' || key == 'd') {
            return key;
        }
    }
}

unsigned char s64_ui_recovery_choice(void)
{
    unsigned char key;
    gotoxy(0u, 24u);
    cputs("Recovery: Y open  D discard  STOP keep ");
    for (;;) {
        key = cgetc();
        if (key == 'y' || key == 'Y') return 'y';
        if (key == 'd' || key == 'D') return 'd';
        if (key == CH_STOP || key == 'n' || key == 'N') return CH_STOP;
    }
}
