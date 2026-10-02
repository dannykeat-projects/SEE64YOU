#ifndef SEE64YOU_PLATFORM_SYSTEM_STATE_H
#define SEE64YOU_PLATFORM_SYSTEM_STATE_H

#include <stdint.h>

typedef enum {
    S64_SYSTEM_MODE_UNMANAGED = 0,
    S64_SYSTEM_MODE_INTERACTIVE,
    S64_SYSTEM_MODE_CPU_INTENSIVE,
    S64_SYSTEM_MODE_IO_SAFE,
    S64_SYSTEM_MODE_TRANSITION
} S64SystemMode;

typedef enum {
    S64_SYSTEM_OK = 0,
    S64_SYSTEM_INVALID,
    S64_SYSTEM_CAPTURE_FAILED,
    S64_SYSTEM_NOT_CAPTURED,
    S64_SYSTEM_APPLY_FAILED,
    S64_SYSTEM_RESTORE_FAILED,
    S64_SYSTEM_VERIFY_FAILED
} S64SystemResult;

/*
 * Extend this only for machine state See64You actually owns temporarily.
 * A valid bit prevents an unreadable setting from being silently restored.
 */
typedef struct {
    uint8_t turbo_control;
    uint8_t turbo_control_valid;
} S64SystemSnapshot;

typedef uint8_t (*S64SystemCaptureFn)(
    void* context,
    S64SystemSnapshot* snapshot
);
typedef uint8_t (*S64SystemApplyModeFn)(
    void* context,
    S64SystemMode mode
);
typedef uint8_t (*S64SystemRestoreFn)(
    void* context,
    const S64SystemSnapshot* snapshot
);
typedef uint8_t (*S64SystemVerifyFn)(
    void* context,
    const S64SystemSnapshot* snapshot
);

typedef struct {
    S64SystemCaptureFn capture;
    S64SystemApplyModeFn apply_mode;
    S64SystemRestoreFn restore;
    S64SystemVerifyFn verify;
} S64SystemBackend;

typedef struct {
    S64SystemBackend backend;
    void* backend_context;
    S64SystemSnapshot snapshot;
    S64SystemMode mode;
    uint8_t captured;
    uint8_t modified;
    uint8_t restored;
} S64SystemState;

void s64_system_state_init(
    S64SystemState* state,
    const S64SystemBackend* backend,
    void* backend_context
);

S64SystemResult s64_system_state_capture(S64SystemState* state);
/* Fresh destination only: adopt the trusted caller snapshot at TRANSITION.
 * No capture, hardware write or new ownership boundary. */
S64SystemResult s64_system_state_adopt(S64SystemState* state,
    const S64SystemSnapshot* snapshot);
S64SystemResult s64_system_state_set_mode(
    S64SystemState* state,
    S64SystemMode mode
);
S64SystemResult s64_system_state_restore(S64SystemState* state);

#endif
