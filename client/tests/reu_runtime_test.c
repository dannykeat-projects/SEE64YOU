#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/reu_runtime.h"
#include "platform/reu_runtime_internal.h"

static unsigned char fake_reu_control[S64_REU_RUNTIME_CONTROL_CAPACITY];
static unsigned char fake_store_ok = 1u;
static unsigned char fake_load_ok = 1u;
static int corrupt_readback = -1;

uint8_t s64_reu_store(
    uint8_t bank,
    uint16_t address,
    const void* source,
    uint16_t length
)
{
    if (!fake_store_ok
        || bank != S64_REU_RUNTIME_CONTROL_BANK
        || address != S64_REU_RUNTIME_CONTROL_BASE
        || source == 0
        || length > sizeof(fake_reu_control)) {
        return 0u;
    }
    memcpy(fake_reu_control, source, length);
    return 1u;
}

uint8_t s64_reu_load(
    uint8_t bank,
    uint16_t address,
    void* destination,
    uint16_t length
)
{
    if (!fake_load_ok
        || bank != S64_REU_RUNTIME_CONTROL_BANK
        || address != S64_REU_RUNTIME_CONTROL_BASE
        || destination == 0
        || length > sizeof(fake_reu_control)) {
        return 0u;
    }
    memcpy(destination, fake_reu_control, length);
    if (corrupt_readback >= address && corrupt_readback < address + length)
        ((uint8_t*)destination)[corrupt_readback - address] ^= 1;
    return 1u;
}

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

