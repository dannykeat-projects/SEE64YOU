#include <stdio.h>
#include <stdlib.h>

#include "platform/requirements.h"

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\\n", message);
        exit(1);
    }
}

int main(void)
{
    S64PlatformRequirements requirements = {0u, 0u, 0u, 0u};

    require(
        s64_platform_check_requirements(0) == S64_PLATFORM_MISSING_UCI,
        "null requirements fail closed"
    );
    require(
        s64_platform_check_requirements(&requirements) == S64_PLATFORM_MISSING_UCI,
        "missing UCI rejected"
    );

    requirements.interface_present = 1u;
    require(
        s64_platform_check_requirements(&requirements) == S64_PLATFORM_MISSING_DOS,
        "missing Ultimate-DOS rejected"
    );

    requirements.dos_target = 1u;
    require(
        s64_platform_check_requirements(&requirements) == S64_PLATFORM_MISSING_REU,
        "missing REU rejected"
    );

    requirements.reu_present = 1u;
    requirements.reu_banks = 2u;
    require(
        s64_platform_check_requirements(&requirements) == S64_PLATFORM_WRONG_REU_SIZE,
        "legacy 128 KiB REU rejected"
    );

    requirements.reu_banks = 255u;
    require(
        s64_platform_check_requirements(&requirements) == S64_PLATFORM_WRONG_REU_SIZE,
        "undersized REU rejected"
    );

    requirements.reu_banks = S64_PLATFORM_REQUIRED_REU_BANKS;
    require(
        s64_platform_check_requirements(&requirements) == S64_PLATFORM_READY,
        "16 MiB REU accepted"
    );

    puts("platform requirements tests passed");
    return 0;
}
