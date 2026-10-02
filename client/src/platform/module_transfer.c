#include "platform/module_transfer.h"

uint8_t s64_module_transfer_plan(
    const S64TransferCapsule* capsule,
    S64ModuleTransferPlan* plan
)
{
    uint32_t payload_length;
    uint32_t reu_source;

    if (capsule == 0 || plan == 0
        || capsule->destination < S64_MODULE_EDITOR
        || capsule->destination > S64_MODULE_SETTINGS
        || capsule->module.id != (uint8_t)capsule->destination
        || capsule->module.cache_state != S64_MODULE_CACHE_READY
        || capsule->module.byte_length <= 2UL) {
        return 0u;
    }

    payload_length = capsule->module.byte_length - 2UL;
    reu_source = capsule->module.cache_address + 2UL;

    if (payload_length == 0UL || payload_length > 65535UL
        || reu_source < capsule->module.cache_address
        || reu_source >= 0x01000000UL
        || payload_length > 0x01000000UL - reu_source
        || payload_length > 65536UL - capsule->module.load_address
        || capsule->module.startup_address < capsule->module.load_address
        || (uint32_t)capsule->module.startup_address
            >= (uint32_t)capsule->module.load_address + payload_length) {
        return 0u;
    }

    plan->reu_source = reu_source;
    plan->ram_destination = capsule->module.load_address;
    plan->payload_length = (uint16_t)payload_length;
    plan->startup_address = capsule->module.startup_address;
    return 1u;
}
