#include <conio.h>
#include <time.h>

#include "launcher/recovery_flow.h"
#include "launcher/recovery_runtime.h"
#include "platform/preflight.h"
#include "platform/module_preload.h"
#include "platform/reu_runtime.h"
#include "platform/session_lifecycle.h"

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

static unsigned char retry_lifecycle(
    const char* message,
    unsigned char allow_retry
)
{
    unsigned char key;
    for (;;) {
        show_title();
        cputs(message);
        cputs("\r\n\r\n");
        cputs(allow_retry ? "R RETRY    X EXIT" : "X EXIT");
        key = (unsigned char)cgetc();
        if (allow_retry && (key == 'R' || key == 'r')) return 1u;
        if (key == 'X' || key == 'x') return 0u;
    }
}

int main(void)
{
    /* Keep the large, session-lived control structure off cc65's stack. */
    static S64LauncherRecoveryRuntime recovery_runtime;
    S64LauncherRecoveryResult result;
    S64SessionLifecycleResult lifecycle_result;
    S64SystemResult system_result;
    S64ModuleTransferPlan transfer_plan;
    uint32_t session_id;
    unsigned char exit_status = 1u;

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
            break;
        }

        if (!handle_recovery_blocked(result)) {
            return 1;
        }
    }

    session_id = (uint32_t)clock();
    if (session_id == 0UL) session_id = 1UL;

    for (;;) {
        lifecycle_result = s64_launcher_session_begin(
            &recovery_runtime,
            session_id,
            0x0201u,
            0UL,
            S64_SYSTEM_MODE_INTERACTIVE
        );
        if (lifecycle_result == S64_SESSION_LIFECYCLE_OK) break;
        /* A failed begin can still own ACTIVE if restore/CLEAN failed. */
        if (recovery_runtime.lifecycle.active_record_written) goto cleanup;
        if (!retry_lifecycle("SESSION START FAILED", 1u)) return 1;
    }

    if (!s64_launcher_runtime_control_create(&recovery_runtime)) {
        for (;;) {
            if (!retry_lifecycle("REU CONTROL VERIFY FAILED", 1u)) {
                goto cleanup;
            }
            if (s64_launcher_runtime_control_create(&recovery_runtime)) {
                break;
            }
        }
    }

    if (!s64_launcher_module_cache_plan(&recovery_runtime)) {
        if (!retry_lifecycle("MODULE CACHE PLAN FAILED", 0u)) goto cleanup;
    }

    for (;;) {
        if (s64_launcher_module_manifest_load(&recovery_runtime)) break;
        if (!retry_lifecycle("MODULE MANIFEST FAILED", 1u)) goto cleanup;
    }

    for (;;) {
        if (s64_system_state_set_mode(&recovery_runtime.system_state,
                S64_SYSTEM_MODE_IO_SAFE) != S64_SYSTEM_OK) goto cleanup;
        if (!s64_launcher_module_session_clear(&recovery_runtime)) goto cleanup;
        show_title();
        cputs("PRELOADING MODULES...\r\n");
        if (!s64_module_preload(&recovery_runtime.dos,
                &recovery_runtime.module_manifest)) {
            if (!retry_lifecycle("MODULE PRELOAD FAILED", 1u)) goto cleanup;
            continue;
        }
        cputs("VERIFYING MODULES...\r\n");
        if (s64_module_cache_verify(&recovery_runtime.module_manifest,
                &recovery_runtime.system_state)) break;
        if (!retry_lifecycle("MODULE INTEGRITY FAILED", 1u)) goto cleanup;
    }
    if (!s64_launcher_module_session_publish(&recovery_runtime)) goto cleanup;
    if (s64_launcher_session_resume_interactive(&recovery_runtime)
            != S64_SYSTEM_OK) goto cleanup;

    show_title();
    cputs("PLATFORM READY\r\n");
    if (result == S64_LAUNCHER_RECOVERY_READY_RESTORED) {
        cputs(s64_launcher_recovery_message(result));
        cputs("\r\n");
    }
    cputs("RECOVERY READY\r\n");
    cputs("SESSION ACTIVE\r\n");
    cputs("REU CONTROL READY\r\n");
    cputs("MODULE CACHE RESERVED\r\n");
    cputs("MODULE MANIFEST READY\r\n\r\n");
    cputs("MODULE CACHE VERIFIED\r\n");
    cputs("SHARED SESSION READY\r\n");
    cputs("OPENING EDITOR...\r\n");

    system_result = s64_launcher_session_prepare_transition(&recovery_runtime);
    if (system_result != S64_SYSTEM_OK) {
        if (!retry_lifecycle("TRANSITION PREP FAILED", 0u)) goto cleanup;
    }
    if (!s64_launcher_transfer_capsule_prepare(
            &recovery_runtime,
            S64_MODULE_EDITOR
        )) {
        if (!retry_lifecycle("TRANSFER CAPSULE FAILED", 0u)) goto cleanup;
    }
    if (!s64_launcher_transfer_plan_load(
            &recovery_runtime,
            &transfer_plan
        )) {
        if (!retry_lifecycle("TRANSFER PLAN FAILED", 0u)) goto cleanup;
    }

    /*
     * The overwrite-safe execution copy is the next migration slice. The
     * verified capsule and transfer plan now bind the ACTIVE session to the
     * exact editor payload, RAM destination and startup address.
     */
    cputs("TRANSFER CAPSULE READY\r\n");
    cputs("TRANSFER PLAN READY\r\n");
    cputs("EDITOR HANDOFF PENDING\r\n");
    system_result = s64_launcher_session_resume_interactive(&recovery_runtime);
    if (system_result != S64_SYSTEM_OK) {
        if (!retry_lifecycle("TRANSITION CANCEL FAILED", 0u)) goto cleanup;
    }

    exit_status = 0u;
cleanup:
    /* All exits that may own durable ACTIVE restore/verify before CLEAN. */
    for (;;) {
        lifecycle_result = s64_launcher_session_end(&recovery_runtime);
        if (lifecycle_result == S64_SESSION_LIFECYCLE_OK) {
            cputs("SESSION CLEAN\r\n");
            return exit_status;
        }
        if (!retry_lifecycle("SESSION CLEANUP FAILED", 1u)) return 1;
    }
}
