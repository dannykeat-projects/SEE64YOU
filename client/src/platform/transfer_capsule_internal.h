#ifndef S64_TRANSFER_CAPSULE_INTERNAL_H
#define S64_TRANSFER_CAPSULE_INTERNAL_H

#include "platform/transfer_capsule.h"
#include "platform/reu.h"
#ifdef S64_MANAGED_OVERLAY
#include "platform/basic_return.h"
#endif

#include <string.h>

#define CAPSULE_JOURNAL_OFFSET 8u
#define CAPSULE_MODULE_OFFSET 32u
#define CAPSULE_CHECKSUM_OFFSET 62u

uint16_t s64_transfer_capsule_checksum(const uint8_t* bytes);
const S64ReuAllocation* s64_transfer_capsule_allocation(const S64ReuRuntimeControl* control);
const char* capsule_filename(S64ModuleId destination);
uint8_t s64_transfer_capsule_module_valid(const S64ReuRuntimeControl* control,
    const S64ModuleDescriptor* module, S64ModuleId destination);
#ifdef S64_MANAGED_OVERLAY
#define capsule_wire (S64_BASIC_RETURN_WORKSPACE)
#define capsule_readback     (S64_BASIC_RETURN_WORKSPACE + S64_TRANSFER_CAPSULE_WIRE_SIZE)
#else
extern uint8_t capsule_wire[S64_TRANSFER_CAPSULE_WIRE_SIZE];
extern uint8_t capsule_readback[S64_TRANSFER_CAPSULE_WIRE_SIZE];
#endif

#endif
