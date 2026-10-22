#include <conio.h>
#include <stdint.h>
#include <string.h>

#include "platform/companion_loader.h"
#include "platform/companion_resident.h"
#include "platform/dos.h"
#include "platform/uci.h"

static S64Dos dos;

int main(void)
{
    S64CompanionImage image;
    S64CompanionResult result;
    volatile uint8_t* resident = (volatile uint8_t*)S64_RESIDENT_LOADER_BASE;

    bgcolor(COLOR_BLUE);
    bordercolor(COLOR_BLUE);
    textcolor(COLOR_LIGHTBLUE);
    clrscr();
    cursor(0);
    cputs("SEE64YOU RESIDENT LOAD PROBE\r\n\r\n");
    cputs("THIS REPLACES THE RUNNING PRG.\r\n");
    cputs("USE ONLY WITH THE MATCHING PACKAGE.\r\n\r\n");
    cputs("Y TO RUN; OTHER KEY EXITS.");
    if (cgetc() != 'y') return 0;

    clrscr();
    cputs("PREFLIGHTING S64LDTST.PRG...\r\n");
    if (!s64_uci_probe_target(S64_DOS_TARGET_PRIMARY)) {
        cputs("ULTIMATE DOS $01 UNAVAILABLE\r\n");
        cgetc();
        return 1;
    }

    s64_dos_init(&dos, s64_uci_execute, S64_DOS_TARGET_PRIMARY);
    result = s64_companion_open_named_preflight(
        &dos,
        S64_RESIDENT_TEST_FILENAME,
        &image
    );
    if (result != S64_COMPANION_OK) {
        cprintf("PREFLIGHT FAIL %u DOS %u\r\n", result, image.dos_result);
        cgetc();
        return 1;
    }

    memcpy(
        (void*)S64_RESIDENT_LOADER_BASE,
        s64_resident_loader_image,
        S64_RESIDENT_LOADER_SIZE
    );
    resident[S64_RESIDENT_PAYLOAD_LO_OFFSET] =
        (uint8_t)(image.payload_size & 0xffu);
    resident[S64_RESIDENT_PAYLOAD_HI_OFFSET] =
        (uint8_t)(image.payload_size >> 8);

    clrscr();
    cprintf("LOADING TEST $%04X-$%04X...\r\n",
        image.load_address, image.load_end);
    cputs("DO NOT RESET UNLESS RED BORDER/FAIL STICKS.");
    s64_resident_loader_enter();
    return 1;
}
