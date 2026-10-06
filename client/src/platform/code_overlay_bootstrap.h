#ifndef SEE64YOU_CODE_OVERLAY_BOOTSTRAP_H
#define SEE64YOU_CODE_OVERLAY_BOOTSTRAP_H
#include "platform/code_overlay.h"

/* Final link order: MODEL/FILE/RUNTIME, then Editor-only PUBLICATION. */
extern const uint16_t s64_code_overlay_linked_lengths[S64_CODE_OVERLAY_COUNT];

/* Resident entry prerequisite, not session adoption. The verified launcher
 * must have published READY assets before invoking the one-shot copier.
 * Persistent control bytes alone never authorize loading executable code.
 * Caller must validate the complete asset/session record after this succeeds,
 * before calling editor code or resuming INTERACTIVE. Reader stays resident. */
uint8_t s64_code_overlay_bootstrap(const S64ReuRuntimeControl* control,
    uint8_t trusted_arrival, uint16_t app_version,
    const uint16_t lengths[S64_CODE_OVERLAY_COUNT], void* slot,
    S64CodeOverlayReader reader);
#endif
