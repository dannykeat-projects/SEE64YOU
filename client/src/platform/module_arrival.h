#ifndef SEE64YOU_PLATFORM_MODULE_ARRIVAL_H
#define SEE64YOU_PLATFORM_MODULE_ARRIVAL_H
#include "platform/module_manifest.h"

/* Always consumes the marker; session adoption is still required afterward. */
uint8_t s64_module_arrival_consume(volatile uint8_t* marker,
    S64ModuleId destination);
#ifdef __CC65__
uint8_t s64_module_transfer_consume_arrival(S64ModuleId destination);
#endif
#endif
