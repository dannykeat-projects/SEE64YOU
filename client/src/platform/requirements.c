#include "requirements.h"

S64PlatformRequirement s64_platform_check_requirements(
    const S64PlatformRequirements* requirements
)
{
    if (requirements == 0 || !requirements->interface_present) {
        return S64_PLATFORM_MISSING_UCI;
    }
    if (!requirements->dos_target) {
        return S64_PLATFORM_MISSING_DOS;
    }
    if (!requirements->reu_present) {
        return S64_PLATFORM_MISSING_REU;
    }
    if (requirements->reu_banks != S64_PLATFORM_REQUIRED_REU_BANKS) {
        return S64_PLATFORM_WRONG_REU_SIZE;
    }
    return S64_PLATFORM_READY;
}
