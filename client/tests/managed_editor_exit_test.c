#include "platform/managed_editor.h"
#include "platform/basic_return.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>

static unsigned stage, modes, reads, revokes, ends, executes;
static jmp_buf arrival;
S64SystemResult s64_system_state_set_mode(S64SystemState* state, S64SystemMode mode)
{ (void)state; assert(mode == S64_SYSTEM_MODE_IO_SAFE); ++modes; return stage == 1 ? S64_SYSTEM_INVALID : S64_SYSTEM_OK; }
uint8_t s64_basic_return_read(const S64ReuRuntimeControl* c, const S64SessionJournal* j, uint8_t* w)
{ (void)c; (void)j; assert(w == S64_BASIC_RETURN_WORKSPACE && modes == 1); ++reads; return stage != 2; }
uint8_t s64_basic_return_revoke(const S64ReuRuntimeControl* c, uint8_t* w)
{ (void)c; assert(w == S64_BASIC_RETURN_WORKSPACE && reads == 1); ++revokes; return stage != 3; }
S64SessionLifecycleResult s64_managed_editor_end(S64ModuleRuntime* r)
{ (void)r; assert(revokes == 1); ++ends; return stage == 4 ? S64_SESSION_LIFECYCLE_RESTORE_FAILED : S64_SESSION_LIFECYCLE_OK; }
void s64_basic_return_execute(const uint8_t* w)
{ assert(w == S64_BASIC_RETURN_WORKSPACE && ends == 1 && stage == 0); ++executes; longjmp(arrival, 1); }
int main(void)
{
    S64ModuleRuntime runtime;
    assert(!s64_managed_editor_exit(0));
    for (stage = 0; stage <= 4; ++stage) {
        modes = reads = revokes = ends = executes = 0;
        if (!setjmp(arrival)) {
            assert(s64_managed_editor_exit(&runtime) == (stage == 2));
            assert(stage != 0 && !executes);
        } else assert(stage == 0 && executes == 1);
        assert(reads == (stage != 1));
        assert(revokes == (stage != 1));
        assert(ends == (stage != 1 && stage != 3));
    }
    puts("managed editor return ordering/failure tests passed");
    return 0;
}
