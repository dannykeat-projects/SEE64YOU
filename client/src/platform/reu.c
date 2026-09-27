#include "reu.h"

#define S64_REU_COMMAND         (*(volatile uint8_t*)0xDF01u)
#define S64_REU_C64_ADDR_LO     (*(volatile uint8_t*)0xDF02u)
#define S64_REU_C64_ADDR_HI     (*(volatile uint8_t*)0xDF03u)
#define S64_REU_ADDR_LO         (*(volatile uint8_t*)0xDF04u)
#define S64_REU_ADDR_HI         (*(volatile uint8_t*)0xDF05u)
#define S64_REU_BANK            (*(volatile uint8_t*)0xDF06u)
#define S64_REU_LENGTH_LO       (*(volatile uint8_t*)0xDF07u)
#define S64_REU_LENGTH_HI       (*(volatile uint8_t*)0xDF08u)
#define S64_REU_ADDR_CONTROL    (*(volatile uint8_t*)0xDF0Au)

#define S64_REU_COMMAND_EXECUTE     0x80u
#define S64_REU_COMMAND_IMMEDIATE   0x10u
#define S64_REU_TRANSFER_TO_REU     0x00u
#define S64_REU_TRANSFER_FROM_REU   0x01u

static uint8_t probe_original;
static uint8_t probe_value;
static uint8_t probe_readback;
static uint8_t probe_candidate_original;
static uint8_t probe_base_read;

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
     * Immediate REU DMA is synchronous from the CPU's point of view. Presence
     * is verified by s64_reu_probe() with a round trip, so the primitive does
     * not depend on model-specific status-flag clearing behaviour.
     */
    S64_REU_COMMAND = (uint8_t)(
        S64_REU_COMMAND_EXECUTE
        | S64_REU_COMMAND_IMMEDIATE
        | transfer_type
    );

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

uint8_t s64_reu_probe(void)
{
    uint8_t restored;

    /*
     * Keep DMA scratch outside the cc65 software stack. Exercise one byte in
     * bank 0 rather than trusting register echo alone, then restore it.
     */
    probe_value = 0xA5u;
    probe_readback = 0u;

    if (!s64_reu_load(0u, 0u, &probe_original, 1u)) {
        return 0u;
    }
    if (!s64_reu_store(0u, 0u, &probe_value, 1u)) {
        return 0u;
    }

    if (!s64_reu_load(0u, 0u, &probe_readback, 1u)) {
        (void)s64_reu_store(0u, 0u, &probe_original, 1u);
        return 0u;
    }

    restored = s64_reu_store(0u, 0u, &probe_original, 1u);
    return restored && probe_readback == probe_value;
}

uint16_t s64_reu_probe_banks(void)
{
    static const uint8_t candidates[] = {
        2u, 4u, 8u, 16u, 32u, 64u, 128u
    };
    uint8_t candidate;
    uint8_t i;

    probe_value = 0x5Au;
    probe_readback = 0xA5u;

    if (!s64_reu_load(0u, 0u, &probe_original, 1u)) {
        return 0u;
    }

    for (i = 0u; i < (uint8_t)sizeof(candidates); ++i) {
        candidate = candidates[i];

        if (!s64_reu_load(candidate, 0u, &probe_candidate_original, 1u)) {
            return 0u;
        }
        if (!s64_reu_store(0u, 0u, &probe_value, 1u)) {
            return 0u;
        }
        if (!s64_reu_store(candidate, 0u, &probe_readback, 1u)) {
            (void)s64_reu_store(0u, 0u, &probe_original, 1u);
            return 0u;
        }
        if (!s64_reu_load(0u, 0u, &probe_base_read, 1u)) {
            (void)s64_reu_store(candidate, 0u, &probe_candidate_original, 1u);
            (void)s64_reu_store(0u, 0u, &probe_original, 1u);
            return 0u;
        }

        if (probe_base_read == probe_readback) {
            return s64_reu_store(0u, 0u, &probe_original, 1u)
                ? (uint16_t)candidate
                : 0u;
        }

        if (probe_base_read != probe_value
            || !s64_reu_store(candidate, 0u, &probe_candidate_original, 1u)
            || !s64_reu_store(0u, 0u, &probe_original, 1u)) {
            return 0u;
        }
    }

    return 256u;
}
