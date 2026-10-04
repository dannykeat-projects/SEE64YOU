#include "platform/managed_editor_state.h"

typedef char S64ManagedApplicationFits[512 - (int)sizeof(S64ApplicationState)];
extern S64ModuleRuntime* s64_managed_editor_state_owner;
extern uint8_t* s64_managed_editor_state_wire;

S64ApplicationState* s64_managed_editor_state_decode(S64ModuleRuntime* runtime, uint8_t* wire)
{
    S64ApplicationState* application;
    uint8_t authorized = runtime != 0 && s64_managed_editor_state_owner == runtime
        && wire != 0 && wire == s64_managed_editor_state_wire;
    s64_managed_editor_state_owner = 0;
    s64_managed_editor_state_wire = 0;
    if (!authorized) return 0;
    application = (S64ApplicationState*)(wire + S64_APPLICATION_STATE_SIZE);
    return s64_application_state_decode_payload(application, wire) ? application : 0;
}
