#include "platform/basic_return_internal.h"
#include "platform/crc32.h"
#include "platform/reu.h"
#include <string.h>

uint8_t s64_basic_return_reserve(S64ReuRuntimeControl* control)
{
    S64ReuAllocation item;
    uint8_t i;
    if (!s64_reu_runtime_valid(control)) return 0;
    for (i = 0; i < control->allocation_count; ++i)
        if (control->allocations[i].owner == S64_REU_OWNER_RUNTIME
            && control->allocations[i].flags == S64_BASIC_RETURN_FLAG) return 0;
    if (!s64_reu_runtime_allocate(control, S64_REU_OWNER_RUNTIME,
            S64_BASIC_RETURN_RECORD_SIZE, 256u, &item)) return 0;
    control->allocations[control->allocation_count - 1].flags = S64_BASIC_RETURN_FLAG;
    return 1;
}

uint8_t s64_basic_return_publish(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, uint8_t* workspace)
{
    const S64ReuAllocation* item = s64_basic_return_allocation(control);
    uint32_t crc;
    uint8_t i;
    if (!workspace || !item || !s64_basic_return_header_valid(workspace)
        || !s64_basic_return_active(control, journal, workspace + 8)) return 0;
    crc = s64_crc32_update(0xffffffffUL, workspace, S64_BASIC_RETURN_RECORD_SIZE - 4) ^ 0xffffffffUL;
    for (i = 0; i < 4; ++i) {
        workspace[S64_BASIC_RETURN_RECORD_SIZE - 4 + i] = (uint8_t)crc;
        crc >>= 8;
    }
    return s64_reu_store((uint8_t)(item->address >> 16), (uint16_t)item->address,
            workspace, S64_BASIC_RETURN_RECORD_SIZE)
        && s64_reu_load((uint8_t)(item->address >> 16), (uint16_t)item->address,
            workspace + S64_BASIC_RETURN_RECORD_SIZE, S64_BASIC_RETURN_RECORD_SIZE)
        && memcmp(workspace, workspace + S64_BASIC_RETURN_RECORD_SIZE,
            S64_BASIC_RETURN_RECORD_SIZE) == 0;
}