int main(void)
{
    S64ReuRuntimeControl control;
    S64ReuRuntimeControl decoded;
    S64ReuAllocation first;
    S64ReuAllocation second;
    uint8_t wire[S64_REU_RUNTIME_WIRE_SIZE];
    uint32_t initial_remaining;
    uint8_t i;
    unsigned corrupt;
    uint16_t checksum;
    S64ReuRuntimeControl before;

    s64_reu_runtime_init(&control, 0x0201u, 0x11223344UL);
    require(s64_reu_runtime_valid(&control), "new control block is valid");
    require(control.next_free == S64_REU_RUNTIME_HEAP_START,
        "allocator starts after reserved control block");
    initial_remaining = s64_reu_runtime_remaining(&control);

    require(
        s64_reu_runtime_allocate(
            &control, S64_REU_OWNER_MODULE, 4096UL, 256u, &first
        ),
        "first aligned module allocation succeeds"
    );
    require(first.address == S64_REU_RUNTIME_HEAP_START,
        "first allocation begins at heap start");
    require(first.length == 4096UL && first.owner == S64_REU_OWNER_MODULE,
        "first allocation metadata recorded");

    require(
        s64_reu_runtime_allocate(
            &control, S64_REU_OWNER_RECOVERY, 1000UL, 1024u, &second
        ),
        "second aligned recovery allocation succeeds"
    );
    require((second.address & 1023UL) == 0UL,
        "second allocation obeys alignment");
    require(second.address >= first.address + first.length,
        "allocations do not overlap");
    require(control.allocation_count == 2u,
        "allocation count tracks ownership entries");
    require(s64_reu_runtime_remaining(&control) < initial_remaining,
        "remaining capacity decreases");

    require(s64_reu_runtime_encode(&control, wire),
        "control block encodes");
    require(s64_reu_runtime_decode(&decoded, wire),
        "control block decodes");
    require(decoded.session_id == control.session_id
        && decoded.app_version == control.app_version,
        "version/session metadata round trips");
    require(decoded.allocation_count == 2u
        && decoded.allocations[1].address == second.address,
        "allocation table round trips");

    control.state_flags = S64_REU_RUNTIME_STATE_APPLICATION_VALID;
    require(s64_reu_runtime_valid(&control),
        "known application-state validity flag is accepted");
    require(s64_reu_runtime_encode(&control, wire)
        && s64_reu_runtime_decode(&decoded, wire)
        && decoded.state_flags == S64_REU_RUNTIME_STATE_APPLICATION_VALID,
        "application-state validity flag round trips");
    control.state_flags = 0u;

    wire[30] ^= 0x80u;
    require(!s64_reu_runtime_decode(&decoded, wire),
        "checksum rejects corrupted control block");

    require(s64_reu_runtime_encode(&control, wire),
        "control block re-encodes before state-flag rejection test");
    wire[12] = 0x80u;
    checksum = s64_reu_runtime_wire_checksum(wire);
    wire[S64_REU_RUNTIME_CHECKSUM_OFFSET] = (uint8_t)checksum;
    wire[S64_REU_RUNTIME_CHECKSUM_OFFSET + 1u] = (uint8_t)(checksum >> 8);
    before = decoded;
    require(!s64_reu_runtime_decode(&decoded, wire),
        "unknown runtime state flags are rejected");
    require(memcmp(&before, &decoded, sizeof(decoded)) == 0,
        "rejected state flags preserve destination control");

    memset(fake_reu_control, 0, sizeof(fake_reu_control));
    require(s64_reu_runtime_store_verified(&control),
        "control block stores and verifies through REU DMA boundary");
    for (corrupt = 0; corrupt < S64_REU_RUNTIME_WIRE_SIZE; ++corrupt) {
        corrupt_readback = (int)corrupt;
        require(!s64_reu_runtime_store_verified(&control), "every control readback byte is verified");
    }
    corrupt_readback = -1;
    memset(&decoded, 0, sizeof(decoded));
    require(s64_reu_runtime_load(&decoded),
        "control block loads from reserved REU region");
    require(decoded.session_id == control.session_id
        && decoded.allocation_count == control.allocation_count,
        "REU-loaded control block matches active metadata");
    require(fake_reu_control[0] == S64_REU_RUNTIME_MAGIC_0
        && fake_reu_control[3] == S64_REU_RUNTIME_MAGIC_3,
        "reserved REU region contains wire-format magic");

    fake_store_ok = 0u;
    require(!s64_reu_runtime_store_verified(&control),
        "REU write failure is surfaced");
    fake_store_ok = 1u;
    fake_load_ok = 0u;
    require(!s64_reu_runtime_store_verified(&control),
        "REU readback failure is surfaced");
    fake_load_ok = 1u;

    s64_reu_runtime_init(&control, 1u, 1UL);
    require(!s64_reu_runtime_allocate(
        &control, S64_REU_OWNER_NONE, 1UL, 1u, 0
    ), "owner zero is rejected");
    require(!s64_reu_runtime_allocate(
        &control, S64_REU_OWNER_MODULE, 0UL, 1u, 0
    ), "zero length is rejected");
    require(!s64_reu_runtime_allocate(
        &control, S64_REU_OWNER_MODULE, 1UL, 3u, 0
    ), "non-power-of-two alignment is rejected");

    s64_reu_runtime_init(&control, 0x0202u, 2UL);
    for (i = 0u; i < S64_REU_RUNTIME_MAX_ALLOCATIONS; ++i) {
        require(s64_reu_runtime_allocate(&control, S64_REU_OWNER_MODULE,
            15360UL, 256u, &first), "every ownership slot can be allocated");
        control.allocations[i].flags = (uint8_t)(i + 1u);
    }
    require(control.allocation_count >= 13u,
        "table accommodates existing ownership and Publisher overlays");
    before = control;
    require(!s64_reu_runtime_allocate(&control, S64_REU_OWNER_MODULE,
        1UL, 1u, &first), "full table rejects another allocation");
    require(memcmp(&before, &control, sizeof(control)) == 0,
        "table exhaustion preserves all ownership metadata");
    memset(fake_reu_control, 0xa5, sizeof(fake_reu_control));
    require(s64_reu_runtime_store_verified(&control), "full table verifies through DMA");
    require(s64_reu_runtime_load(&decoded)
        && memcmp(control.allocations, decoded.allocations, sizeof(control.allocations)) == 0,
        "all ownership entries round trip");
    require(fake_reu_control[S64_REU_RUNTIME_WIRE_SIZE] == 0xa5u,
        "DMA preserves reserved bytes after the control record");
    require(s64_reu_runtime_encode(&control, wire), "full table encodes");
    wire[4] = 1u;
    checksum = s64_reu_runtime_wire_checksum(wire);
    wire[S64_REU_RUNTIME_CHECKSUM_OFFSET] = (uint8_t)checksum;
    wire[S64_REU_RUNTIME_CHECKSUM_OFFSET + 1u] = (uint8_t)(checksum >> 8);
    before = decoded;
    require(!s64_reu_runtime_decode(&decoded, wire), "old control format rejects even valid checksum");
    require(memcmp(&before, &decoded, sizeof(decoded)) == 0,
        "rejected old format preserves destination state");
    s64_reu_runtime_init(&control, 1u, 1UL);

    control.next_free = S64_REU_RUNTIME_HEAP_END - 16UL;
    require(!s64_reu_runtime_allocate(
        &control, S64_REU_OWNER_MODULE, 32UL, 1u, 0
    ), "allocation beyond 16 MiB is rejected");

    puts("REU runtime allocator tests passed");
    return 0;
}
