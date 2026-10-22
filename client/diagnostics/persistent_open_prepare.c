#include "publisher/persistent_services.h"

#include <string.h>

uint8_t s64_persistent_open_stage_init(
    S64PersistentOpenStage* stage, S64ReuRuntimeControl* control)
{
    S64ReuAllocation* allocation = 0;
    uint8_t i;

    if (!stage) return 0u;
    memset(stage, 0, sizeof(*stage));
    stage->result = S64_DOCUMENT_SINK_ERROR;
    if (!s64_reu_runtime_valid(control)) return 0u;
    for (i = 0u; i < control->allocation_count; ++i) {
        if (control->allocations[i].owner == S64_REU_OWNER_PUBLISHER
            && control->allocations[i].flags == S64_OPEN_STORAGE_FLAG) {
            if (allocation) return 0u;
            allocation = &control->allocations[i];
        }
    }
    if (!allocation) {
        if (!s64_reu_runtime_allocate(control, S64_REU_OWNER_PUBLISHER,
                S64_OPEN_STORAGE_SIZE, 256u, 0)) return 0u;
        allocation = &control->allocations[control->allocation_count - 1u];
        allocation->flags = S64_OPEN_STORAGE_FLAG;
    }
    if (allocation->length != S64_OPEN_STORAGE_SIZE
        || (allocation->address & 255UL)) return 0u;
    stage->address = allocation->address;
    stage->line_empty = 1u;
    stage->result = S64_DOCUMENT_OK;
    return 1u;
}
