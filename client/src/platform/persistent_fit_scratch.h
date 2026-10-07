#ifndef SEE64YOU_PLATFORM_PERSISTENT_FIT_SCRATCH_H
#define SEE64YOU_PLATFORM_PERSISTENT_FIT_SCRATCH_H

#include <stdint.h>
#include "platform/basic_return.h"

/*
 * Diagnostic-only proof of the persistent-runtime RAM model.
 *
 * The validated BASIC-return record occupies the first
 * S64_BASIC_RETURN_WORKSPACE_SIZE bytes of $C800-$CBFF during clean exit.
 * The remaining bytes are a synchronous, non-reentrant scratch lane. The
 * production pivot may adopt this layout only after the fit target and exit
 * ordering have been validated; ordinary builds do not use these aliases.
 */
#define S64_PERSISTENT_FIT_SCRATCH_BASE \
    (S64_BASIC_RETURN_WORKSPACE + S64_BASIC_RETURN_WORKSPACE_SIZE)
#define S64_PERSISTENT_FIT_SCRATCH_CAPACITY \
    (1024u - S64_BASIC_RETURN_WORKSPACE_SIZE)

typedef char S64PersistentFitScratchPositive[
    S64_PERSISTENT_FIT_SCRATCH_CAPACITY
];

#endif
