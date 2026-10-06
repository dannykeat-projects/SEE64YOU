#ifndef SEE64YOU_PLATFORM_MODULE_SESSION_INTERNAL_H
#define SEE64YOU_PLATFORM_MODULE_SESSION_INTERNAL_H

#include "platform/module_session.h"
#ifdef S64_MANAGED_OVERLAY
#include "platform/basic_return.h"
#endif

#define S64_MODULE_SESSION_JOURNAL_OFFSET 8u
#define S64_MODULE_SESSION_MODULE_OFFSET 32u
#define S64_MODULE_SESSION_MODULE_SIZE 26u
#define S64_MODULE_SESSION_CHECKSUM_OFFSET (S64_MODULE_SESSION_WIRE_SIZE - 2u)

#ifdef S64_MANAGED_OVERLAY
#define s64_module_session_wire     (S64_BASIC_RETURN_WORKSPACE)
#define s64_module_session_readback     (S64_BASIC_RETURN_WORKSPACE + S64_MODULE_SESSION_WIRE_SIZE)
#define s64_module_session_journal_wire     (S64_BASIC_RETURN_WORKSPACE + 2u * S64_MODULE_SESSION_WIRE_SIZE)
#else
extern uint8_t s64_module_session_wire[S64_MODULE_SESSION_WIRE_SIZE];
extern uint8_t s64_module_session_readback[S64_MODULE_SESSION_WIRE_SIZE];
extern uint8_t s64_module_session_journal_wire[S64_SESSION_JOURNAL_WIRE_SIZE];
#endif

const S64ReuAllocation* s64_module_session_allocation(
    const S64ReuRuntimeControl* control);
uint8_t s64_module_session_active_session(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal);
uint8_t s64_module_session_package_ready(const S64ReuRuntimeControl* control,
    const S64ModuleManifest* manifest);
uint16_t s64_module_session_checksum(const uint8_t* bytes);

#endif
