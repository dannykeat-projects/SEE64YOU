#ifndef SEE64YOU_PLATFORM_TURBO_H
#define SEE64YOU_PLATFORM_TURBO_H

#include <stdint.h>

#include "platform/system_state.h"

typedef uint8_t (*S64TurboReadFn)(void* context);
typedef void (*S64TurboWriteFn)(void* context, uint8_t value);

typedef struct {
    S64TurboReadFn read;
    S64TurboWriteFn write;
    void* io_context;
    uint8_t captured_control;
    uint8_t controllable;
} S64TurboBackend;

/*
 * The injected form is host-testable. The hardware form uses the existing
 * C64U $D031 register path on cc65 builds and behaves as unavailable on host.
 */
void s64_turbo_backend_init(
    S64TurboBackend* backend,
    S64TurboReadFn read,
    S64TurboWriteFn write,
    void* io_context
);
void s64_turbo_backend_init_hardware(S64TurboBackend* backend);
void s64_turbo_backend_bind(S64SystemBackend* backend);

#endif
