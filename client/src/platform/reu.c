#include "reu.h"

#define S64_REU_STATUS          (*(volatile uint8_t*)0xDF00u)
#define S64_REU_COMMAND         (*(volatile uint8_t*)0xDF01u)
#define S64_REU_C64_ADDR_LO     (*(volatile uint8_t*)0xDF02u)
#define S64_REU_C64_ADDR_HI     (*(volatile uint8_t*)0xDF03u)
#define S64_REU_ADDR_LO         (*(volatile uint8_t*)0xDF04u)
#define S64_REU_ADDR_HI         (*(volatile uint8_t*)0xDF05u)
#define S64_REU_BANK            (*(volatile uint8_t*)0xDF06u)
#define S64_REU_LENGTH_LO       (*(volatile uint8_t*)0xDF07u)
#define S64_REU_LENGTH_HI       (*(volatile uint8_t*)0xDF08u)
#define S64_REU_ADDR_CONTROL    (*(volatile uint8_t*)0xDF0Au)

#define S64_REU_STATUS_END_OF_BLOCK 0x40u
#define S64_REU_COMMAND_EXECUTE     0x80u
#define S64_REU_COMMAND_IMMEDIATE   0x10u
#define S64_REU_TRANSFER_TO_REU     0x00u
#define S64_REU_TRANSFER_FROM_REU   0x01u

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

    /* Clear stale completion flags before starting the immediate DMA. */
    (void)S64_REU_STATUS;
    S64_REU_COMMAND = (uint8_t)(
        S64_REU_COMMAND_EXECUTE
        | S64_REU_COMMAND_IMMEDIATE
        | transfer_type
    );

    return (S64_REU_STATUS & S64_REU_STATUS_END_OF_BLOCK) != 0u;
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

uint8_t s64_reu_probe(void)
{
    uint8_t original;
    uint8_t probe = 0xA5u;
    uint8_t readback = 0u;
    uint8_t restored;

    /*
     * Exercise one byte in bank 0 rather than trusting register echo alone.
     * The original byte is restored before returning.
     */
    if (!s64_reu_load(0u, 0u, &original, 1u)) {
        return 0u;
    }
    if (!s64_reu_store(0u, 0u, &probe, 1u)) {
        return 0u;
    }

    if (!s64_reu_load(0u, 0u, &readback, 1u)) {
        (void)s64_reu_store(0u, 0u, &original, 1u);
        return 0u;
    }

    restored = s64_reu_store(0u, 0u, &original, 1u);
    return restored && readback == probe;
}
