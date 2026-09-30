#include <stdint.h>

#include "platform/reu.h"

#define DIAG_PROGRESS (*(volatile uint8_t*)0xC000u)
#define DIAG_RESULT   (*(volatile uint8_t*)0xC001u)
#define DIAG_BANKS_LO (*(volatile uint8_t*)0xC002u)
#define DIAG_BANKS_HI (*(volatile uint8_t*)0xC003u)

#define DIAG_PASS 0xA5u
#define DIAG_FAIL 0x5Au

static uint8_t dma_scratch;

static void stop_with(uint8_t result)
{
    DIAG_RESULT = result;
    for (;;) {
    }
}

int main(void)
{
    static uint16_t banks;
    static uint16_t bank;

    DIAG_RESULT = 0u;
    DIAG_BANKS_LO = 0u;
    DIAG_BANKS_HI = 0u;

    DIAG_PROGRESS = 0x10u;
    dma_scratch = 0x5Au;
    if (!s64_reu_store(0u, 0u, &dma_scratch, 1u)) {
        stop_with(DIAG_FAIL);
    }

    DIAG_PROGRESS = 0x11u;
    dma_scratch = 0u;
    if (!s64_reu_load(0u, 0u, &dma_scratch, 1u)) {
        stop_with(DIAG_FAIL);
    }

    DIAG_PROGRESS = 0x12u;
    if (dma_scratch != 0x5Au) {
        stop_with(DIAG_FAIL);
    }

    DIAG_PROGRESS = 0x20u;
    if (!s64_reu_probe()) {
        stop_with(DIAG_FAIL);
    }

    DIAG_PROGRESS = 0x30u;
    banks = s64_reu_probe_banks();
    if (banks == 0u) {
        stop_with(DIAG_FAIL);
    }

    dma_scratch = 0u;
    (void)s64_reu_load(0u, 0u, &dma_scratch, 1u);
    if (dma_scratch != 0x5Au) stop_with(DIAG_FAIL);

    DIAG_PROGRESS = 0x32u;
    /* Companion startup must preserve occupied document/clipboard banks. */
    for (bank = 0u; bank < banks; ++bank) {
        dma_scratch = (uint8_t)(bank ^ 0xa7u);
        (void)s64_reu_store((uint8_t)bank, 0u, &dma_scratch, 1u);
    }
    DIAG_PROGRESS = 0x33u;
    if (s64_reu_probe_banks() != banks) stop_with(DIAG_FAIL);
    DIAG_PROGRESS = 0x34u;
    for (bank = 0u; bank < banks; ++bank) {
        (void)s64_reu_load((uint8_t)bank, 0u, &dma_scratch, 1u);
        if (dma_scratch != (uint8_t)(bank ^ 0xa7u)) stop_with(DIAG_FAIL);
    }

    DIAG_BANKS_LO = (uint8_t)banks;
    DIAG_BANKS_HI = (uint8_t)(banks >> 8);
    DIAG_PROGRESS = 0x40u;
    stop_with(DIAG_PASS);
    return 0;
}
