#ifndef SEE64YOU_PLATFORM_UCI_H
#define SEE64YOU_PLATFORM_UCI_H

#include <stdint.h>

typedef struct {
    uint8_t interface_present;
    uint8_t http_target;
} S64UciCapabilities;

/*
 * Probe the published Ultimate Command Interface without assuming a firmware
 * version. Missing/unsupported UCI is a normal offline condition.
 */
void s64_uci_probe_capabilities(S64UciCapabilities* capabilities);

#endif
