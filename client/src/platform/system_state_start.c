#include "platform/system_state.h"
#include <string.h>

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
