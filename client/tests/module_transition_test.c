#include "platform/module_transition.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned fault, clears, stores, loads, modes, transitions, resumes;
static unsigned valid_control = 1;
static S64TransferCapsule saved;
uint8_t s64_reu_runtime_valid(const S64ReuRuntimeControl* c) { (void)c; return valid_control; }
S64SystemResult s64_system_state_set_mode(S64SystemState* s, S64SystemMode mode)
{
    assert(mode == S64_SYSTEM_MODE_IO_SAFE); ++modes;
    if (fault == 1) return S64_SYSTEM_APPLY_FAILED;
    s->mode = mode; return S64_SYSTEM_OK;
}
S64SystemResult s64_module_runtime_prepare_transition(S64ModuleRuntime* r)
{
    ++transitions;
    if (fault == 7) return S64_SYSTEM_APPLY_FAILED;
    r->system_state.mode = S64_SYSTEM_MODE_TRANSITION; return S64_SYSTEM_OK;
}
S64SystemResult s64_module_runtime_resume_interactive(S64ModuleRuntime* r)
{
    ++resumes;
    if (fault == 9) return S64_SYSTEM_APPLY_FAILED;
    r->system_state.mode = S64_SYSTEM_MODE_INTERACTIVE; return S64_SYSTEM_OK;
}
uint8_t s64_transfer_capsule_clear(const S64ReuRuntimeControl* c)
{
    (void)c; ++clears;
    if ((fault == 2 && clears == 1) || (fault == 8 && clears == 2)) return 0;
    memset(&saved, 0, sizeof(saved)); return 1;
}
uint8_t s64_transfer_capsule_store(const S64ReuRuntimeControl* c, const S64SessionJournal* j,
    const S64ModuleManifest* m, S64ModuleId source, S64ModuleId destination)
{
    (void)c; (void)m; ++stores;
    if (fault == 3 || fault == 8 || fault == 9) return 0;
    saved.source = source; saved.destination = destination; saved.journal = *j;
    saved.module = m->modules[(uint8_t)destination - 1u]; return 1;
}
uint8_t s64_transfer_capsule_load(const S64ReuRuntimeControl* c, S64TransferCapsule* out)
{
    (void)c; ++loads; if (fault == 4) return 0;
    *out = saved;
    if (fault == 5) out->journal.state = 0;
    if (fault == 6) out->module.startup_address = 0;
    if (fault == 10) ++out->journal.sequence;
    if (fault == 11) out->destination = S64_MODULE_SETTINGS;
    if (fault == 12) ++out->module.byte_length;
    if (fault == 13) ++out->module.startup_address;
    if (fault == 14) ++out->module.integrity;
    if (fault == 15) out->source = S64_MODULE_SETTINGS;
    return 1;
}
static void setup(S64ModuleRuntime* r, S64ModuleTransferPlan* p)
{
    unsigned i;
    memset(r, 0, sizeof(*r)); memset(p, 0xa5, sizeof(*p));
    r->current_module = S64_MODULE_EDITOR; r->system_state.captured = 1;
    r->system_state.mode = S64_SYSTEM_MODE_INTERACTIVE;
    r->system_state.snapshot.turbo_control = 0x25; r->system_state.snapshot.turbo_control_valid = 1;
    r->lifecycle.active_record_written = 1; r->lifecycle.journal.state = S64_SESSION_STATE_ACTIVE;
    r->lifecycle.journal.sequence = 7; r->lifecycle.journal.session_id = 23;
    r->lifecycle.journal.package_version = 0x0201;
    r->reu_control.session_id = 23; r->reu_control.app_version = 0x0201;
    for (i=0; i<3; ++i) {
        S64ModuleDescriptor* m = &r->module_manifest.modules[i];
        m->id = i+1; m->cache_state = S64_MODULE_CACHE_READY;
        m->cache_address = 0x030000UL; m->byte_length = 0x3002UL;
        m->load_address = 0x0801; m->startup_address = 0x080d;
    }
    fault = clears = stores = loads = modes = transitions = resumes = 0; valid_control = 1;
}
static void zero_plan(const S64ModuleTransferPlan* p)
{
    S64ModuleTransferPlan empty; memset(&empty, 0, sizeof(empty));
    assert(memcmp(p, &empty, sizeof(empty)) == 0);
}
int main(void)
{
    S64ModuleRuntime r;
    S64ModuleTransferPlan p;
    S64SessionLifecycle lifecycle;
    S64SystemSnapshot snapshot;
    static const unsigned char pairs[][2] = {{1,2},{2,1},{2,3},{3,2}};
    unsigned i;
    for (i=0; i<4; ++i) {
        setup(&r, &p); r.current_module = pairs[i][0];
        lifecycle = r.lifecycle; snapshot = r.system_state.snapshot;
        assert(s64_module_transition_prepare(&r, pairs[i][1], &p) == S64_MODULE_TRANSITION_OK);
        assert(saved.source == (S64ModuleId)pairs[i][0]);
        assert(p.destination == pairs[i][1] && p.reu_source == 0x030002UL && p.payload_length == 0x3000);
        assert(r.current_module == pairs[i][0] && r.system_state.mode == S64_SYSTEM_MODE_TRANSITION);
        assert(memcmp(&r.lifecycle, &lifecycle, sizeof(lifecycle)) == 0);
        assert(memcmp(&r.system_state.snapshot, &snapshot, sizeof(snapshot)) == 0);
        assert(s64_module_transition_cancel(&r, &p) == S64_MODULE_TRANSITION_OK);
        zero_plan(&p); assert(r.system_state.mode == S64_SYSTEM_MODE_INTERACTIVE);
    }
    for (i=1; i<=15; ++i) {
        setup(&r, &p); fault = i; lifecycle = r.lifecycle; snapshot = r.system_state.snapshot;
        assert(s64_module_transition_prepare(&r, S64_MODULE_PUBLISHER, &p)
            == ((i==1 || i==8 || i==9) ? S64_MODULE_TRANSITION_RECOVERY_REQUIRED : S64_MODULE_TRANSITION_FAILED));
        zero_plan(&p);
        assert(memcmp(&r.lifecycle, &lifecycle, sizeof(lifecycle)) == 0);
        assert(memcmp(&r.system_state.snapshot, &snapshot, sizeof(snapshot)) == 0);
        if (i!=1 && i!=8 && i!=9) assert(r.system_state.mode == S64_SYSTEM_MODE_INTERACTIVE);
    }
    for (i=0; i<9; ++i) {
        setup(&r, &p);
        switch(i) {
            case 0: r.lifecycle.clean_record_written=1; break;
            case 1: r.lifecycle.active_record_written=0; break;
            case 2: r.system_state.restored=1; break;
            case 3: r.shared_invalidated=1; break;
            case 4: r.lifecycle.journal.state=S64_SESSION_STATE_CLEAN; break;
            case 5: ++r.lifecycle.journal.session_id; break;
            case 6: valid_control=0; break;
            case 7: r.system_state.captured=0; break;
            case 8: r.system_state.mode=S64_SYSTEM_MODE_TRANSITION; break;
        }
        assert(s64_module_transition_prepare(&r, S64_MODULE_PUBLISHER, &p) == S64_MODULE_TRANSITION_INVALID);
        zero_plan(&p); assert(!modes && !clears && !stores && !loads);
    }
    setup(&r,&p); assert(s64_module_transition_prepare(&r,S64_MODULE_EDITOR,&p)==S64_MODULE_TRANSITION_INVALID); zero_plan(&p);
    assert(s64_module_transition_prepare(&r,(S64ModuleId)0,&p)==S64_MODULE_TRANSITION_INVALID);
    assert(s64_module_transition_prepare(&r,(S64ModuleId)4,&p)==S64_MODULE_TRANSITION_INVALID);
    assert(s64_module_transition_prepare(0,S64_MODULE_PUBLISHER,&p)==S64_MODULE_TRANSITION_INVALID);
    assert(s64_module_transition_prepare(&r,S64_MODULE_PUBLISHER,0)==S64_MODULE_TRANSITION_INVALID);
    puts("module transition ownership/order/failure tests passed"); return 0;
}
