#ifndef SEE64YOU_PLATFORM_REU_RUNTIME_H
#define SEE64YOU_PLATFORM_REU_RUNTIME_H

#include <stdint.h>

#include "platform/reu_layout.h"

#define S64_REU_RUNTIME_MAGIC_0 'S'
#define S64_REU_RUNTIME_MAGIC_1 '6'
#define S64_REU_RUNTIME_MAGIC_2 '4'
#define S64_REU_RUNTIME_MAGIC_3 'R'

#define S64_REU_RUNTIME_FORMAT_VERSION 2u
#define S64_REU_RUNTIME_ABI_VERSION 2u
#define S64_REU_RUNTIME_WIRE_SIZE 192u
#define S64_REU_RUNTIME_MAX_ALLOCATIONS 16u

#define S64_REU_RUNTIME_HEAP_START 0x00020100UL
#define S64_REU_RUNTIME_HEAP_END   0x01000000UL

typedef enum {
    S64_REU_OWNER_NONE = 0,
    S64_REU_OWNER_RUNTIME,
    S64_REU_OWNER_EDITOR,
    S64_REU_OWNER_HISTORY,
    S64_REU_OWNER_CLIPBOARD,
    S64_REU_OWNER_MODULE,
    S64_REU_OWNER_RECOVERY,
    S64_REU_OWNER_PUBLISHER,
    S64_REU_OWNER_PREVIEW,
    S64_REU_OWNER_UPDATE
} S64ReuOwner;

typedef struct {
    uint8_t owner;
    uint8_t flags;
    uint32_t address;
    uint32_t length;
} S64ReuAllocation;

typedef struct {
    uint8_t format_version;
    uint8_t abi_version;
    uint16_t app_version;
    uint32_t session_id;
    uint8_t state_flags;
    uint8_t allocation_count;
    uint32_t next_free;
    S64ReuAllocation allocations[S64_REU_RUNTIME_MAX_ALLOCATIONS];
} S64ReuRuntimeControl;

void s64_reu_runtime_init(
    S64ReuRuntimeControl* control,
    uint16_t app_version,
    uint32_t session_id
);

uint8_t s64_reu_runtime_valid(const S64ReuRuntimeControl* control);

uint8_t s64_reu_runtime_allocate(
    S64ReuRuntimeControl* control,
    S64ReuOwner owner,
    uint32_t length,
    uint16_t alignment,
    S64ReuAllocation* allocation
);

uint32_t s64_reu_runtime_remaining(const S64ReuRuntimeControl* control);

uint8_t s64_reu_runtime_encode(
    const S64ReuRuntimeControl* control,
    uint8_t wire[S64_REU_RUNTIME_WIRE_SIZE]
);

uint8_t s64_reu_runtime_decode(
    S64ReuRuntimeControl* control,
    const uint8_t wire[S64_REU_RUNTIME_WIRE_SIZE]
);

uint8_t s64_reu_runtime_store(const S64ReuRuntimeControl* control);
uint8_t s64_reu_runtime_load(S64ReuRuntimeControl* control);
uint8_t s64_reu_runtime_store_verified(const S64ReuRuntimeControl* control);

#endif
