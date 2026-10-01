#include "platform/turbo.h"

#include <string.h>

#define S64_TURBO_SPEED_MASK  0x0fu
#define S64_TURBO_MAX_SPEED   0x0fu
#define S64_TURBO_PROBE_SPEED 0x0eu

#ifdef __CC65__
#define S64_TURBO_CONTROL (*(volatile uint8_t*)0xD031u)

static uint8_t hardware_read(void* context)
{
    (void)context;
    return S64_TURBO_CONTROL;
}

static void hardware_write(void* context, uint8_t value)
{
    (void)context;
    S64_TURBO_CONTROL = value;
}
#else
static uint8_t hardware_read(void* context)
{
    (void)context;
    return 0xffu;
}

static void hardware_write(void* context, uint8_t value)
{
    (void)context;
    (void)value;
}
#endif

static uint8_t capture_state(
    void* context,
    S64SystemSnapshot* snapshot
)
{
    S64TurboBackend* backend = (S64TurboBackend*)context;
    uint8_t observed;
    uint8_t probe;

    if (backend == 0 || snapshot == 0
        || backend->read == 0 || backend->write == 0) {
        return 0u;
    }

    backend->captured_control = backend->read(backend->io_context);
    backend->controllable = 0u;
    snapshot->turbo_control = backend->captured_control;
    snapshot->turbo_control_valid = 0u;

    /*
     * $D031 reading $FF is ambiguous: it can mean either unavailable register
     * control or a valid max-speed value. Probe the speed nibble, immediately
     * restore the captured byte, and only claim ownership if restoration can
     * be verified.
     */
    if (backend->captured_control == 0xffu) {
        probe = (uint8_t)(
            (backend->captured_control & (uint8_t)~S64_TURBO_SPEED_MASK)
            | S64_TURBO_PROBE_SPEED
        );
        backend->write(backend->io_context, probe);
        observed = backend->read(backend->io_context);
        backend->write(backend->io_context, backend->captured_control);

        if ((observed & S64_TURBO_SPEED_MASK) != S64_TURBO_PROBE_SPEED) {
            return 1u;
        }
        if (backend->read(backend->io_context) != backend->captured_control) {
            return 0u;
        }
    }

    backend->controllable = 1u;
    snapshot->turbo_control_valid = 1u;
    return 1u;
}

static uint8_t apply_mode(void* context, S64SystemMode mode)
{
    S64TurboBackend* backend = (S64TurboBackend*)context;
    uint8_t requested;
    uint8_t observed;

    if (backend == 0 || backend->read == 0 || backend->write == 0) {
        return 0u;
    }
    if (!backend->controllable) return 1u;

    requested = backend->captured_control;
    if (mode == S64_SYSTEM_MODE_CPU_INTENSIVE) {
        requested = (uint8_t)(
            (backend->captured_control & (uint8_t)~S64_TURBO_SPEED_MASK)
            | S64_TURBO_MAX_SPEED
        );
    }

    backend->write(backend->io_context, requested);
    observed = backend->read(backend->io_context);

    if (mode == S64_SYSTEM_MODE_CPU_INTENSIVE) {
        if ((observed & S64_TURBO_SPEED_MASK) == S64_TURBO_MAX_SPEED) {
            return 1u;
        }
    } else if (observed == requested) {
        return 1u;
    }

    backend->write(backend->io_context, backend->captured_control);
    return 0u;
}

static uint8_t restore_state(
    void* context,
    const S64SystemSnapshot* snapshot
)
{
    S64TurboBackend* backend = (S64TurboBackend*)context;

    if (backend == 0 || snapshot == 0 || backend->write == 0) return 0u;
    if (!snapshot->turbo_control_valid) return 1u;

    backend->write(backend->io_context, snapshot->turbo_control);
    return 1u;
}

static uint8_t verify_state(
    void* context,
    const S64SystemSnapshot* snapshot
)
{
    S64TurboBackend* backend = (S64TurboBackend*)context;

    if (backend == 0 || snapshot == 0 || backend->read == 0) return 0u;
    if (!snapshot->turbo_control_valid) return 1u;

    return backend->read(backend->io_context) == snapshot->turbo_control;
}

void s64_turbo_backend_init(
    S64TurboBackend* backend,
    S64TurboReadFn read,
    S64TurboWriteFn write,
    void* io_context
)
{
    if (backend == 0) return;

    memset(backend, 0, sizeof(*backend));
    backend->read = read;
    backend->write = write;
    backend->io_context = io_context;
}

void s64_turbo_backend_init_hardware(S64TurboBackend* backend)
{
    s64_turbo_backend_init(backend, hardware_read, hardware_write, 0);
}

void s64_turbo_backend_bind(S64SystemBackend* backend)
{
    if (backend == 0) return;

    backend->capture = capture_state;
    backend->apply_mode = apply_mode;
    backend->restore = restore_state;
    backend->verify = verify_state;
}
