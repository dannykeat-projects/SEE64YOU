#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/system_state.h"
#include "platform/turbo.h"

typedef struct {
    uint8_t value;
    uint8_t writable;
    unsigned int writes;
} FakeRegister;

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static uint8_t fake_read(void* context)
{
    FakeRegister* reg = (FakeRegister*)context;
    return reg->value;
}

static void fake_write(void* context, uint8_t value)
{
    FakeRegister* reg = (FakeRegister*)context;
    ++reg->writes;
    if (reg->writable) reg->value = value;
}

static void init_state(
    S64SystemState* state,
    S64SystemBackend* system_backend,
    S64TurboBackend* turbo_backend,
    FakeRegister* reg
)
{
    s64_turbo_backend_init(turbo_backend, fake_read, fake_write, reg);
    s64_turbo_backend_bind(system_backend);
    s64_system_state_init(state, system_backend, turbo_backend);
}

int main(void)
{
    S64SystemState state;
    S64SystemBackend system_backend;
    S64TurboBackend turbo_backend;
    FakeRegister reg;

    memset(&reg, 0, sizeof(reg));
    reg.value = 0x25u;
    reg.writable = 1u;
    init_state(&state, &system_backend, &turbo_backend, &reg);

    require(s64_system_state_capture(&state) == S64_SYSTEM_OK,
        "normal turbo capture succeeds");
    require(state.snapshot.turbo_control_valid
        && state.snapshot.turbo_control == 0x25u,
        "normal turbo capture records original byte");
    require(reg.writes == 0u, "unambiguous capture performs no write");

    require(
        s64_system_state_set_mode(&state, S64_SYSTEM_MODE_CPU_INTENSIVE)
            == S64_SYSTEM_OK,
        "cpu-intensive mode applies"
    );
    require(reg.value == 0x2fu, "cpu-intensive preserves upper bits and sets max speed");

    require(
        s64_system_state_set_mode(&state, S64_SYSTEM_MODE_IO_SAFE)
            == S64_SYSTEM_OK,
        "io-safe mode applies"
    );
    require(reg.value == 0x25u, "io-safe restores captured control byte");

    require(
        s64_system_state_set_mode(&state, S64_SYSTEM_MODE_TRANSITION)
            == S64_SYSTEM_OK,
        "transition mode applies"
    );
    require(reg.value == 0x25u, "transition remains conservative before hardware proof");

    require(
        s64_system_state_set_mode(&state, S64_SYSTEM_MODE_CPU_INTENSIVE)
            == S64_SYSTEM_OK,
        "second cpu-intensive mode applies"
    );
    require(s64_system_state_restore(&state) == S64_SYSTEM_OK,
        "manager restores captured turbo byte");
    require(reg.value == 0x25u, "restore returns exact original turbo byte");

    memset(&reg, 0, sizeof(reg));
    reg.value = 0xffu;
    reg.writable = 0u;
    init_state(&state, &system_backend, &turbo_backend, &reg);
    require(s64_system_state_capture(&state) == S64_SYSTEM_OK,
        "unavailable turbo is a non-fatal optional capability");
    require(!state.snapshot.turbo_control_valid,
        "unavailable turbo is not claimed as restorable state");
    {
        unsigned int probe_writes = reg.writes;
        require(
            s64_system_state_set_mode(&state, S64_SYSTEM_MODE_CPU_INTENSIVE)
                == S64_SYSTEM_OK,
            "unavailable turbo mode degrades to no-op"
        );
        require(reg.writes == probe_writes,
            "unavailable turbo receives no mode write");
    }

    memset(&reg, 0, sizeof(reg));
    reg.value = 0xffu;
    reg.writable = 1u;
    init_state(&state, &system_backend, &turbo_backend, &reg);
    require(s64_system_state_capture(&state) == S64_SYSTEM_OK,
        "ambiguous ff turbo can be probed");
    require(state.snapshot.turbo_control_valid,
        "successful probe claims turbo control");
    require(reg.value == 0xffu,
        "probe restores ambiguous original byte before use");
    require(reg.writes == 2u,
        "ambiguous probe performs probe and immediate restore");

    memset(&reg, 0, sizeof(reg));
    reg.value = 0x25u;
    reg.writable = 1u;
    init_state(&state, &system_backend, &turbo_backend, &reg);
    require(s64_system_state_capture(&state) == S64_SYSTEM_OK,
        "failure case capture succeeds");
    reg.writable = 0u;
    require(
        s64_system_state_set_mode(&state, S64_SYSTEM_MODE_CPU_INTENSIVE)
            == S64_SYSTEM_APPLY_FAILED,
        "failed turbo write is reported through manager"
    );
    require(reg.value == 0x25u, "failed acceleration leaves original value");

    puts("turbo backend tests passed");
    return 0;
}
