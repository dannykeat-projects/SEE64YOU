#include "platform/reu_runtime.h"
#include "platform/reu_runtime_internal.h"
#include "platform/reu.h"
#include <string.h>

static uint8_t alignment_valid(uint16_t alignment)
{
    return alignment != 0u && (alignment & (uint16_t)(alignment - 1u)) == 0u;
}

static uint32_t align_up(uint32_t value, uint16_t alignment)
{
    uint32_t mask = (uint32_t)alignment - 1UL;
    return (value + mask) & ~mask;
}

static void put_u16(uint8_t* out, uint16_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
}

static void put_u32(uint8_t* out, uint32_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
    out[2] = (uint8_t)(value >> 16);
    out[3] = (uint8_t)(value >> 24);
}


void s64_reu_runtime_init(
    S64ReuRuntimeControl* control,
    uint16_t app_version,
    uint32_t session_id
)
{
    if (control == 0) return;

    memset(control, 0, sizeof(*control));
    control->format_version = S64_REU_RUNTIME_FORMAT_VERSION;
    control->abi_version = S64_REU_RUNTIME_ABI_VERSION;
    control->app_version = app_version;
    control->session_id = session_id;
    control->next_free = S64_REU_RUNTIME_HEAP_START;
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

uint32_t s64_reu_runtime_remaining(const S64ReuRuntimeControl* control)
{
    if (!s64_reu_runtime_valid(control)) return 0UL;
    return S64_REU_RUNTIME_HEAP_END - control->next_free;
}

uint8_t s64_reu_runtime_encode(
    const S64ReuRuntimeControl* control,
    uint8_t wire[S64_REU_RUNTIME_WIRE_SIZE]
)
{
    uint8_t i;
    uint16_t offset;

    if (wire == 0 || !s64_reu_runtime_valid(control)) return 0u;

    memset(wire, 0, S64_REU_RUNTIME_WIRE_SIZE);
    wire[0] = S64_REU_RUNTIME_MAGIC_0;
    wire[1] = S64_REU_RUNTIME_MAGIC_1;
    wire[2] = S64_REU_RUNTIME_MAGIC_2;
    wire[3] = S64_REU_RUNTIME_MAGIC_3;
    wire[4] = control->format_version;
    wire[5] = control->abi_version;
    put_u16(&wire[6], control->app_version);
    put_u32(&wire[8], control->session_id);
    wire[12] = control->state_flags;
    wire[13] = control->allocation_count;
    put_u32(&wire[16], control->next_free);
    put_u32(&wire[20], S64_REU_RUNTIME_HEAP_END);

    offset = S64_REU_RUNTIME_HEADER_SIZE;
    for (i = 0u; i < control->allocation_count; ++i) {
        wire[offset] = control->allocations[i].owner;
        wire[offset + 1u] = control->allocations[i].flags;
        put_u32(&wire[offset + 2u], control->allocations[i].address);
        put_u32(&wire[offset + 6u], control->allocations[i].length);
        offset = (uint16_t)(offset + S64_REU_RUNTIME_ENTRY_SIZE);
    }

    put_u16(&wire[S64_REU_RUNTIME_CHECKSUM_OFFSET], s64_reu_runtime_wire_checksum(wire));
    return 1u;
}


uint8_t s64_reu_runtime_store(const S64ReuRuntimeControl* control)
{
    uint8_t wire[S64_REU_RUNTIME_WIRE_SIZE];

    if (!s64_reu_runtime_encode(control, wire)) return 0u;
    return s64_reu_store(
        S64_REU_RUNTIME_CONTROL_BANK,
        S64_REU_RUNTIME_CONTROL_BASE,
        wire,
        S64_REU_RUNTIME_WIRE_SIZE
    );
}


uint8_t s64_reu_runtime_store_verified(const S64ReuRuntimeControl* control)
{
    S64ReuRuntimeControl readback;

    if (!s64_reu_runtime_store(control)
        || !s64_reu_runtime_load(&readback)) {
        return 0u;
    }

    return readback.format_version == control->format_version
        && readback.abi_version == control->abi_version
        && readback.app_version == control->app_version
        && readback.session_id == control->session_id
        && readback.state_flags == control->state_flags
        && readback.allocation_count == control->allocation_count
        && readback.next_free == control->next_free
        && memcmp(
            readback.allocations,
            control->allocations,
            sizeof(control->allocations)
        ) == 0;
}
