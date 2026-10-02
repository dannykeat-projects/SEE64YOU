#include <conio.h>

#include "launcher/recovery_flow.h"
#include "launcher/recovery_runtime.h"

static void show_title(void)
{
    clrscr();
    cputs("SEE64YOU\r\n");
    cputs("C64 ULTIMATE\r\n\r\n");
}

static void show_diagnostics_placeholder(void)
{
    show_title();
    cputs("RECOVERY DIAGNOSTICS\r\n\r\n");
    cputs("INTEGRATED DIAGNOSTICS\r\n");
    cputs("NOT YET WIRED\r\n\r\n");
    cputs("PRESS ANY KEY");
    (void)cgetc();
}

static unsigned char handle_blocked(
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

    show_title();
    cputs("CHECKING PREVIOUS SESSION...\r\n");

    s64_launcher_recovery_runtime_init_hardware(&recovery_runtime);

    for (;;) {
        result = s64_launcher_recovery_runtime_run(&recovery_runtime);

        if (result == S64_LAUNCHER_RECOVERY_READY_NONE
            || result == S64_LAUNCHER_RECOVERY_READY_CLEAN
            || result == S64_LAUNCHER_RECOVERY_READY_RESTORED) {
            show_title();
            if (result == S64_LAUNCHER_RECOVERY_READY_RESTORED) {
                cputs(s64_launcher_recovery_message(result));
                cputs("\r\n\r\n");
            }
            cputs("RECOVERY READY\r\n");
            cputs("EDITOR HANDOFF PENDING\r\n");
            return 0;
        }

        if (!handle_blocked(result)) {
            return 1;
        }
    }
}
