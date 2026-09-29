#ifndef SEE64YOU_PLATFORM_TURBO_H
#define SEE64YOU_PLATFORM_TURBO_H

#include <stdint.h>

typedef struct {
    uint8_t original;
    uint8_t active;
} S64TurboGuard;

/*
 * Temporarily request the fastest C64U CPU speed through $D031.
 * Returns 1 when register-controlled turbo was detected and enabled.
 * The caller must always pair a successful begin with s64_turbo_restore().
 */
uint8_t s64_turbo_begin_max(S64TurboGuard* guard);
void s64_turbo_restore(S64TurboGuard* guard);

#endif
