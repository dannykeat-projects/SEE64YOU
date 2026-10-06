#ifndef SEE64YOU_MANAGED_EDITOR_PUBLISHER_ASSETS_H
#define SEE64YOU_MANAGED_EDITOR_PUBLISHER_ASSETS_H

#include <stdint.h>
#include "platform/module_runtime.h"

/*
 * Prepare the exact Publisher overlay namespace while the managed Editor owns
 * an ACTIVE session in IO_SAFE mode. This is intentionally an Editor PREP
 * overlay operation, not launcher-resident code.
 *
 * Returns 1 only after the Publisher manifest, allocations, asset CRCs and S64Q
 * session record have all been readback-verified. Failure leaves transition
 * unauthorized; the producer rolls the runtime-control allocation plan back
 * when possible.
 */
uint8_t s64_managed_editor_pub_assets_prepare(
    S64ModuleRuntime* runtime,
    uint8_t* scratch
);

/* PREP-only exact Publisher transfer capsule producer. Caller owns IO_SAFE and
 * supplies the synchronous $C800 workspace. Success leaves the verified wire
 * in scratch for s64_module_transfer_capsule_enter(). */
uint8_t s64_managed_editor_publisher_capsule_store(
    S64ModuleRuntime* runtime,
    uint8_t* scratch
);

#endif
