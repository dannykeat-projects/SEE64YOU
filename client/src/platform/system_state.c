#include "platform/system_state.h"

#include <string.h>

static uint8_t mode_valid(S64SystemMode mode)
{
    return mode == S64_SYSTEM_MODE_INTERACTIVE
        || mode == S64_SYSTEM_MODE_CPU_INTENSIVE
        || mode == S64_SYSTEM_MODE_IO_SAFE
        || mode == S64_SYSTEM_MODE_TRANSITION;
}

void s64_system_state_init(
    S64SystemState* state,
    const S64SystemBackend* backend,
    void* backend_context
)
{
    if (state == 0) return;

    memset(state, 0, sizeof(*state));
    state->mode = S64_SYSTEM_MODE_UNMANAGED;
    state->backend_context = backend_context;
    if (backend != 0) {
        state->backend = *backend;
    }
}

S64SystemResult s64_system_state_capture(S64SystemState* state)
{
    if (state == 0 || state->backend.capture == 0) {
        return S64_SYSTEM_INVALID;
    }

    if (state->captured && !state->restored) {
        return S64_SYSTEM_OK;
    }

    memset(&state->snapshot, 0, sizeof(state->snapshot));
    if (!state->backend.capture(state->backend_context, &state->snapshot)) {
        state->captured = 0u;
        state->modified = 0u;
        state->restored = 0u;
        state->mode = S64_SYSTEM_MODE_UNMANAGED;
        return S64_SYSTEM_CAPTURE_FAILED;
    }

    state->captured = 1u;
    state->modified = 0u;
    state->restored = 0u;
    state->mode = S64_SYSTEM_MODE_UNMANAGED;
    return S64_SYSTEM_OK;
}

S64SystemResult s64_system_state_adopt(S64SystemState* state,
    const S64SystemSnapshot* snapshot)
{
    if (state == 0 || snapshot == 0 || snapshot->turbo_control_valid > 1u
        || state->captured || state->backend.apply_mode == 0
        || state->backend.restore == 0) return S64_SYSTEM_INVALID;
    state->snapshot = *snapshot;
    state->captured = 1u;
    state->modified = 1u;
    state->restored = 0u;
    state->mode = S64_SYSTEM_MODE_TRANSITION;
    return S64_SYSTEM_OK;
}

S64SystemResult s64_system_state_set_mode(
    S64SystemState* state,
    S64SystemMode mode
)
{
    if (state == 0 || state->backend.apply_mode == 0 || !mode_valid(mode)) {
        return S64_SYSTEM_INVALID;
    }
    if (!state->captured || state->restored) {
        return S64_SYSTEM_NOT_CAPTURED;
    }
    if (state->modified && state->mode == mode) {
        return S64_SYSTEM_OK;
    }

    if (!state->backend.apply_mode(state->backend_context, mode)) {
        return S64_SYSTEM_APPLY_FAILED;
    }

    state->mode = mode;
    state->modified = 1u;
    return S64_SYSTEM_OK;
}

S64SystemResult s64_system_state_restore(S64SystemState* state)
{
    if (state == 0 || state->backend.restore == 0) {
        return S64_SYSTEM_INVALID;
    }
    if (!state->captured) {
        return S64_SYSTEM_NOT_CAPTURED;
    }
    if (state->restored) {
        return S64_SYSTEM_OK;
    }

    if (state->modified) {
        if (!state->backend.restore(state->backend_context, &state->snapshot)) {
            return S64_SYSTEM_RESTORE_FAILED;
        }
        if (state->backend.verify != 0
            && !state->backend.verify(state->backend_context, &state->snapshot)) {
            return S64_SYSTEM_VERIFY_FAILED;
        }
    }

    state->modified = 0u;
    state->restored = 1u;
    state->mode = S64_SYSTEM_MODE_UNMANAGED;
    return S64_SYSTEM_OK;
}
