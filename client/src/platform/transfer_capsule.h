#ifndef SEE64YOU_PLATFORM_TRANSFER_CAPSULE_H
#define SEE64YOU_PLATFORM_TRANSFER_CAPSULE_H

#include <stdint.h>
#include "platform/module_manifest.h"
#include "platform/session_journal.h"

#define S64_TRANSFER_CAPSULE_WIRE_SIZE 64u
#define S64_TRANSFER_CAPSULE_ALLOCATION_FLAG 2u
#define S64_TRANSFER_CAPSULE_FORMAT_VERSION 1u

typedef struct {
    S64ModuleId destination;
    S64SessionJournal journal;
    S64ModuleDescriptor module;
} S64TransferCapsule;

uint8_t s64_transfer_capsule_reserve(S64ReuRuntimeControl* control);
uint8_t s64_transfer_capsule_clear(const S64ReuRuntimeControl* control);
uint8_t s64_transfer_capsule_store(
    const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal,
    const S64ModuleManifest* manifest,
    S64ModuleId destination
);
uint8_t s64_transfer_capsule_load(
    const S64ReuRuntimeControl* control,
    S64TransferCapsule* capsule
);

#endif
