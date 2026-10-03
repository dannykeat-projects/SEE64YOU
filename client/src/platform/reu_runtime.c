#include "platform/reu_runtime.h"

#include <string.h>

#include "platform/reu.h"

#include "platform/reu_runtime_internal.h"

uint8_t s64_reu_runtime_owner_valid(uint8_t owner)
{
    return owner >= S64_REU_OWNER_RUNTIME && owner <= S64_REU_OWNER_UPDATE;
}

static uint16_t get_u16(const uint8_t* in)
{
    return (uint16_t)(in[0] | ((uint16_t)in[1] << 8));
}

static uint32_t get_u32(const uint8_t* in)
{
    return (uint32_t)in[0]
        | ((uint32_t)in[1] << 8)
        | ((uint32_t)in[2] << 16)
        | ((uint32_t)in[3] << 24);
}

uint16_t s64_reu_runtime_wire_checksum(const uint8_t* data)
{
    uint16_t value = 0x64a5u;
    uint16_t i;

    for (i = 0u; i < S64_REU_RUNTIME_CHECKSUM_OFFSET; ++i) {
        value = (uint16_t)((value << 5) | (value >> 11));
        value = (uint16_t)(value ^ data[i]);
    }
    return value == 0u ? 1u : value;
}

uint8_t s64_reu_runtime_valid(const S64ReuRuntimeControl* control)
{
    uint8_t i;
    uint32_t expected = S64_REU_RUNTIME_HEAP_START;

    if (control == 0
        || control->format_version != S64_REU_RUNTIME_FORMAT_VERSION
        || control->abi_version != S64_REU_RUNTIME_ABI_VERSION
        || control->session_id == 0UL
        || control->allocation_count > S64_REU_RUNTIME_MAX_ALLOCATIONS
        || control->next_free < S64_REU_RUNTIME_HEAP_START
        || control->next_free > S64_REU_RUNTIME_HEAP_END) {
        return 0u;
    }

    for (i = 0u; i < control->allocation_count; ++i) {
        const S64ReuAllocation* item = &control->allocations[i];

        if (!s64_reu_runtime_owner_valid(item->owner)
            || item->length == 0UL
            || item->address < expected
            || item->address < S64_REU_RUNTIME_HEAP_START
            || item->address >= S64_REU_RUNTIME_HEAP_END
            || item->length > S64_REU_RUNTIME_HEAP_END - item->address) {
            return 0u;
        }
        expected = item->address + item->length;
    }

    return control->next_free >= expected;
}

uint8_t s64_reu_runtime_decode(
    S64ReuRuntimeControl* control,
    const uint8_t wire[S64_REU_RUNTIME_WIRE_SIZE]
)
{
    S64ReuRuntimeControl decoded;
    uint16_t stored_checksum;
    uint16_t offset;
    uint8_t i;

    if (control == 0 || wire == 0
        || wire[0] != S64_REU_RUNTIME_MAGIC_0
        || wire[1] != S64_REU_RUNTIME_MAGIC_1
        || wire[2] != S64_REU_RUNTIME_MAGIC_2
        || wire[3] != S64_REU_RUNTIME_MAGIC_3) {
        return 0u;
    }

    stored_checksum = get_u16(&wire[S64_REU_RUNTIME_CHECKSUM_OFFSET]);
    if (stored_checksum == 0u || stored_checksum != s64_reu_runtime_wire_checksum(wire)) {
        return 0u;
    }
    if (get_u32(&wire[20]) != S64_REU_RUNTIME_HEAP_END) {
        return 0u;
    }

    memset(&decoded, 0, sizeof(decoded));
    decoded.format_version = wire[4];
    decoded.abi_version = wire[5];
    decoded.app_version = get_u16(&wire[6]);
    decoded.session_id = get_u32(&wire[8]);
    decoded.state_flags = wire[12];
    decoded.allocation_count = wire[13];
    decoded.next_free = get_u32(&wire[16]);

    if (decoded.allocation_count > S64_REU_RUNTIME_MAX_ALLOCATIONS) {
        return 0u;
    }

    offset = S64_REU_RUNTIME_HEADER_SIZE;
    for (i = 0u; i < decoded.allocation_count; ++i) {
        decoded.allocations[i].owner = wire[offset];
        decoded.allocations[i].flags = wire[offset + 1u];
        decoded.allocations[i].address = get_u32(&wire[offset + 2u]);
        decoded.allocations[i].length = get_u32(&wire[offset + 6u]);
        offset = (uint16_t)(offset + S64_REU_RUNTIME_ENTRY_SIZE);
    }

    if (!s64_reu_runtime_valid(&decoded)) return 0u;
    *control = decoded;
    return 1u;
}


uint8_t s64_reu_runtime_load(S64ReuRuntimeControl* control)
{
    uint8_t wire[S64_REU_RUNTIME_WIRE_SIZE];

    if (control == 0
        || !s64_reu_load(
            S64_REU_RUNTIME_CONTROL_BANK,
            S64_REU_RUNTIME_CONTROL_BASE,
            wire,
            S64_REU_RUNTIME_WIRE_SIZE
        )) {
        return 0u;
    }
    return s64_reu_runtime_decode(control, wire);
}
