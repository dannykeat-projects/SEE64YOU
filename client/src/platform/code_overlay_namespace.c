#include "platform/code_overlay_namespace.h"

#include <string.h>

void s64_code_overlay_namespace_clear(S64CodeOverlayNamespace* namespace_state)
{
    if (namespace_state != 0) {
        memset(namespace_state, 0, sizeof(*namespace_state));
    }
}

uint8_t s64_code_overlay_namespace_capture(
    S64CodeOverlayNamespace* namespace_state,
    const S64ReuRuntimeControl* control,
    const S64ReuAllocation* caches,
    uint8_t count
)
{
    uint8_t i;
    uint8_t j;
    uint8_t found;

    if (namespace_state == 0 || control == 0 || caches == 0
        || count == 0u || count > S64_CODE_OVERLAY_MAX_COUNT
        || !s64_reu_runtime_valid(control)) {
        return 0u;
    }

    for (i = 0u; i < count; ++i) {
        if (caches[i].owner != S64_REU_OWNER_MODULE
            || caches[i].length == 0UL
            || caches[i].length > S64_CODE_OVERLAY_CAPACITY) {
            return 0u;
        }
        found = 0u;
        for (j = 0u; j < control->allocation_count; ++j) {
            const S64ReuAllocation* item = &control->allocations[j];
            if (item->owner == caches[i].owner
                && item->flags == caches[i].flags
                && item->address == caches[i].address
                && item->length == caches[i].length) {
                found = 1u;
                break;
            }
        }
        if (!found) return 0u;
        for (j = 0u; j < i; ++j) {
            if (caches[j].address == caches[i].address) return 0u;
        }
    }

    s64_code_overlay_namespace_clear(namespace_state);
    namespace_state->count = count;
    for (i = 0u; i < count; ++i) {
        namespace_state->caches[i] = caches[i];
    }
    return 1u;
}

uint8_t s64_code_overlay_namespace_activate(
    const S64CodeOverlayNamespace* namespace_state,
    const S64ReuRuntimeControl* control,
    S64CodeOverlayReader reader
)
{
    if (namespace_state == 0 || namespace_state->count == 0u) {
        s64_code_overlay_configure(0, 0, 0, 0, 0, 0);
        return 0u;
    }

    return s64_code_overlay_configure(
        control,
        namespace_state->caches,
        namespace_state->count,
        (void*)S64_CODE_OVERLAY_ADDRESS,
        S64_CODE_OVERLAY_CAPACITY,
        reader
    );
}
