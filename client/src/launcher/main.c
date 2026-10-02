#include <conio.h>

#include "launcher/recovery_flow.h"
#include "launcher/recovery_runtime.h"
#include "platform/preflight.h"

static void show_title(void)
{
    clrscr();
    cputs("SEE64YOU\r\n");
    cputs("C64 ULTIMATE\r\n\r\n");
}

static void show_diagnostics_placeholder(void)
{
    show_title();
    cputs("DIAGNOSTICS\r\n\r\n");
    cputs("INTEGRATED DIAGNOSTICS\r\n");
    cputs("NOT YET WIRED\r\n\r\n");
    cputs("PRESS ANY KEY");
    (void)cgetc();
}

static void show_preflight_problem(const S64PlatformPreflight* preflight)
{
    show_title();

    switch (preflight->requirement) {
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

        case S64_PLATFORM_READY:
            cputs("SD STORAGE NOT READY\r\n\r\n");
            cputs("CHECK SD CARD IS INSERTED\r\n");
            cputs("AND WRITABLE, THEN RECHECK\r\n");
            break;
    }

    cputs("\r\nR RECHECK    X EXIT");
}

static unsigned char run_preflight(void)
{
    S64PlatformPreflight preflight;
    unsigned char key;

    for (;;) {
        show_title();
        cputs("CHECKING PLATFORM...\r\n");
        s64_platform_preflight_run(&preflight);

        if (preflight.requirement == S64_PLATFORM_READY
            && preflight.storage_ready) {
            return 1u;
        }

        show_preflight_problem(&preflight);
        for (;;) {
            key = (unsigned char)cgetc();
            if (key == 'R' || key == 'r') break;
            if (key == 'X' || key == 'x') return 0u;
        }
    }
}

static unsigned char handle_recovery_blocked(
    S64LauncherRecoveryResult result
)
{
    unsigned char key;

    for (;;) {
        show_title();
        cputs(s64_launcher_recovery_message(result));
        cputs("\r\n\r\n");

        if (s64_launcher_recovery_retryable(result)) {
            cputs("R RETRY  D DIAGNOSTICS  X EXIT");
        } else {
            cputs("D DIAGNOSTICS  X EXIT");
        }

        key = (unsigned char)cgetc();
        if ((key == 'R' || key == 'r')
            && s64_launcher_recovery_retryable(result)) {
            return 1u;
        }
        if (key == 'D' || key == 'd') {
            show_diagnostics_placeholder();
            continue;
        }
        if (key == 'X' || key == 'x') {
            return 0u;
        }
    }
}

int main(void)
{
    S64LauncherRecoveryRuntime recovery_runtime;
    S64LauncherRecoveryResult result;

    if (!run_preflight()) {
        return 1;
    }

    show_title();
    cputs("CHECKING PREVIOUS SESSION...\r\n");
    s64_launcher_recovery_runtime_init_hardware(&recovery_runtime);

    for (;;) {
        result = s64_launcher_recovery_runtime_run(&recovery_runtime);

        if (result == S64_LAUNCHER_RECOVERY_READY_NONE
            || result == S64_LAUNCHER_RECOVERY_READY_CLEAN
            || result == S64_LAUNCHER_RECOVERY_READY_RESTORED) {
            show_title();
            cputs("PLATFORM READY\r\n");
            if (result == S64_LAUNCHER_RECOVERY_READY_RESTORED) {
                cputs(s64_launcher_recovery_message(result));
                cputs("\r\n");
            }
            cputs("RECOVERY READY\r\n\r\n");
            cputs("EDITOR HANDOFF PENDING\r\n");
            return 0;
        }

        if (!handle_recovery_blocked(result)) {
            return 1;
        }
    }
}
