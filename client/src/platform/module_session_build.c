#include "platform/module_session.h"

uint8_t s64_module_session_reserve(S64ReuRuntimeControl* control)
{
    S64ReuAllocation allocation;
    uint8_t i;
    if (!s64_reu_runtime_valid(control)) return 0u;
    for (i = 0u; i < control->allocation_count; ++i) {
        if (control->allocations[i].owner == S64_REU_OWNER_RUNTIME
            && control->allocations[i].flags == S64_MODULE_SESSION_ALLOCATION_FLAG)
            return 0u;
    }
    if (!s64_reu_runtime_allocate(control, S64_REU_OWNER_RUNTIME,
            S64_MODULE_SESSION_WIRE_SIZE, 256u, &allocation)) return 0u;
    control->allocations[control->allocation_count - 1u].flags =
        S64_MODULE_SESSION_ALLOCATION_FLAG;
    return 1u;
}
