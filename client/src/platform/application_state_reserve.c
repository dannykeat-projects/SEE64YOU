#include "platform/application_state_reserve.h"

uint8_t s64_application_state_reserve(S64ReuRuntimeControl* control)
{
    S64ReuAllocation item;
    uint8_t i;

    if (!s64_reu_runtime_valid(control)) return 0u;
    for (i = 0u; i < control->allocation_count; ++i) {
        if (control->allocations[i].flags == S64_APPLICATION_STATE_FLAG) {
            return 0u;
        }
    }
    if (!s64_reu_runtime_allocate(control, S64_REU_OWNER_RUNTIME,
            S64_APPLICATION_STATE_SIZE, 256u, &item)) {
        return 0u;
    }
    control->allocations[control->allocation_count - 1u].flags =
        S64_APPLICATION_STATE_FLAG;
    return 1u;
}
