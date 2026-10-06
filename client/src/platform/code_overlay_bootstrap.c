#include "platform/code_overlay_bootstrap.h"
#include "platform/code_assets.h"

uint8_t s64_code_overlay_bootstrap(const S64ReuRuntimeControl* control,
    uint8_t trusted_arrival, uint16_t app_version,
    const uint16_t lengths[S64_CODE_OVERLAY_COUNT], void* slot,
    S64CodeOverlayReader reader)
{
    S64ReuAllocation caches[S64_CODE_OVERLAY_COUNT];
    uint8_t i, j, found;
    /* A failed or repeated entry must not retain an earlier selector. */
    s64_code_overlay_configure(0, 0, 0, 0, 0, 0);
    if (!trusted_arrival || !app_version || !lengths
        || !s64_reu_runtime_valid(control)
        || control->app_version != app_version) return 0u;
    for (i = 0u; i < S64_CODE_OVERLAY_COUNT; ++i) {
        if (!lengths[i] || lengths[i] > S64_CODE_OVERLAY_CAPACITY) return 0u;
        found = 0u;
        for (j = 0u; j < control->allocation_count; ++j) {
            const S64ReuAllocation* item = &control->allocations[j];
            if (item->flags != S64_CODE_ASSET_CACHE_FLAG + i
                || item->owner != S64_REU_OWNER_MODULE) continue;
            if (found || item->length != lengths[i]
                || (item->address & 255UL)) return 0u;
            caches[i] = *item;
            found = 1u;
        }
        if (!found) return 0u;
    }
    if (s64_code_overlay_configure(control, caches, S64_CODE_OVERLAY_COUNT, slot,
            S64_CODE_OVERLAY_CAPACITY, reader)
        && s64_code_overlay_select(3u)) return 1u;
    s64_code_overlay_configure(0, 0, 0, 0, 0, 0);
    return 0u;
}
