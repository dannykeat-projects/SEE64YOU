#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/system_state.h"

typedef struct {
    unsigned int captures;
    unsigned int applies;
    unsigned int restores;
    unsigned int verifies;
    uint8_t capture_ok;
    uint8_t apply_ok;
    uint8_t restore_ok;
    uint8_t verify_ok;
    uint8_t captured_turbo;
    S64SystemMode last_mode;
    S64SystemSnapshot restored_snapshot;
} MockBackend;

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static uint8_t capture(void* context, S64SystemSnapshot* snapshot)
{
    MockBackend* mock = (MockBackend*)context;
    ++mock->captures;
    if (!mock->capture_ok) return 0u;

    snapshot->turbo_control = mock->captured_turbo;
    snapshot->turbo_control_valid = 1u;
    return 1u;
}

static uint8_t apply_mode(void* context, S64SystemMode mode)
{
    MockBackend* mock = (MockBackend*)context;
    ++mock->applies;
    mock->last_mode = mode;
    return mock->apply_ok;
}

static uint8_t restore(void* context, const S64SystemSnapshot* snapshot)
{
    MockBackend* mock = (MockBackend*)context;
    ++mock->restores;
    mock->restored_snapshot = *snapshot;
    return mock->restore_ok;
}

static uint8_t verify(void* context, const S64SystemSnapshot* snapshot)
{
    MockBackend* mock = (MockBackend*)context;
    ++mock->verifies;
    require(snapshot->turbo_control == mock->captured_turbo,
        "verify receives original snapshot");
    return mock->verify_ok;
}

static S64SystemBackend backend_with_verify(void)
{
    S64SystemBackend backend;
    backend.capture = capture;
    backend.apply_mode = apply_mode;
    backend.restore = restore;
    backend.verify = verify;
    return backend;
}

static MockBackend ready_mock(void)
{
    MockBackend mock;
    memset(&mock, 0, sizeof(mock));
    mock.capture_ok = 1u;
    mock.apply_ok = 1u;
    mock.restore_ok = 1u;
    mock.verify_ok = 1u;
    mock.captured_turbo = 0x37u;
    return mock;
}

