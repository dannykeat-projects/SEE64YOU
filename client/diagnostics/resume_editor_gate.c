#include <conio.h>
#include <time.h>
#include "resume_editor_gate.h"
#include "launcher/basic_return_runtime.h"
#include "launcher/code_assets_runtime.h"
#include "platform/basic_return.h"
#include "platform/module_preload.h"
#include "platform/package_version.h"
#include "platform/preflight.h"

int main(void)
{
    static S64LauncherRecoveryRuntime runtime;
    S64PlatformPreflight preflight;
    S64ModuleTransferPlan plan;
    S64LauncherRecoveryResult recovered;
    uint32_t session;
    unsigned char key;
    clrscr();
    cputs("RESUMED EDITOR GATE\r\n\r\nP POSITIVE   N CORRUPT   X EXIT\r\n");
    do {
        key = cgetc();
        if (key == 'p') key = 'P';
        if (key == 'n') key = 'N';
        if (key == 'x') key = 'X';
    } while (key != 'P' && key != 'N' && key != 'X');
    if (key == 'X') return 0;
    s64_platform_preflight_run(&preflight);
    if (preflight.requirement != S64_PLATFORM_READY || !preflight.storage_ready) {
        cputs("RUN NORMAL LAUNCHER\r\n");
        return 1;
    }
    cputs("PLATFORM READY\r\n");
    s64_launcher_recovery_runtime_init_hardware(&runtime);
    recovered = s64_launcher_recovery_runtime_run(&runtime);
    if (recovered > S64_LAUNCHER_RECOVERY_READY_RESTORED) {
        cputs("RUN NORMAL LAUNCHER\r\n");
        return 1;
    }
    session = (uint32_t)clock();
    if (!session) session = 1;
    if (s64_launcher_session_begin(&runtime, session, S64_PACKAGE_VERSION, 0,
            S64_SYSTEM_MODE_INTERACTIVE) != S64_SESSION_LIFECYCLE_OK
        || !s64_launcher_runtime_control_create(&runtime)
        || !s64_launcher_module_cache_plan(&runtime)
        || !s64_launcher_module_manifest_load(&runtime)
        || s64_system_state_set_mode(&runtime.system_state, S64_SYSTEM_MODE_IO_SAFE)
            != S64_SYSTEM_OK
        || !s64_launcher_module_session_clear(&runtime)
        || !s64_module_preload(&runtime.dos, &runtime.module_manifest)
        || !s64_module_cache_verify(&runtime.module_manifest, &runtime.system_state)
        || !s64_launcher_module_session_publish(&runtime)
        || !s64_launcher_basic_return_prepare(&runtime)
        || !s64_launcher_code_assets_prepare(&runtime)) goto cleanup;
    cputs("EXACT PACKAGE READY\r\n");
    if (!s64_resume_editor_fixture(&runtime, key == 'N', S64_BASIC_RETURN_WORKSPACE))
        goto cleanup;
    cputs(key == 'N' ? "AUTHORITATIVE STATE CORRUPTED\r\n" : "ACTIVE FIXTURE VERIFIED\r\n");
    if (s64_launcher_session_resume_interactive(&runtime) != S64_SYSTEM_OK
        || s64_launcher_session_prepare_transition(&runtime) != S64_SYSTEM_OK
        || !s64_launcher_transfer_capsule_prepare(&runtime, S64_MODULE_EDITOR)
        || !s64_launcher_transfer_plan_load(&runtime, &plan)) goto cleanup;
    cputs("EDITOR TRANSFER\r\n");
    (void)s64_module_transfer_execute(&plan);
cleanup:
    /* Failure never transfers; original production cleanup remains authoritative. */
    cputs("GATE SETUP FAILED\r\n");
    if (runtime.lifecycle.active_record_written) {
        /* A late diagnostic failure may already have asserted authority. */
        if (s64_reu_runtime_valid(&runtime.reu_control)) {
            runtime.reu_control.state_flags &= (uint8_t)~S64_REU_RUNTIME_STATE_APPLICATION_VALID;
            if (s64_system_state_set_mode(&runtime.system_state, S64_SYSTEM_MODE_IO_SAFE)
                    != S64_SYSTEM_OK || !s64_reu_runtime_store_verified(&runtime.reu_control)) {
                cputs("ACTIVE: RUN LAUNCHER\r\n");
                for (;;) {}
            }
        }
        if (s64_launcher_basic_return_end(&runtime) != S64_SESSION_LIFECYCLE_OK) {
            cputs("ACTIVE: RUN LAUNCHER\r\n");
            for (;;) {}
        }
        cputs("SESSION CLEAN\r\n");
    }
    return 1;
}
