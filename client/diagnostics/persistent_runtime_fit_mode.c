#include <stdint.h>

/*
 * Diagnostic-only placeholder for the future in-process Publisher mode.
 * The real Editor root imports this through the normal overlay veneer so the
 * fit includes the resident dispatch cost without retaining the old PRG
 * transfer/application-state machinery.
 */
uint8_t s64_persistent_runtime_fit_publisher(void)
{
    return 1u;
}