int main(void)
{
    S64SystemState state;
    S64SystemBackend backend = backend_with_verify();
    MockBackend mock = ready_mock();

    s64_system_state_init(&state, &backend, &mock);
    require(state.mode == S64_SYSTEM_MODE_UNMANAGED, "init is unmanaged");
    require(
        s64_system_state_set_mode(&state, S64_SYSTEM_MODE_INTERACTIVE)
            == S64_SYSTEM_NOT_CAPTURED,
        "mode change requires capture"
    );
    require(mock.applies == 0u, "uncaptured mode performs no write");

    require(
        s64_system_state_capture(&state) == S64_SYSTEM_OK,
        "capture succeeds"
    );
    require(state.captured && !state.modified, "capture state recorded");
    require(state.snapshot.turbo_control == 0x37u
        && state.snapshot.turbo_control_valid,
        "capture preserves original setting");

    require(
        s64_system_state_capture(&state) == S64_SYSTEM_OK,
        "duplicate capture is idempotent"
    );
    require(mock.captures == 1u, "duplicate capture avoids hardware read");

    require(
        s64_system_state_set_mode(&state, S64_SYSTEM_MODE_INTERACTIVE)
            == S64_SYSTEM_OK,
        "interactive mode applies"
    );
    require(state.modified && state.mode == S64_SYSTEM_MODE_INTERACTIVE,
        "applied mode recorded");
    require(mock.applies == 1u
        && mock.last_mode == S64_SYSTEM_MODE_INTERACTIVE,
        "backend receives interactive mode");

    require(
        s64_system_state_set_mode(&state, S64_SYSTEM_MODE_INTERACTIVE)
            == S64_SYSTEM_OK,
        "duplicate mode is idempotent"
    );
    require(mock.applies == 1u, "duplicate mode avoids hardware write");

    mock.apply_ok = 0u;
    require(
        s64_system_state_set_mode(&state, S64_SYSTEM_MODE_CPU_INTENSIVE)
            == S64_SYSTEM_APPLY_FAILED,
        "failed mode write is reported"
    );
    require(state.mode == S64_SYSTEM_MODE_INTERACTIVE,
        "failed mode keeps previous logical mode");
    mock.apply_ok = 1u;

    require(
        s64_system_state_set_mode(&state, S64_SYSTEM_MODE_IO_SAFE)
            == S64_SYSTEM_OK,
        "io-safe mode applies"
    );
    require(mock.last_mode == S64_SYSTEM_MODE_IO_SAFE,
        "io-safe reaches backend");

    mock.verify_ok = 0u;
    require(
        s64_system_state_restore(&state) == S64_SYSTEM_VERIFY_FAILED,
        "restore verification failure is reported"
    );
    require(mock.restores == 1u && mock.verifies == 1u,
        "failed verification attempted restore and verify");
    require(state.modified && !state.restored,
        "failed verification remains retryable");

    mock.verify_ok = 1u;
    require(
        s64_system_state_restore(&state) == S64_SYSTEM_OK,
        "restore retry succeeds"
    );
    require(mock.restores == 2u && mock.verifies == 2u,
        "restore retry repeats hardware operation");
    require(!state.modified && state.restored
        && state.mode == S64_SYSTEM_MODE_UNMANAGED,
        "successful restore clears managed mode");
    require(mock.restored_snapshot.turbo_control == 0x37u,
        "restore receives original snapshot");

    require(
        s64_system_state_restore(&state) == S64_SYSTEM_OK,
        "completed restore is idempotent"
    );
    require(mock.restores == 2u, "idempotent restore avoids hardware write");

    mock.captured_turbo = 0x42u;
    require(
        s64_system_state_capture(&state) == S64_SYSTEM_OK,
        "new session can capture after restore"
    );
    require(mock.captures == 2u && state.snapshot.turbo_control == 0x42u,
        "new capture refreshes original state");

    mock.restore_ok = 0u;
    require(
        s64_system_state_set_mode(&state, S64_SYSTEM_MODE_TRANSITION)
            == S64_SYSTEM_OK,
        "transition mode applies"
    );
    require(
        s64_system_state_restore(&state) == S64_SYSTEM_RESTORE_FAILED,
        "restore write failure is reported"
    );
    require(!state.restored && state.modified,
        "restore failure remains retryable");

    {
        S64SystemState failed;
        MockBackend failed_mock = ready_mock();
        failed_mock.capture_ok = 0u;
        s64_system_state_init(&failed, &backend, &failed_mock);
        require(
            s64_system_state_capture(&failed) == S64_SYSTEM_CAPTURE_FAILED,
            "capture failure is reported"
        );
        require(
            s64_system_state_set_mode(&failed, S64_SYSTEM_MODE_CPU_INTENSIVE)
                == S64_SYSTEM_NOT_CAPTURED,
            "failed capture prevents writes"
        );
        require(failed_mock.applies == 0u, "failed capture changes nothing");
    }

    {
        S64SystemState no_verify;
        MockBackend no_verify_mock = ready_mock();
        S64SystemBackend no_verify_backend = backend_with_verify();
        no_verify_backend.verify = 0;

        s64_system_state_init(&no_verify, &no_verify_backend, &no_verify_mock);
        require(s64_system_state_capture(&no_verify) == S64_SYSTEM_OK,
            "capture without verify callback");
        require(
            s64_system_state_set_mode(&no_verify, S64_SYSTEM_MODE_CPU_INTENSIVE)
                == S64_SYSTEM_OK,
            "cpu-intensive mode applies"
        );
        require(s64_system_state_restore(&no_verify) == S64_SYSTEM_OK,
            "restore may omit verification when backend cannot verify");
        require(no_verify_mock.verifies == 0u, "optional verify not called");
    }

    puts("system state manager tests passed");
    return 0;
}
