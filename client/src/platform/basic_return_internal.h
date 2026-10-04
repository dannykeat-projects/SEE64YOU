#ifndef SEE64YOU_BASIC_RETURN_INTERNAL_H
#define SEE64YOU_BASIC_RETURN_INTERNAL_H
#include "platform/basic_return.h"
uint8_t s64_basic_return_header_valid(const uint8_t* wire);
uint8_t s64_basic_return_active(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, uint8_t* expected);
#endif
