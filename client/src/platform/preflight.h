#ifndef SEE64YOU_PLATFORM_PREFLIGHT_H
#define SEE64YOU_PLATFORM_PREFLIGHT_H

#include <stdint.h>

#include "platform/requirements.h"

typedef struct {
    S64PlatformRequirement requirement;
    uint8_t storage_ready;
} S64PlatformPreflight;

void s64_platform_preflight_run(S64PlatformPreflight* preflight);

#endif
