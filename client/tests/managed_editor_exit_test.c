#include "platform/managed_editor.h"
#include "platform/basic_return.h"
#include "platform/managed_editor_state.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

static unsigned stage, modes, reads, revokes, ends, executes, applications, flagged;
static uint8_t snapshot[321];
uint8_t s64_test_basic_return_workspace[1024];
static jmp_buf arrival;
S64SystemResult s64_system_state_set_mode(S64SystemState* state, S64SystemMode mode)
{ assert(mode == S64_SYSTEM_MODE_IO_SAFE); state->mode = mode; ++modes; return stage == 1 ? S64_SYSTEM_INVALID : S64_SYSTEM_OK; }
uint8_t s64_basic_return_read(const S64ReuRuntimeControl* c, const S64SessionJournal* j, uint8_t* w)
{ unsigned i; (void)c; (void)j; assert(w == S64_BASIC_RETURN_WORKSPACE && modes == 1);
  for (i = 0; i < sizeof(snapshot); ++i) w[i] = (uint8_t)i;
  memcpy(snapshot, w, sizeof(snapshot)); ++reads; return stage != 2; }
uint8_t s64_reu_runtime_valid(const S64ReuRuntimeControl* c)
{ return c != 0; }
uint8_t s64_reu_runtime_store_verified(const S64ReuRuntimeControl* c)
{ assert(reads == 1 && !revokes && !ends && !c->state_flags); ++applications;
  assert(!memcmp(snapshot, S64_BASIC_RETURN_WORKSPACE, sizeof(snapshot)));
  return stage != 5; }
uint8_t s64_basic_return_revoke(const S64ReuRuntimeControl* c, uint8_t* w)
{ (void)c; assert(w == S64_BASIC_RETURN_WORKSPACE && reads == 1 && applications == 1); ++revokes; return stage != 3; }
S64SessionLifecycleResult s64_managed_editor_end(S64ModuleRuntime* r)
{ assert(revokes == 1 && !r->reu_control.state_flags); ++ends; return stage == 4 ? S64_SESSION_LIFECYCLE_RESTORE_FAILED : S64_SESSION_LIFECYCLE_OK; }
void s64_basic_return_execute(const uint8_t* w)
{ assert(w == S64_BASIC_RETURN_WORKSPACE && ends == 1 && stage == 0);
  assert(!memcmp(w, snapshot, sizeof(snapshot)));
  ++executes; longjmp(arrival, 1); }
int main(void)
{
    S64ModuleRuntime runtime;
    assert(!s64_managed_editor_exit(0));
    for (flagged = 0; flagged <= 1; ++flagged) for (stage = 0; stage <= 5; ++stage) {
        modes = reads = revokes = ends = executes = applications = 0;
        memset(&runtime, 0, sizeof(runtime));
        if (flagged) runtime.reu_control.state_flags = S64_REU_RUNTIME_STATE_APPLICATION_VALID;
        if (!setjmp(arrival)) {
            assert(s64_managed_editor_exit(&runtime) == (stage == 2));
            assert(stage != 0 && !executes);
        } else assert(stage == 0 && executes == 1);
        assert(reads == (stage != 1));
        assert(applications == (stage != 1));
        assert(revokes == (stage != 1 && stage != 5));
        assert(ends == (stage != 1 && stage != 3 && stage != 5));
    }
    puts("managed editor return ordering/failure tests passed");
    return 0;
}
