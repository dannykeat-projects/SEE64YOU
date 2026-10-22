#include "reu.h"

#define S64_REU_C64_ADDR_LO     (*(volatile uint8_t*)0xDF02u)
#define S64_REU_C64_ADDR_HI     (*(volatile uint8_t*)0xDF03u)
#define S64_REU_ADDR_LO         (*(volatile uint8_t*)0xDF04u)
#define S64_REU_ADDR_HI         (*(volatile uint8_t*)0xDF05u)
#define S64_REU_BANK            (*(volatile uint8_t*)0xDF06u)
#define S64_REU_LENGTH_LO       (*(volatile uint8_t*)0xDF07u)
#define S64_REU_LENGTH_HI       (*(volatile uint8_t*)0xDF08u)
#define S64_REU_ADDR_CONTROL    (*(volatile uint8_t*)0xDF0Au)

#define S64_REU_TRANSFER_TO_REU     0xECu
#define S64_REU_TRANSFER_FROM_REU   0xEDu

void __fastcall__ s64_reu_kick(uint8_t command);

static uint8_t probe_original;
static uint8_t probe_value;
static uint8_t probe_readback;
static uint8_t probe_bank_original;

static uint8_t s64_reu_transfer(
    uint8_t transfer_type,
    uint8_t bank,
    uint16_t reu_address,
    void* c64_memory,
    uint16_t length
)
{
    unsigned int c64_address;

    if (c64_memory == 0 || length == 0u) {
        return 0u;
    }

    c64_address = (unsigned int)c64_memory;

    S64_REU_C64_ADDR_LO = (uint8_t)c64_address;
    S64_REU_C64_ADDR_HI = (uint8_t)(c64_address >> 8);
    S64_REU_ADDR_LO = (uint8_t)reu_address;
    S64_REU_ADDR_HI = (uint8_t)(reu_address >> 8);
    S64_REU_BANK = bank;
    S64_REU_LENGTH_LO = (uint8_t)length;
    S64_REU_LENGTH_HI = (uint8_t)(length >> 8);
    S64_REU_ADDR_CONTROL = 0u;

    /*
     * Use the same delayed-trigger sequence as cc65's c64-reu.emd driver.
     * The assembly helper preserves the CPU memory configuration and flags.
     */
    s64_reu_kick(transfer_type);
    return 1u;
}

uint8_t s64_reu_store(
    uint8_t bank,
    uint16_t address,
    const void* source,
    uint16_t length
)
{
    return s64_reu_transfer(
        S64_REU_TRANSFER_TO_REU,
        bank,
        address,
        (void*)source,
        length
    );
}

uint8_t s64_reu_load(
    uint8_t bank,
    uint16_t address,
    void* destination,
    uint16_t length
)
{
    return s64_reu_transfer(
        S64_REU_TRANSFER_FROM_REU,
        bank,
        address,
        destination,
        length
    );
}

/* Fixed one-byte scratch avoids DMA into the cc65 software stack. */
static void probe_load(uint8_t bank)
{
    (void)s64_reu_load(bank, 0u, &probe_readback, 1u);
}

static void probe_store(uint8_t bank)
{
    (void)s64_reu_store(bank, 0u, &probe_value, 1u);
}

uint8_t s64_reu_probe(void)
{
    uint8_t present;
    probe_load(0u);
    probe_original = probe_readback;
    probe_value = 0xa5u;
    probe_store(0u);
    probe_load(0u);
    present = probe_readback == probe_value;
    probe_value = probe_original;
    probe_store(0u);
    return present;
}

uint16_t s64_reu_probe_banks(void)
{
    static uint8_t bank;
    uint8_t present;
    /* Check writable power-of-two banks and aliasing without destroying the
     * document/clipboard that companions have already received in the REU. */
    probe_load(0u);
    probe_original = probe_readback;
    for (bank = 1u; bank != 0u; bank = (uint8_t)(bank << 1)) {
        probe_load(bank);
        probe_bank_original = probe_readback;
        probe_value = (uint8_t)(probe_original ^ 0xffu);
        probe_store(bank);
        /* Read bank 0 first: unmapped REU banks can echo the last bus byte. */
        probe_load(0u);
        present = probe_readback == probe_original;
        probe_load(bank);
        present = present && probe_readback == probe_value;
        probe_value = probe_bank_original;
        probe_store(bank);
        if (!present) return bank;
    }
    return 256u;
}
