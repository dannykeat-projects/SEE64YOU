#ifndef SEE64YOU_PLATFORM_REQUIREMENTS_H
#define SEE64YOU_PLATFORM_REQUIREMENTS_H

#include <stdint.h>

#define S64_PLATFORM_REQUIRED_REU_BANKS 256u

typedef enum {
    S64_PLATFORM_READY = 0,
    S64_PLATFORM_MISSING_UCI,
    S64_PLATFORM_MISSING_DOS,
    S64_PLATFORM_MISSING_REU,
    S64_PLATFORM_WRONG_REU_SIZE
} S64PlatformRequirement;

typedef struct {
    uint8_t interface_present;
    uint8_t dos_target;
    uint8_t reu_present;
    uint16_t reu_banks;
} S64PlatformRequirements;

S64PlatformRequirement s64_platform_check_requirements(
    const S64PlatformRequirements* requirements
);

#endif
