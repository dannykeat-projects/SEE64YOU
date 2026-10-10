#include "platform/reu_runtime.h"
#include "platform/reu_runtime_internal.h"

static uint8_t alignment_valid(uint16_t alignment)
{
    return alignment != 0u && (alignment & (uint16_t)(alignment - 1u)) == 0u;
}

static uint32_t align_up(uint32_t value, uint16_t alignment)
{
    uint32_t mask = (uint32_t)alignment - 1UL;
    return (value + mask) & ~mask;
}

uint8_t s64_reu_runtime_allocate(
    S64ReuRuntimeControl* control,
    S64ReuOwner owner,
    uint32_t length,
    uint16_t alignment,
    S64ReuAllocation* allocation
)
{
    uint32_t start;
    S64ReuAllocation* item;

    if (!s64_reu_runtime_valid(control)
        || !s64_reu_runtime_owner_valid((uint8_t)owner)
        || length == 0UL
        || !alignment_valid(alignment)
        || control->allocation_count >= S64_REU_RUNTIME_MAX_ALLOCATIONS) {
        return 0u;
    }

    start = align_up(control->next_free, alignment);
    if (start < control->next_free
        || start >= S64_REU_RUNTIME_HEAP_END
        || length > S64_REU_RUNTIME_HEAP_END - start) {
        return 0u;
    }

    item = &control->allocations[control->allocation_count];
    item->owner = (uint8_t)owner;
    item->flags = 0u;
    item->address = start;
    item->length = length;

    ++control->allocation_count;
    control->next_free = start + length;
    if (allocation != 0) {
        *allocation = *item;
    }
    return 1u;
}

