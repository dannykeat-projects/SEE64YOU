#ifndef SEE64YOU_MANAGED_EDITOR_H
#define SEE64YOU_MANAGED_EDITOR_H
#include "platform/module_runtime.h"
/* Returns only for failure or a clean exit without valid return context. */
uint8_t s64_managed_editor_exit(S64ModuleRuntime* runtime);
/* Called only after resident bootstrap loads verified RUNTIME on trusted
 * copier arrival. Failure leaves durable ACTIVE for launcher recovery. */
uint8_t s64_managed_editor_adopt(S64ModuleRuntime* runtime, S64Dos* dos,
    uint16_t version);
S64SessionLifecycleResult s64_managed_editor_end(S64ModuleRuntime* runtime);
#endif
