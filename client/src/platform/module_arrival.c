#include "platform/module_arrival.h"

uint8_t s64_module_arrival_consume(volatile uint8_t* marker,
    S64ModuleId destination)
{
    uint8_t valid;
    if (marker == 0) return 0u;
    valid = destination >= S64_MODULE_EDITOR
        && destination <= S64_MODULE_SETTINGS
        && marker[0] == 'S' && marker[1] == '6'
        && marker[2] == (uint8_t)destination;
    marker[0] = 0u;
    marker[1] = 0u;
    marker[2] = 0u;
    return valid;
}

#ifdef __CC65__
uint8_t s64_module_transfer_consume_arrival(S64ModuleId destination)
{
    return s64_module_arrival_consume((volatile uint8_t*)0x02a7u,
        destination);
}
#endif
