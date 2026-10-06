#ifndef SEE64YOU_MANAGED_PUBLISHER_H
#define SEE64YOU_MANAGED_PUBLISHER_H
#include "platform/module_runtime.h"
#include "document.h"

/* Called only after resident trusted-arrival bootstrap loads RUNTIME. */
uint8_t s64_managed_publisher_adopt(S64ModuleRuntime* runtime, S64Dos* dos,
    uint16_t version);
/* Consumes adopt's one-shot authority and checked workspace wire. Call before
 * reusing workspace. Failed calls leave outputs unchanged. No SD reads. */
uint8_t s64_managed_publisher_document(S64ModuleRuntime* runtime,
    S64Buffer* body, S64Document* document, uint8_t* dirty, uint8_t* replaced);
/* Prepare the authenticated shared state for Publisher -> Editor. Returns
 * 0 for a safely retryable failure, 1 for success, 2 for recovery-required. */
uint8_t s64_managed_publisher_return_state(S64ModuleRuntime* runtime,
    const S64Buffer* body, const S64Document* document, uint8_t replaced,
    uint8_t* wire);
#endif
