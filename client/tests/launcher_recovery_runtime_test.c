#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "launcher/recovery_runtime.h"
#include "platform/reu_layout.h"

typedef struct {
    uint8_t value;
    uint8_t writable;
} FakeRegister;

static uint8_t fake_reu_control[S64_REU_RUNTIME_CONTROL_CAPACITY];

uint8_t s64_reu_store(
    uint8_t bank,
    uint16_t address,
    const void* source,
    uint16_t length
)
{
    if (bank != S64_REU_RUNTIME_CONTROL_BANK
        || address != S64_REU_RUNTIME_CONTROL_BASE
        || source == 0
        || length > sizeof(fake_reu_control)) {
        return 0u;
    }
    memcpy(fake_reu_control, source, length);
    return 1u;
}

uint8_t s64_reu_load(
    uint8_t bank,
    uint16_t address,
    void* destination,
    uint16_t length
)
{
    if (bank != S64_REU_RUNTIME_CONTROL_BANK
        || address != S64_REU_RUNTIME_CONTROL_BASE
        || destination == 0
        || length > sizeof(fake_reu_control)) {
        return 0u;
    }
    memcpy(destination, fake_reu_control, length);
    return 1u;
}

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static uint8_t fake_read(void* context)
{
    return ((FakeRegister*)context)->value;
}

static void fake_write(void* context, uint8_t value)
{
    FakeRegister* reg = (FakeRegister*)context;
    if (reg->writable) reg->value = value;
}

static S64UciResult mock_execute(
    const S64UciCommandPart* parts,
    uint8_t part_count,
    S64UciReply* reply
)
{
    uint8_t opcode;

    if (part_count == 0u || parts[0].length < 2u
        || parts[0].data[0] != S64_DOS_TARGET_PRIMARY) {
        return S64_UCI_PROTOCOL_ERROR;
    }

    opcode = parts[0].data[1];
    if (reply != 0) {
        reply->status_length = 0u;
        if (reply->status != 0 && reply->status_capacity != 0u) {
            reply->status[0] = '\0';
        }
    }

    if (opcode == 0x11u) {
        static const char ok[] = "00,OK";
        if (reply != 0 && reply->status != 0
            && reply->status_capacity > sizeof(ok)) {
            memcpy(reply->status, ok, sizeof(ok));
            reply->status_length = sizeof(ok) - 1u;
        }
        return S64_UCI_OK;
    }

    if (opcode == 0x08u) {
        static const char missing[] = "88,FILE NOT FOUND";
        if (reply != 0 && reply->status != 0
            && reply->status_capacity > sizeof(missing)) {
            memcpy(reply->status, missing, sizeof(missing));
            reply->status_length = sizeof(missing) - 1u;
        }
        return S64_UCI_OK;
    }

    return S64_UCI_PROTOCOL_ERROR;
}

int main(void)
{
    S64LauncherRecoveryRuntime runtime;
    FakeRegister reg;
    S64LauncherRecoveryResult result;

    memset(&reg, 0, sizeof(reg));
    reg.value = 0x25u;
    reg.writable = 1u;

    s64_launcher_recovery_runtime_init(
        &runtime,
        mock_execute,
        fake_read,
        fake_write,
        &reg
    );

    require(runtime.dos.target == S64_DOS_TARGET_PRIMARY,
        "launcher recovery uses primary DOS target");
    require(runtime.store.load != 0 && runtime.store.save != 0,
        "launcher owns journal load/save callbacks");
    require(runtime.recovery_backend.restore != 0
        && runtime.recovery_backend.verify != 0,
        "launcher owns restore/verify callbacks");
    require(runtime.lifecycle.system_state == &runtime.system_state,
        "launcher runtime owns lifecycle system state");
    require(runtime.lifecycle.store.save != 0,
        "launcher runtime owns lifecycle journal save callback");

    result = s64_launcher_recovery_runtime_run(&runtime);
    require(result == S64_LAUNCHER_RECOVERY_READY_NONE,
        "missing journal proceeds through real DOS adapter");

    require(
        s64_launcher_recovery_runtime_run(0)
            == S64_LAUNCHER_RECOVERY_INVALID,
        "null runtime is rejected"
    );
    require(
        s64_launcher_session_begin(
            0, 1UL, 1u, 0UL, S64_SYSTEM_MODE_INTERACTIVE
        ) == S64_SESSION_LIFECYCLE_INVALID,
        "null lifecycle runtime begin is rejected"
    );
    require(
        s64_launcher_session_end(0) == S64_SESSION_LIFECYCLE_INVALID,
        "null lifecycle runtime end is rejected"
    );
    require(
        s64_launcher_session_prepare_transition(0) == S64_SYSTEM_INVALID,
        "null transition runtime is rejected"
    );
    require(
        s64_launcher_session_resume_interactive(0) == S64_SYSTEM_INVALID,
        "null resume runtime is rejected"
    );
    require(!s64_launcher_runtime_control_create(0),
        "null runtime control creation is rejected");

    require(
        s64_system_state_capture(&runtime.system_state) == S64_SYSTEM_OK,
        "test runtime captures system state"
    );
    runtime.lifecycle.active_record_written = 1u;
    runtime.lifecycle.clean_record_written = 0u;
    runtime.lifecycle.journal.session_id = 0x11223344UL;
    runtime.lifecycle.journal.package_version = 0x0201u;
    memset(fake_reu_control, 0, sizeof(fake_reu_control));
    require(s64_launcher_runtime_control_create(&runtime),
        "launcher creates and verifies active-session REU control block");
    require(runtime.reu_control.session_id == 0x11223344UL
        && runtime.reu_control.app_version == 0x0201u,
        "launcher control block inherits journal session metadata");
    require(
        s64_launcher_session_prepare_transition(&runtime) == S64_SYSTEM_OK,
        "active launcher session enters transition mode"
    );
    require(runtime.system_state.mode == S64_SYSTEM_MODE_TRANSITION,
        "transition mode is recorded");
    require(runtime.lifecycle.active_record_written,
        "transition keeps durable session logically active");
    require(
        s64_launcher_session_resume_interactive(&runtime) == S64_SYSTEM_OK,
        "cancelled handoff resumes interactive mode"
    );
    require(runtime.system_state.mode == S64_SYSTEM_MODE_INTERACTIVE,
        "interactive mode restored without cleaning session");

    puts("launcher recovery runtime tests passed");
    return 0;
}
