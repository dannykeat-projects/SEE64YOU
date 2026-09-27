#ifndef SEE64YOU_REU_H
#define SEE64YOU_REU_H

#include <stdint.h>

/*
 * Raw REU DMA access. Callers remain responsible for choosing REU addresses
 * that fit the configured device size.
 */
uint8_t s64_reu_probe(void);
uint8_t s64_reu_store(
    uint8_t bank,
    uint16_t address,
    const void* source,
    uint16_t length
);
uint8_t s64_reu_load(
    uint8_t bank,
    uint16_t address,
    void* destination,
    uint16_t length
);

#endif
