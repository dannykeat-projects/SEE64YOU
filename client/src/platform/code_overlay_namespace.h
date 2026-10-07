#ifndef SEE64YOU_PLATFORM_CODE_OVERLAY_NAMESPACE_H
#define SEE64YOU_PLATFORM_CODE_OVERLAY_NAMESPACE_H

#include "platform/code_overlay.h"

typedef struct {
    S64ReuAllocation caches[S64_CODE_OVERLAY_MAX_COUNT];
    uint8_t count;
} S64CodeOverlayNamespace;

/*
 * Snapshot already-verified allocator handles for one logical code namespace.
 * Activation revalidates every handle against the live allocator before it can
 * become executable.
 */
uint8_t s64_code_overlay_namespace_capture(
    S64CodeOverlayNamespace* namespace_state,
    const S64ReuRuntimeControl* control,
    const S64ReuAllocation* caches,
    uint8_t count
);

uint8_t s64_code_overlay_namespace_activate(
    const S64CodeOverlayNamespace* namespace_state,
    const S64ReuRuntimeControl* control,
    S64CodeOverlayReader reader
);

void s64_code_overlay_namespace_clear(S64CodeOverlayNamespace* namespace_state);

#endif
