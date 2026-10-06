#ifndef SEE64YOU_CODE_OVERLAY_H
#define SEE64YOU_CODE_OVERLAY_H

#include "platform/reu_runtime.h"

#define S64_CODE_OVERLAY_MAX_COUNT 4u
#ifdef S64_CODE_ASSET_PUBLISHER
#define S64_CODE_OVERLAY_COUNT 3u
#else
#define S64_CODE_OVERLAY_COUNT S64_CODE_OVERLAY_MAX_COUNT
#endif
#define S64_CODE_OVERLAY_ADDRESS 0x8c00u
#define S64_CODE_OVERLAY_CAPACITY 0x3c00u

/* Reader code must stay resident while it overwrites the execution slot. */
typedef uint8_t (*S64CodeOverlayReader)(uint8_t, uint16_t, void*, uint16_t);

/* Only supply caches whose exact linked asset bytes have been verified. */
uint8_t s64_code_overlay_configure(
    const S64ReuRuntimeControl* control,
    const S64ReuAllocation* caches,
    uint8_t actual_count,
    void* slot,
    uint16_t capacity,
    S64CodeOverlayReader reader
);
uint8_t s64_code_overlay_select(uint8_t id);
#ifdef __CC65__
void __fastcall__ s64_code_overlay_enter(uint8_t id);
#endif

#endif
