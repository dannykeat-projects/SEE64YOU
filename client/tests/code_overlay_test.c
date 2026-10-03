#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "platform/code_overlay.h"

static unsigned char slot[S64_CODE_OVERLAY_CAPACITY];
static unsigned int reads;
static uint8_t fail_read;
uint8_t s64_reu_load(uint8_t bank, uint16_t offset, void* destination, uint16_t length)
{
    ++reads;
    assert(bank == 2 && offset >= 0x100);
    memset(destination, offset & 255, length);
    return !fail_read;
}

int main(void)
{
    S64ReuRuntimeControl control;
    S64ReuAllocation caches[3];
    uint8_t i;
    memset(&control, 0, sizeof(control));
    control.format_version = S64_REU_RUNTIME_FORMAT_VERSION;
    control.abi_version = S64_REU_RUNTIME_ABI_VERSION;
    control.session_id = 1;
    control.allocation_count = 3;
    for (i = 0; i < 3; ++i) {
        caches[i].owner = S64_REU_OWNER_MODULE;
        caches[i].flags = 0;
        caches[i].address = S64_REU_RUNTIME_HEAP_START + i * 0x100UL;
        caches[i].length = 0x100;
        control.allocations[i] = caches[i];
    }
    control.next_free = caches[2].address + caches[2].length;
    assert(!s64_code_overlay_select(1));
    assert(s64_code_overlay_configure(&control, caches, slot, sizeof(slot), s64_reu_load));
    assert(s64_code_overlay_select(1) && reads == 1);
    assert(s64_code_overlay_select(1) && reads == 1);
    assert(s64_code_overlay_select(2) && reads == 2);
    fail_read = 1;
    assert(!s64_code_overlay_select(3) && reads == 3);
    fail_read = 0;
    assert(s64_code_overlay_select(2) && reads == 4);
    assert(!s64_code_overlay_select(0));
    assert(!s64_code_overlay_select(4));
    caches[2] = caches[1];
    assert(!s64_code_overlay_configure(&control, caches, slot, sizeof(slot), s64_reu_load));
    assert(!s64_code_overlay_select(2));
    caches[2] = control.allocations[2];
    caches[1].owner = S64_REU_OWNER_EDITOR;
    assert(!s64_code_overlay_configure(&control, caches, slot, sizeof(slot), s64_reu_load));
    caches[1] = control.allocations[1];
    caches[1].length = S64_CODE_OVERLAY_CAPACITY + 1UL;
    assert(!s64_code_overlay_configure(&control, caches, slot, sizeof(slot), s64_reu_load));
    caches[1] = control.allocations[1];
    ++caches[1].address;
    assert(!s64_code_overlay_configure(&control, caches, slot, sizeof(slot), s64_reu_load));
    puts("code overlay tests passed");
    return 0;
}
