#include <assert.h>
#include <stdio.h>
#include <string.h>
#define main launcher_main
#include "launcher/main.c"
#undef main

static unsigned stage, ends, clean, cleanup_failure, resumes, clears, verifications;
static unsigned capsules, plans, executions;
void clrscr(void) {}
void cputs(const char* text) { (void)text; }
char cgetc(void) { return 'X'; }
void s64_platform_preflight_run(S64PlatformPreflight* p)
{ memset(p, 0, sizeof(*p)); p->requirement = S64_PLATFORM_READY; p->storage_ready = 1; }
void s64_launcher_recovery_runtime_init_hardware(S64LauncherRecoveryRuntime* r)
{ memset(r, 0, sizeof(*r)); }
S64LauncherRecoveryResult s64_launcher_recovery_runtime_run(S64LauncherRecoveryRuntime* r)
{ (void)r; return S64_LAUNCHER_RECOVERY_READY_NONE; }
const char* s64_launcher_recovery_message(S64LauncherRecoveryResult r)
{ (void)r; return ""; }
uint8_t s64_launcher_recovery_retryable(S64LauncherRecoveryResult r)
{ (void)r; return 0; }
S64SessionLifecycleResult s64_launcher_session_begin(S64LauncherRecoveryRuntime* r,
    uint32_t id, uint16_t version, uint32_t ref, S64SystemMode mode)
{
    (void)id; (void)version; (void)ref; (void)mode;
    r->lifecycle.active_record_written = 1;
    return stage == 1 ? S64_SESSION_LIFECYCLE_RESTORE_FAILED : S64_SESSION_LIFECYCLE_OK;
}
uint8_t s64_launcher_runtime_control_create(S64LauncherRecoveryRuntime* r)
{ (void)r; return stage != 2; }
uint8_t s64_launcher_module_cache_plan(S64LauncherRecoveryRuntime* r)
{ (void)r; return stage != 3; }
uint8_t s64_launcher_module_manifest_load(S64LauncherRecoveryRuntime* r)
{ (void)r; return stage != 4; }
S64SystemResult s64_system_state_set_mode(S64SystemState* s, S64SystemMode m)
{ (void)s; (void)m; return stage == 5 ? S64_SYSTEM_INVALID : S64_SYSTEM_OK; }
uint8_t s64_launcher_module_session_clear(S64LauncherRecoveryRuntime* r)
{ (void)r; ++clears; return stage != 11; }
uint8_t s64_launcher_module_session_publish(S64LauncherRecoveryRuntime* r)
{ (void)r; assert(verifications == 1 && stage != 10); return stage != 12; }
uint8_t s64_module_preload(S64Dos* d, S64ModuleManifest* m)
{ (void)d; (void)m; assert(clears == 1); return stage != 6; }
uint8_t s64_module_cache_verify(S64ModuleManifest* m, S64SystemState* s)
{ (void)m; (void)s; ++verifications; return stage != 10; }
S64SystemResult s64_launcher_session_prepare_transition(S64LauncherRecoveryRuntime* r)
{ (void)r; return stage == 8 ? S64_SYSTEM_INVALID : S64_SYSTEM_OK; }
uint8_t s64_launcher_transfer_capsule_prepare(S64LauncherRecoveryRuntime* r,
    S64ModuleId destination)
{
    assert(r->lifecycle.active_record_written && verifications == 1);
    assert(destination == S64_MODULE_EDITOR);
    ++capsules;
    return stage != 13;
}
uint8_t s64_launcher_transfer_plan_load(S64LauncherRecoveryRuntime* r,
    S64ModuleTransferPlan* plan)
{
    (void)plan;
    assert(r->lifecycle.active_record_written && capsules == 1 && stage != 13);
    ++plans;
    return stage != 14;
}
S64SystemResult s64_launcher_session_resume_interactive(S64LauncherRecoveryRuntime* r)
{
    (void)r; ++resumes;
    return stage == 7 || (stage == 9 && resumes == 2)
        ? S64_SYSTEM_INVALID : S64_SYSTEM_OK;
}
S64SessionLifecycleResult s64_launcher_session_end(S64LauncherRecoveryRuntime* r)
{
    assert(r->lifecycle.active_record_written);
    ++ends;
    if (cleanup_failure) return S64_SESSION_LIFECYCLE_RESTORE_FAILED;
    ++clean;
    r->lifecycle.active_record_written = 0;
    return S64_SESSION_LIFECYCLE_OK;
}
#ifdef S64_CODE_ASSET_LAUNCHER
uint8_t s64_launcher_code_assets_prepare(S64LauncherRecoveryRuntime* r)
{ (void)r; assert(verifications == 1); return stage != 15; }
S64SessionLifecycleResult s64_launcher_code_assets_end(S64LauncherRecoveryRuntime* r)
{ return s64_launcher_session_end(r); }
#endif
#ifdef S64_EXECUTABLE_HANDOFF
uint8_t s64_launcher_basic_return_prepare(S64LauncherRecoveryRuntime* r)
{ (void)r; assert(verifications == 1); return stage != 16; }
S64SessionLifecycleResult s64_launcher_basic_return_end(S64LauncherRecoveryRuntime* r)
{ return s64_launcher_session_end(r); }
uint8_t s64_module_transfer_execute(const S64ModuleTransferPlan* plan)
{
    assert(plan && plans == 1 && stage != 14);
    ++executions;
    return 0u;
}
#endif
int main(void)
{
#ifdef S64_EXECUTABLE_HANDOFF
    for (stage = 0; stage <= 16; ++stage) {
#elif defined(S64_CODE_ASSET_LAUNCHER)
    for (stage = 0; stage <= 15; ++stage) {
#else
    for (stage = 0; stage <= 14; ++stage) {
#endif
        ends = clean = resumes = clears = verifications = 0;
        capsules = plans = executions = 0;
#ifdef S64_EXECUTABLE_HANDOFF
        /* A copier that returns failed to start: always revoke/restore/CLEAN. */
        assert(launcher_main() == 1);
        assert(executions == (stage == 0 || stage == 9));
#else
        assert(launcher_main() == (stage != 0));
#endif
        assert(ends == 1 && clean == 1);
        if (stage == 0 || stage == 9 || stage == 13 || stage == 14) assert(capsules == 1);
        if (stage == 0 || stage == 9 || stage == 14) assert(plans == 1);
        if (stage == 13) assert(plans == 0);
#ifdef S64_CODE_ASSET_LAUNCHER
        if (stage == 15) assert(capsules == 0 && plans == 0);
#endif
    }
    stage = 2; cleanup_failure = 1; ends = clean = 0;
    assert(launcher_main() == 1);
    assert(ends == 1 && clean == 0);
    puts("launcher exit tests passed");
    return 0;
}
