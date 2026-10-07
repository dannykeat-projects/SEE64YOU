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
