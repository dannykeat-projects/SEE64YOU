#include "platform/application_state_internal.h"

uint8_t s64_application_state_decode(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, S64ApplicationState* state, const uint8_t* wire)
{
    return state && s64_application_state_wire_valid(control, journal, wire)
        && s64_application_state_decode_payload(state, wire);
}

uint8_t s64_application_state_load(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, S64ApplicationState* state, uint8_t* wire)
{
    return state && s64_application_state_load_wire(control, journal, wire)
        && s64_application_state_decode_payload(state, wire);
}
