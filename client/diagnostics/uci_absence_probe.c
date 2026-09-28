#include <stdint.h>

#include "platform/uci.h"

#define DIAG_PROGRESS (*(volatile uint8_t*)0xC080u)
#define DIAG_RESULT   (*(volatile uint8_t*)0xC081u)
#define DIAG_DETAIL   (*(volatile uint8_t*)0xC082u)

#define DIAG_PASS 0xA5u
#define DIAG_FAIL 0x5Au

int main(void)
{
    S64UciCapabilities capabilities;
    uint8_t detail = 0u;

    DIAG_PROGRESS = 1u;
    DIAG_RESULT = 0u;
    DIAG_DETAIL = 0u;

    s64_uci_probe_capabilities(&capabilities);
    DIAG_PROGRESS = 2u;

    if (capabilities.interface_present) detail |= 0x01u;
    if (capabilities.dos_target) detail |= 0x02u;
    if (s64_uci_probe_target(0x03u)) detail |= 0x04u;
    if (capabilities.http_target) detail |= 0x08u;

    DIAG_DETAIL = detail;
    if (detail != 0u) {
        DIAG_RESULT = DIAG_FAIL;
    } else {
        DIAG_PROGRESS = 0x80u;
        DIAG_RESULT = DIAG_PASS;
    }

    for (;;) {
    }
}
