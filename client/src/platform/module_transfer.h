#ifndef SEE64YOU_PLATFORM_MODULE_TRANSFER_H
#define SEE64YOU_PLATFORM_MODULE_TRANSFER_H

#include <stdint.h>
#include "platform/transfer_capsule.h"

typedef struct {
    uint32_t reu_source;
    uint16_t ram_destination;
    uint16_t payload_length;
    uint16_t startup_address;
} S64ModuleTransferPlan;

uint8_t s64_module_transfer_plan_safe(const S64ModuleTransferPlan* plan);
uint8_t s64_module_transfer_plan(
    const S64TransferCapsule* capsule,
    S64ModuleTransferPlan* plan
);
#ifdef __CC65__
/* Success does not return: execution continues at plan->startup_address. */
uint8_t s64_module_transfer_execute(const S64ModuleTransferPlan* plan);
#endif

#endif
