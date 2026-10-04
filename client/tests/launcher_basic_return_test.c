#include "launcher/basic_return_runtime.h"
#include "platform/basic_return.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned stage, reserves, stores, publishes, modes, revokes, ends;
uint8_t s64_basic_return_reserve(S64ReuRuntimeControl* c)
{ (void)c; ++reserves; return stage != 2; }
uint8_t s64_reu_runtime_store_verified(const S64ReuRuntimeControl* c)
{ (void)c; ++stores; return stage != 3; }
uint8_t s64_basic_return_publish(const S64ReuRuntimeControl* c, const S64SessionJournal* j, uint8_t* w)
{ (void)c; (void)j; assert(w == S64_BASIC_RETURN_WORKSPACE); ++publishes; return stage != 4; }
S64SystemResult s64_system_state_set_mode(S64SystemState* s, S64SystemMode mode)
{ (void)s; assert(mode == S64_SYSTEM_MODE_IO_SAFE); ++modes; return stage == 5 ? S64_SYSTEM_INVALID : S64_SYSTEM_OK; }
uint8_t s64_basic_return_revoke(const S64ReuRuntimeControl* c, uint8_t* w)
{ (void)c; assert(w == S64_BASIC_RETURN_WORKSPACE); ++revokes; return stage != 6; }
S64SessionLifecycleResult s64_launcher_code_assets_end(S64LauncherRecoveryRuntime* r)
{ ++ends; r->module_session_invalidated = 1; return stage == 7 ? S64_SESSION_LIFECYCLE_RESTORE_FAILED : S64_SESSION_LIFECYCLE_OK; }
int main(void)
{
    S64LauncherRecoveryRuntime runtime;
    assert(!s64_launcher_basic_return_prepare(0));
    assert(s64_launcher_basic_return_end(0) == S64_SESSION_LIFECYCLE_INVALID);
    for (stage = 0; stage <= 7; ++stage) {
        memset(&runtime, 0, sizeof(runtime));
        reserves = stores = publishes = modes = revokes = ends = 0;
        runtime.lifecycle.active_record_written = stage != 1;
        runtime.system_state.captured = 1;
        runtime.system_state.mode = S64_SYSTEM_MODE_IO_SAFE;
        assert(s64_launcher_basic_return_prepare(&runtime) == (stage == 0 || stage >= 5));
        assert(runtime.basic_return_reserved == (stage != 1 && stage != 2));
        assert(reserves == (stage != 1));
        assert(stores == (stage != 1 && stage != 2));
        assert(publishes == (stage != 1 && stage != 2 && stage != 3));
        assert(s64_launcher_basic_return_end(&runtime) ==
            (stage >= 5 ? S64_SESSION_LIFECYCLE_RESTORE_FAILED : S64_SESSION_LIFECYCLE_OK));
        assert(ends == (stage != 5 && stage != 6));
        if (stage == 7) {
            /* CLEAN-save retry after completed revocation never reopens I/O. */
            assert(runtime.module_session_invalidated && modes == 1 && revokes == 1);
            stage = 0;
            assert(s64_launcher_basic_return_end(&runtime) == S64_SESSION_LIFECYCLE_OK);
            assert(modes == 1 && revokes == 1 && ends == 2);
            break;
        }
    }
    memset(&runtime, 0, sizeof(runtime));
    runtime.lifecycle.active_record_written = 1; runtime.system_state.captured = 1;
    runtime.system_state.mode = S64_SYSTEM_MODE_INTERACTIVE;
    assert(!s64_launcher_basic_return_prepare(&runtime));
    puts("launcher BASIC return prepare/cleanup/retry tests passed");
    return 0;
}
