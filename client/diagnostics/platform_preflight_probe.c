#include <conio.h>

#include "platform/preflight.h"

static void show_requirement(S64PlatformRequirement requirement)
{
    clrscr();
    cputs("SEE64YOU PLATFORM CHECK\r\n\r\n");

    switch (requirement) {
        case S64_PLATFORM_MISSING_UCI:
            cputs("COMMAND INTERFACE REQUIRED\r\n\r\n");
            cputs("OPEN ULTIMATE MENU\r\n");
            cputs("F2 SETTINGS\r\n");
            cputs("C64 AND CARTRIDGE SETTINGS\r\n");
            cputs("COMMAND INTERFACE: ENABLED\r\n");
            break;
        case S64_PLATFORM_MISSING_DOS:
            cputs("ULTIMATE-DOS REQUIRED\r\n\r\n");
            cputs("COMMAND INTERFACE IS READY\r\n");
            cputs("CHECK ULTIMATE FIRMWARE\r\n");
            break;
        case S64_PLATFORM_MISSING_REU:
            cputs("16 MB REU REQUIRED\r\n\r\n");
            cputs("OPEN ULTIMATE MENU\r\n");
            cputs("F2 SETTINGS\r\n");
            cputs("C64 AND CARTRIDGE SETTINGS\r\n");
            cputs("RAM EXPANSION UNIT: ENABLED\r\n");
            cputs("REU SIZE: 16 MB\r\n");
            break;
        case S64_PLATFORM_WRONG_REU_SIZE:
            cputs("REU SIZE MUST BE 16 MB\r\n\r\n");
            cputs("OPEN ULTIMATE MENU\r\n");
            cputs("F2 SETTINGS\r\n");
            cputs("C64 AND CARTRIDGE SETTINGS\r\n");
            cputs("REU SIZE: 16 MB\r\n");
            break;
        default:
            break;
    }

    cputs("\r\nR RECHECK    X EXIT");
}

static void show_storage_requirement(void)
{
    clrscr();
    cputs("SEE64YOU PLATFORM CHECK\r\n\r\n");
    cputs("SD STORAGE NOT READY\r\n\r\n");
    cputs("CHECK SD CARD IS INSERTED\r\n");
    cputs("AND WRITABLE, THEN RECHECK\r\n");
    cputs("\r\nR RECHECK    X EXIT");
}

int main(void)
{
    S64PlatformPreflight preflight;
    unsigned char key;

    for (;;) {
        s64_platform_preflight_run(&preflight);
        if (preflight.requirement == S64_PLATFORM_READY
            && preflight.storage_ready) {
            clrscr();
            cputs("SEE64YOU PLATFORM CHECK\r\n\r\n");
            cputs("COMMAND INTERFACE READY\r\n");
            cputs("ULTIMATE-DOS       READY\r\n");
            cputs("MEMORY 16 MB       READY\r\n");
            cputs("STORAGE            READY\r\n");
            cputs("\r\nPLATFORM READY\r\n");
            return 0;
        }

        if (preflight.requirement == S64_PLATFORM_READY) {
            show_storage_requirement();
        } else {
            show_requirement(preflight.requirement);
        }

        for (;;) {
            key = (unsigned char)cgetc();
            if (key == 'R' || key == 'r') break;
            if (key == 'X' || key == 'x') return 1;
        }
    }
}
