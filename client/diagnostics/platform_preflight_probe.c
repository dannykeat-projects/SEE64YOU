#include <conio.h>
#include <stdint.h>

#include "platform/requirements.h"
#include "platform/reu.h"
#include "platform/uci.h"

static S64PlatformRequirement check_platform(void)
{
    S64UciCapabilities capabilities;
    S64PlatformRequirements requirements;

    s64_uci_probe_capabilities(&capabilities);
    requirements.interface_present = capabilities.interface_present;
    requirements.dos_target = capabilities.dos_target;
    requirements.reu_present = s64_reu_probe();
    requirements.reu_banks = requirements.reu_present
        ? s64_reu_probe_banks()
        : 0u;

    return s64_platform_check_requirements(&requirements);
}

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

int main(void)
{
    S64PlatformRequirement requirement;
    unsigned char key;

    for (;;) {
        requirement = check_platform();
        if (requirement == S64_PLATFORM_READY) {
            clrscr();
            cputs("SEE64YOU PLATFORM CHECK\r\n\r\n");
            cputs("COMMAND INTERFACE READY\r\n");
            cputs("ULTIMATE-DOS       READY\r\n");
            cputs("MEMORY 16 MB       READY\r\n");
            cputs("\r\nPLATFORM READY\r\n");
            return 0;
        }

        show_requirement(requirement);
        for (;;) {
            key = (unsigned char)cgetc();
            if (key == 'R' || key == 'r') break;
            if (key == 'X' || key == 'x') return 1;
        }
    }
}
