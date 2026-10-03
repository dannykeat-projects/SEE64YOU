#include "platform/module_transfer.h"

#ifdef __CC65__
#include <string.h>

#define S64_TRANSFER_STUB_BASE 0x0700u
#define S64_TRANSFER_PARAM_BASE 0x07d0u
#define S64_TRANSFER_STUB_CAPACITY (S64_TRANSFER_PARAM_BASE - S64_TRANSFER_STUB_BASE)

extern const uint8_t s64_module_transfer_stub_image[];
extern const uint8_t s64_module_transfer_stub_end[];
void s64_module_transfer_stub_enter(void);
#endif

uint8_t s64_module_transfer_plan_safe(const S64ModuleTransferPlan* plan)
{
    uint32_t ram_end;

    if (plan == 0 || plan->payload_length == 0u
        || plan->destination < S64_MODULE_EDITOR
        || plan->destination > S64_MODULE_SETTINGS
        || plan->reu_source >= 0x01000000UL
        || (uint32_t)plan->payload_length
            > 0x01000000UL - plan->reu_source
        || plan->ram_destination < 0x0801u) {
        return 0u;
    }

    ram_end = (uint32_t)plan->ram_destination + plan->payload_length;
    return ram_end <= 0x0000d000UL
        && plan->startup_address >= plan->ram_destination
        && (uint32_t)plan->startup_address < ram_end;
}

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

    plan->destination = (uint8_t)capsule->destination;
    plan->reu_source = reu_source;
    plan->ram_destination = capsule->module.load_address;
    plan->payload_length = (uint16_t)payload_length;
    plan->startup_address = capsule->module.startup_address;
    return s64_module_transfer_plan_safe(plan);
}

#ifdef __CC65__
uint8_t s64_module_transfer_execute(const S64ModuleTransferPlan* plan)
{
    volatile uint8_t* params = (volatile uint8_t*)S64_TRANSFER_PARAM_BASE;
    unsigned int stub_size;

    if (!s64_module_transfer_plan_safe(plan)) return 0u;
    stub_size = (unsigned int)(
        s64_module_transfer_stub_end - s64_module_transfer_stub_image
    );
    if (stub_size == 0u || stub_size > S64_TRANSFER_STUB_CAPACITY) return 0u;

    memcpy((void*)S64_TRANSFER_STUB_BASE,
        s64_module_transfer_stub_image, stub_size);

    params[0] = (uint8_t)plan->reu_source;
    params[1] = (uint8_t)(plan->reu_source >> 8);
    params[2] = (uint8_t)(plan->reu_source >> 16);
    params[3] = (uint8_t)plan->ram_destination;
    params[4] = (uint8_t)(plan->ram_destination >> 8);
    params[5] = (uint8_t)plan->payload_length;
    params[6] = (uint8_t)(plan->payload_length >> 8);
    params[7] = (uint8_t)plan->startup_address;
    params[8] = (uint8_t)(plan->startup_address >> 8);
    params[9] = 0u;
    params[10] = plan->destination;

    s64_module_transfer_stub_enter();
    return 0u;
}

#endif
