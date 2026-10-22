#include "platform/reu_runtime.h"
#include "platform/reu_runtime_internal.h"
#include "platform/reu.h"
#include <string.h>

static void put_u16(uint8_t* out, uint16_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
}

static void put_u32(uint8_t* out, uint32_t value)
{
    put_u16(out, (uint16_t)value);
    put_u16(out + 2u, (uint16_t)(value >> 16));
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


static uint8_t verify_wire(const uint8_t* wire)
{
    uint8_t readback[S64_REU_RUNTIME_WIRE_SIZE];
    /* Compare the complete canonical record, including reserved bytes. */
    return s64_reu_load(S64_REU_RUNTIME_CONTROL_BANK,
            S64_REU_RUNTIME_CONTROL_BASE, readback, sizeof(readback))
        && !memcmp(wire, readback, sizeof(readback));
}

uint8_t s64_reu_runtime_store_verified(const S64ReuRuntimeControl* control)
{
    uint8_t wire[S64_REU_RUNTIME_WIRE_SIZE];
    if (!s64_reu_runtime_encode(control, wire)
        || !s64_reu_store(S64_REU_RUNTIME_CONTROL_BANK,
            S64_REU_RUNTIME_CONTROL_BASE, wire, sizeof(wire))) return 0u;
    return verify_wire(wire);
}
