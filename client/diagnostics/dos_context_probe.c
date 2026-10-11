#include <stdint.h>

#include "platform/dos.h"
#include "platform/package_paths.h"
#include "platform/uci.h"

/* Read-only hardware probe. Requires the existing application-folder config.
 * $C040 = $A5 on success; $C041 reports the failed stage otherwise.
 * No credentials are read and no SD files or turbo settings are written. */
#define RESULT (*(volatile uint8_t*)0xC040u)
#define DETAIL (*(volatile uint8_t*)0xC041u)

int main(void)
{
    S64Dos primary;
    S64Dos secondary;
    uint8_t exists;

    RESULT = 0u;
    DETAIL = 1u;
    s64_dos_init(&primary, s64_uci_execute, S64_DOS_TARGET_PRIMARY);
    s64_dos_init(&secondary, s64_uci_execute, S64_DOS_TARGET_SECONDARY);
    if (s64_dos_change_dir(&secondary, S64_PACKAGE_DIRECTORY) != S64_DOS_OK)
        return 1;
    DETAIL = 2u;
    if (s64_dos_file_exists(&secondary, "SEE64YOU.CFG", &exists)
            != S64_DOS_OK || !exists) return 1;
    DETAIL = 3u;
    if (s64_dos_change_dir(&primary, S64_SD_ROOT_DIRECTORY) != S64_DOS_OK)
        return 1;
    DETAIL = 4u;
    if (s64_dos_file_exists(&secondary, "SEE64YOU.CFG", &exists)
            != S64_DOS_OK || !exists) return 1;
    DETAIL = 5u;
    if (s64_dos_open_read(&secondary,
            S64_PACKAGE_DIRECTORY "SEE64YOU.CFG") != S64_DOS_OK) return 1;
    DETAIL = 6u;
    if (s64_dos_close(&secondary) != S64_DOS_OK) return 1;
    DETAIL = 7u;
    if (s64_dos_open_read(&secondary, "SEE64YOU.CFG") != S64_DOS_OK)
        return 1;
    DETAIL = 8u;
    if (s64_dos_close(&secondary) != S64_DOS_OK) return 1;
    DETAIL = 0u;
    RESULT = 0xA5u;
    return 0;
}
