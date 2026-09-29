#include "platform/turbo.h"

#define S64_TURBO_CONTROL (*(volatile uint8_t*)0xD031u)
#define S64_TURBO_SPEED_MASK 0x0fu
#define S64_TURBO_MAX_SPEED  0x0fu
#define S64_TURBO_PROBE_SPEED 0x0eu

uint8_t s64_turbo_begin_max(S64TurboGuard* guard)
{
    uint8_t requested;

    if (guard == 0) return 0u;

    guard->original = S64_TURBO_CONTROL;
    guard->active = 0u;

    /*
     * $D031 reads $FF when register-controlled turbo is unavailable.
     * Probe index 14 in that ambiguous case so an unavailable register
     * cannot be mistaken for max-speed + disabled badline timing.
     */
    if (guard->original == 0xffu) {
        S64_TURBO_CONTROL = (uint8_t)(
            (guard->original & (uint8_t)~S64_TURBO_SPEED_MASK)
            | S64_TURBO_PROBE_SPEED
        );
        if ((S64_TURBO_CONTROL & S64_TURBO_SPEED_MASK)
            != S64_TURBO_PROBE_SPEED) {
            return 0u;
        }
    }

    requested = (uint8_t)(
        (guard->original & (uint8_t)~S64_TURBO_SPEED_MASK)
        | S64_TURBO_MAX_SPEED
    );
    S64_TURBO_CONTROL = requested;
    if ((S64_TURBO_CONTROL & S64_TURBO_SPEED_MASK)
        != S64_TURBO_MAX_SPEED) {
        S64_TURBO_CONTROL = guard->original;
        return 0u;
    }

    guard->active = 1u;
    return 1u;
}

void s64_turbo_restore(S64TurboGuard* guard)
{
    if (guard == 0 || !guard->active) return;

    S64_TURBO_CONTROL = guard->original;
    guard->active = 0u;
}
