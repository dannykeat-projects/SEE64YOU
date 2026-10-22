#ifndef SEE64YOU_PLATFORM_MODULE_SESSION_H
#define SEE64YOU_PLATFORM_MODULE_SESSION_H
#include "platform/module_manifest.h"
#include "platform/session_journal.h"

#define S64_MODULE_SESSION_WIRE_SIZE 128u
#define S64_MODULE_SESSION_ALLOCATION_FLAG 1u

/* Reserve after module caches, before persisting the allocator control. */
uint8_t s64_module_session_reserve(S64ReuRuntimeControl* control);
/* Revoke shared readiness before changing cached bytes. Caller owns IO_SAFE. */
uint8_t s64_module_session_clear(const S64ReuRuntimeControl* control);
/* Caller owns IO_SAFE. Publish only the completely verified package. */
uint8_t s64_module_session_store_verified(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const S64ModuleManifest* manifest);
/* Caller supplies the current journal from its trusted active lifecycle or
 * verified handoff capsule. Cold inspection must read the newest DOS journal.
 * A CLEAN or mismatched journal rejects the record without changing output. */
uint8_t s64_module_session_load(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, S64ModuleManifest* manifest);
#endif
