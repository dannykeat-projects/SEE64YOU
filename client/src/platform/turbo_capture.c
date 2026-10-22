#include "platform/turbo.h"

#define S64_TURBO_SPEED_MASK 0x0fu
#define S64_TURBO_PROBE_SPEED 0x0eu

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

void s64_turbo_backend_bind(S64SystemBackend* backend)
{
    if (backend == 0) return;
    s64_turbo_backend_bind_adopt(backend);
    backend->capture = capture_state;
}
