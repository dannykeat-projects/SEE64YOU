#ifndef SEE64YOU_PLATFORM_APPLICATION_STATE_INTERNAL_H
#define SEE64YOU_PLATFORM_APPLICATION_STATE_INTERNAL_H
#include "platform/application_state.h"
uint8_t s64_application_state_active(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, uint8_t* expected);
uint8_t s64_application_state_payload_valid(const uint8_t* wire);
uint8_t s64_application_state_wire_valid(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const uint8_t* wire);
#endif
