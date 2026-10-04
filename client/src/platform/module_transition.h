#ifndef SEE64YOU_PLATFORM_MODULE_TRANSITION_H
#define SEE64YOU_PLATFORM_MODULE_TRANSITION_H
#include "platform/module_runtime.h"
#include "platform/module_transfer.h"

typedef enum {
    S64_MODULE_TRANSITION_OK = 0,
    S64_MODULE_TRANSITION_INVALID,
    S64_MODULE_TRANSITION_FAILED,
    S64_MODULE_TRANSITION_RECOVERY_REQUIRED
} S64ModuleTransitionResult;

/* Existing adopted owner only. No SD read, allocation, capture or journal save.
 * OK publishes a readback-validated destination capsule and plan at TRANSITION.
 * FAILED cancels to INTERACTIVE; RECOVERY_REQUIRED blocks further execution. */
S64ModuleTransitionResult s64_module_transition_prepare(
    S64ModuleRuntime* runtime, S64ModuleId destination, S64ModuleTransferPlan* plan);
/* Revoke a prepared/failed candidate before resuming the source owner. */
S64ModuleTransitionResult s64_module_transition_cancel(
    S64ModuleRuntime* runtime, S64ModuleTransferPlan* plan);
#endif
