#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/reu_runtime.h"

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

    wire[30] ^= 0x80u;
    require(!s64_reu_runtime_decode(&decoded, wire),
        "checksum rejects corrupted control block");

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

    control.next_free = S64_REU_RUNTIME_HEAP_END - 16UL;
    require(!s64_reu_runtime_allocate(
        &control, S64_REU_OWNER_MODULE, 32UL, 1u, 0
    ), "allocation beyond 16 MiB is rejected");

    puts("REU runtime allocator tests passed");
    return 0;
}
