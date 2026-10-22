#ifndef SEE64YOU_REU_RUNTIME_INTERNAL_H
#define SEE64YOU_REU_RUNTIME_INTERNAL_H
#include <stdint.h>
#include "platform/reu_runtime.h"

#define S64_REU_RUNTIME_HEADER_SIZE 24u
#define S64_REU_RUNTIME_ENTRY_SIZE 10u
#define S64_REU_RUNTIME_CHECKSUM_OFFSET (S64_REU_RUNTIME_WIRE_SIZE - 2u)

#if S64_REU_RUNTIME_HEADER_SIZE + S64_REU_RUNTIME_ENTRY_SIZE * S64_REU_RUNTIME_MAX_ALLOCATIONS > S64_REU_RUNTIME_CHECKSUM_OFFSET
#error REU allocation table exceeds its wire record
#endif
#if S64_REU_RUNTIME_WIRE_SIZE > S64_REU_RUNTIME_CONTROL_CAPACITY
#error REU control record exceeds its reserved region
#endif

/* Shared validation/codec helpers; not part of the module ABI. */
uint8_t s64_reu_runtime_owner_valid(uint8_t owner);
uint16_t s64_reu_runtime_wire_checksum(const uint8_t* data);
#endif
