#include <conio.h>
#include <string.h>
#include "platform/reu.h"

/* Exercise the production startup/cleanup path, rather than a duplicate. */
#define main launcher_main
#include "launcher/main.c"
#undef main

/* Samples assume today's control/cache range [0x020000, 0x050100).
 * Every sentinel is backed up in C64 RAM and restored before returning. */
static const uint32_t addresses[] = {
    0x000000UL, 0x00efc0UL, 0x00f000UL, 0x00ffc0UL,
    0x010000UL, 0x01f000UL, 0x01fc00UL, 0x01ff00UL,
    0x0200c0UL, 0x050100UL, 0x060000UL, 0x070040UL,
    0x0ffff0UL, 0x1fffc0UL, 0x3fffc0UL, 0x7fffc0UL, 0xffffc0UL
};
#define SAMPLE_COUNT (sizeof(addresses) / sizeof(addresses[0]))
#define SAMPLE_SIZE 16u

static uint8_t originals[SAMPLE_COUNT][SAMPLE_SIZE];
static uint8_t sentinel[SAMPLE_SIZE], actual[SAMPLE_SIZE];
static S64SystemState outer;
static S64TurboBackend outer_turbo;
static S64SystemBackend outer_backend;
static uint8_t requested_turbo;

/* Diagnostic-only preset; production modes keep their existing mapping. */
static uint8_t test_mode(void* context, S64SystemMode mode)
{
    S64TurboBackend* turbo = (S64TurboBackend*)context;
    (void)mode;
    requested_turbo = (turbo->captured_control & 0xf0u) | 2u;
    if (!turbo->controllable) return 0u;
    turbo->write(turbo->io_context, requested_turbo);
    if (turbo->read(turbo->io_context) == requested_turbo) return 1u;
    turbo->write(turbo->io_context, turbo->captured_control);
    return 0u;
}

int main(void)
{
    uint8_t i, j, prepared = 0u, passed = 1u;
    uint32_t address;
    int result;

    s64_turbo_backend_init_hardware(&outer_turbo);
    s64_turbo_backend_bind(&outer_backend);
    outer_backend.apply_mode = test_mode;
    s64_system_state_init(&outer, &outer_backend, &outer_turbo);
    if (s64_system_state_capture(&outer) != S64_SYSTEM_OK) return 1;

    for (i = 0u; i < SAMPLE_COUNT; ++i) {
        address = addresses[i];
        if (!s64_reu_load((uint8_t)(address >> 16), (uint16_t)address,
                originals[i], SAMPLE_SIZE)) {
            passed = 0u;
            break;
        }
        ++prepared;
        for (j = 0u; j < SAMPLE_SIZE; ++j)
            sentinel[j] = (uint8_t)(0xa5u ^ i ^ (j * 13u));
        if (!s64_reu_store((uint8_t)(address >> 16), (uint16_t)address,
                sentinel, SAMPLE_SIZE)) {
            passed = 0u;
            break;
        }
    }
    if (passed && s64_system_state_set_mode(&outer,
            S64_SYSTEM_MODE_CPU_INTENSIVE) == S64_SYSTEM_OK) {
        result = launcher_main();
        if (result != 0
            || outer_turbo.read(outer_turbo.io_context) != requested_turbo)
            passed = 0u;
        for (i = 0u; i < SAMPLE_COUNT; ++i) {
            address = addresses[i];
            if (!s64_reu_load((uint8_t)(address >> 16), (uint16_t)address,
                    actual, SAMPLE_SIZE)) passed = 0u;
            for (j = 0u; j < SAMPLE_SIZE; ++j) {
                if (actual[j] != (uint8_t)(0xa5u ^ i ^ (j * 13u)))
                    passed = 0u;
            }
        }
    } else passed = 0u;

    for (i = 0u; i < prepared; ++i) {
        address = addresses[i];
        if (!s64_reu_store((uint8_t)(address >> 16), (uint16_t)address,
                originals[i], SAMPLE_SIZE)
            || !s64_reu_load((uint8_t)(address >> 16), (uint16_t)address,
                actual, SAMPLE_SIZE)
            || memcmp(actual, originals[i], SAMPLE_SIZE) != 0) passed = 0u;
    }
    if (s64_system_state_restore(&outer) != S64_SYSTEM_OK) passed = 0u;
    cputs(passed ? "CACHE GATE PASS\r\n" : "CACHE GATE FAIL\r\n");
    cprintf("START %u INNER %u FINAL %u\r\n", outer.snapshot.turbo_control,
        requested_turbo, outer_turbo.read(outer_turbo.io_context));
    return passed ? 0 : 1;
}
