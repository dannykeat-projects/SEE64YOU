#include "platform/code_overlay.h"
#include "platform/reu.h"
#ifdef __CC65__
#include <conio.h>
#endif

/* Retain verified transfer coordinates without unused allocator metadata. */
typedef struct {
    uint16_t offset;
    uint16_t length;
    uint8_t bank;
} S64CodeOverlayCache;
static S64CodeOverlayCache overlay_caches[S64_CODE_OVERLAY_COUNT];
static uint8_t overlay_count;
static void* overlay_slot;
static S64CodeOverlayReader overlay_reader;
static uint8_t overlay_active;

uint8_t s64_code_overlay_configure(
    const S64ReuRuntimeControl* control,
    const S64ReuAllocation* caches, uint8_t actual_count,
    void* slot, uint16_t capacity, S64CodeOverlayReader reader)
{
    uint8_t i;
    uint8_t j;
    uint8_t found;
    overlay_slot = 0;
    overlay_active = 0;
    overlay_count = 0;
    if ((actual_count != 3u && actual_count != 4u)
        || actual_count > S64_CODE_OVERLAY_COUNT) return 0;
    if (!control || !caches || !slot || !reader ||
        capacity != S64_CODE_OVERLAY_CAPACITY || !s64_reu_runtime_valid(control)) return 0;
#ifdef __CC65__
    if ((uint16_t)slot != S64_CODE_OVERLAY_ADDRESS) return 0;
#endif
    for (i = 0; i < actual_count; ++i) {
        if (caches[i].owner != S64_REU_OWNER_MODULE || !caches[i].length ||
            caches[i].length > capacity) return 0;
        found = 0;
        for (j = 0; j < control->allocation_count; ++j) {
            if (control->allocations[j].owner == caches[i].owner &&
                control->allocations[j].address == caches[i].address &&
                control->allocations[j].length == caches[i].length &&
                control->allocations[j].flags == caches[i].flags) found = 1;
        }
        if (!found) return 0;
        for (j = 0; j < i; ++j) {
            if (caches[j].address == caches[i].address) return 0;
        }
    }
    for (i = 0; i < actual_count; ++i) {
        overlay_caches[i].bank = (uint8_t)(caches[i].address >> 16);
        overlay_caches[i].offset = (uint16_t)caches[i].address;
        overlay_caches[i].length = (uint16_t)caches[i].length;
    }
    overlay_count = actual_count;
    overlay_reader = reader;
    overlay_slot = slot;
    return 1;
}

uint8_t s64_code_overlay_select(uint8_t id)
{
    S64CodeOverlayCache* cache;
    if (!overlay_slot || !id || id > overlay_count) {
        overlay_active = 0;
        return 0;
    }
    if (overlay_active == id) return 1;
    overlay_active = 0;
    cache = &overlay_caches[id - 1u];
    if (!overlay_reader(cache->bank, cache->offset,
        overlay_slot, cache->length)) return 0;
    overlay_active = id;
    return 1;
}

#ifdef __CC65__
void __fastcall__ s64_code_overlay_enter(uint8_t id)
{
    if (s64_code_overlay_select(id)) return;
    /* Never jump into missing or partially copied executable bytes. */
    clrscr();
    cputs("Code cache unavailable. Restart launcher.");
    for (;;) { }
}
#endif
