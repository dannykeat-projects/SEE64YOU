#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "launcher/recovery_runtime.h"
#include "platform/package_paths.h"
#include "platform/reu_layout.h"
#include "platform/application_state_reserve.h"

typedef struct {
    uint8_t value;
    uint8_t writable;
    unsigned reads, writes;
} FakeRegister;

static uint8_t fake_reu_control[S64_REU_RUNTIME_CONTROL_CAPACITY];
static unsigned manifest_closes, dos_commands;
static unsigned asset_directory_selections;
static uint8_t fake_session[S64_MODULE_SESSION_WIRE_SIZE];
static uint8_t fake_capsule[S64_TRANSFER_CAPSULE_WIRE_SIZE];
static uint8_t fail_session_store, fail_clean;
static unsigned clean_saves;

uint8_t s64_reu_store(
    uint8_t bank,
    uint16_t address,
    const void* source,
    uint16_t length
)
{
    if (bank == 5u && address == 0x100u && length == sizeof(fake_session)) {
        if (fail_session_store) return 0u;
        memcpy(fake_session, source, length);
        return 1u;
    }
    if (bank == 5u && address == 0x200u && length == sizeof(fake_capsule)) {
        memcpy(fake_capsule, source, length);
        return 1u;
    }
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
    if (bank == 5u && address == 0x100u && length == sizeof(fake_session)) {
        memcpy(destination, fake_session, length);
        return 1u;
    }
    if (bank == 5u && address == 0x200u && length == sizeof(fake_capsule)) {
        memcpy(destination, fake_capsule, length);
        return 1u;
    }
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
    ++((FakeRegister*)context)->reads;
    return ((FakeRegister*)context)->value;
}

static void fake_write(void* context, uint8_t value)
{
    FakeRegister* reg = (FakeRegister*)context;
    ++reg->writes;
    if (reg->writable) reg->value = value;
}

static S64UciResult mock_execute(
    const S64UciCommandPart* parts,
    uint8_t part_count,
    S64UciReply* reply
)
{
    uint8_t opcode;
    ++dos_commands;

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

    if (opcode == 0x02u || opcode == 0x03u) {
        if (opcode == 0x03u) ++manifest_closes;
        strcpy(reply->status, "00,OK");
        reply->status_length = 5u;
        return S64_UCI_OK;
    }
    if (opcode == 0x04u) {
        static const char manifest[] =
            "S64MODULES1\n"
            "EDITOR|S64EDIT.PRG|0801|080D|48|1234ABCD\n"
            "PUBLISHER|S64PUB.PRG|0801|080D|48|1234ABCD\n"
            "SETTINGS|S64CFG.PRG|0801|080D|48|1234ABCD\n";
        require(reply->data_sink((const uint8_t*)manifest,
            sizeof(manifest) - 1u, reply->data_context), "manifest sink accepts ASCII data");
        return S64_UCI_OK;
    }

    if (opcode == 0x11u) {
        static const char ok[] = "00,OK";
        if (part_count == 2u && parts[1].length == strlen(S64_PACKAGE_ASSETS_DIRECTORY)
            && !memcmp(parts[1].data, S64_PACKAGE_ASSETS_DIRECTORY,
                       strlen(S64_PACKAGE_ASSETS_DIRECTORY))) ++asset_directory_selections;
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

static S64SessionJournalResult save_clean_test(void* context, S64SessionJournal* journal)
{
    S64LauncherRecoveryRuntime* runtime = (S64LauncherRecoveryRuntime*)context;
    require(journal->state == S64_SESSION_STATE_CLEAN,
        "cleanup attempts only a CLEAN supersession");
    require(runtime->module_session_invalidated && fake_session[0] == 0u
        && fake_capsule[0] == 0u && runtime->system_state.restored,
        "shared revocation and verified restoration precede CLEAN");
    ++clean_saves;
    if (fail_clean) return S64_SESSION_JOURNAL_IO_ERROR;
    ++journal->sequence;
    return S64_SESSION_JOURNAL_OK;
}

int main(void)
{
    S64LauncherRecoveryRuntime runtime, destination_runtime;
    S64SessionJournal wrong_journal;
    unsigned prior_commands, prior_writes;
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
    require(!s64_launcher_module_manifest_load(0),
        "null module manifest runtime is rejected");

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
    require(s64_launcher_module_cache_plan(&runtime),
        "launcher allocates and persists three module cache slots");
    require(runtime.reu_control.allocation_count == S64_MODULE_COUNT + 3u,
        "launcher control block records module caches, shared session, transfer capsule and application state");
    require(runtime.reu_control.allocations[S64_MODULE_COUNT + 2u].flags
            == S64_APPLICATION_STATE_FLAG,
        "launcher reserves the single shared application-state record");
    require(!s64_module_descriptor_loadable(&runtime.module_manifest.modules[0]),
        "cache planning does not pretend unmeasured module is loadable");
    require(s64_launcher_module_manifest_load(&runtime),
        "launcher reads and parses wire ASCII manifest through DOS");
    require(manifest_closes == 1u, "manifest handle closes after read");
    require(asset_directory_selections > 0u,
        "module manifest is fetched from SYSTEM rather than the PRG directory");
    require(s64_session_journal_prepare(&runtime.lifecycle.journal,
        runtime.reu_control.session_id, runtime.reu_control.app_version, 0x2345UL,
        &runtime.system_state.snapshot), "fixture retains the real captured restore state");
    runtime.lifecycle.journal.sequence = 3u;
    require(!s64_launcher_module_session_clear(&runtime),
        "shared metadata requires IO_SAFE scope");
    require(s64_system_state_set_mode(&runtime.system_state,
        S64_SYSTEM_MODE_IO_SAFE) == S64_SYSTEM_OK, "enter metadata IO_SAFE scope");
    require(s64_launcher_module_session_clear(&runtime), "shared readiness is revoked");
    require(!s64_launcher_module_session_publish(&runtime),
        "unverified module package cannot publish metadata");
    {
        uint8_t i;
        for (i = 0u; i < S64_MODULE_COUNT; ++i)
            runtime.module_manifest.modules[i].cache_state = S64_MODULE_CACHE_READY;
    }
    require(s64_launcher_module_session_publish(&runtime),
        "verified metadata round trips through the consumer load API");
    runtime.lifecycle.clean_record_written = 1u;
    require(!s64_launcher_module_session_publish(&runtime),
        "clean launcher cannot republish shared readiness");
    runtime.lifecycle.clean_record_written = 0u;
    require(
        s64_launcher_session_prepare_transition(&runtime) == S64_SYSTEM_OK,
        "active launcher session enters transition mode"
    );
    require(runtime.system_state.mode == S64_SYSTEM_MODE_TRANSITION,
        "transition mode is recorded");
    require(s64_launcher_transfer_capsule_prepare(&runtime, S64_MODULE_EDITOR),
        "transition prepares and verifies editor capsule");
    require(fake_capsule[0] == 'S' && fake_capsule[3] == 'T'
        && fake_capsule[6] == S64_MODULE_EDITOR && fake_capsule[7] == 0u,
        "editor capsule is published to allocator-owned REU");
    {
        S64ModuleTransferPlan plan;
        require(s64_launcher_transfer_plan_load(&runtime, &plan),
            "launcher derives transfer plan from verified capsule");
        require(plan.reu_source
                == runtime.module_manifest.modules[0].cache_address + 2UL
            && plan.ram_destination
                == runtime.module_manifest.modules[0].load_address
            && plan.payload_length
                == runtime.module_manifest.modules[0].byte_length - 2UL
            && plan.startup_address
                == runtime.module_manifest.modules[0].startup_address,
            "transfer plan skips PRG header and preserves exact editor targets");
    }
    require(!s64_launcher_transfer_capsule_prepare(&runtime, (S64ModuleId)0),
        "invalid transfer destination is rejected");
    require(runtime.lifecycle.active_record_written,
        "transition keeps durable session logically active");
    require(
        s64_launcher_session_resume_interactive(&runtime) == S64_SYSTEM_OK,
        "cancelled handoff resumes interactive mode"
    );
    require(runtime.system_state.mode == S64_SYSTEM_MODE_INTERACTIVE,
        "interactive mode restored without cleaning session");

    require(s64_launcher_session_prepare_transition(&runtime) == S64_SYSTEM_OK,
        "caller reaches original-state TRANSITION before adoption");
    s64_launcher_recovery_runtime_init(&destination_runtime, mock_execute,
        fake_read, fake_write, &reg);
    prior_commands = dos_commands;
    prior_writes = reg.writes;
    wrong_journal = runtime.lifecycle.journal;
    wrong_journal.state = S64_SESSION_STATE_CLEAN;
    require(!s64_launcher_module_session_adopt(&destination_runtime, S64_MODULE_EDITOR,
        0x201u, &wrong_journal), "CLEAN transfer cannot adopt a session");
    require(!s64_launcher_module_session_adopt(&destination_runtime, S64_MODULE_EDITOR,
        0x202u, &runtime.lifecycle.journal), "different compiled package cannot adopt");
    require(!s64_launcher_module_session_adopt(&destination_runtime, (S64ModuleId)0,
        0x201u, &runtime.lifecycle.journal), "invalid destination cannot adopt");
    reg.value = 0x2fu;
    require(!s64_launcher_module_session_adopt(&destination_runtime, S64_MODULE_EDITOR,
        0x201u, &runtime.lifecycle.journal), "unexpected turbo at TRANSITION rejects adoption");
    require(!destination_runtime.system_state.captured
        && !destination_runtime.lifecycle.active_record_written,
        "rejected adoption claims no machine/session ownership");
    reg.value = 0x25u;
    require(s64_launcher_module_session_adopt(&destination_runtime, S64_MODULE_EDITOR,
        0x201u, &runtime.lifecycle.journal), "verified destination adopts the ACTIVE session");
    require(dos_commands == prior_commands && reg.writes == prior_writes,
        "adoption does not use DOS or write/probe turbo");
    require(destination_runtime.lifecycle.journal.sequence == 3u
        && destination_runtime.lifecycle.journal.recovery_ref == 0x2345UL
        && destination_runtime.system_state.snapshot.turbo_control == 0x25u
        && destination_runtime.system_state.mode == S64_SYSTEM_MODE_TRANSITION
        && destination_runtime.current_module == S64_MODULE_EDITOR,
        "adoption retains the original journal and restore state");
    require(!s64_launcher_module_session_adopt(&destination_runtime, S64_MODULE_EDITOR,
        0x201u, &runtime.lifecycle.journal), "owned destination cannot adopt twice");
    require(s64_system_state_set_mode(&destination_runtime.system_state,
        S64_SYSTEM_MODE_CPU_INTENSIVE) == S64_SYSTEM_OK && reg.value == 0x2fu,
        "adopted destination may accelerate through the common manager");
    require(!destination_runtime.module_session_invalidated, "published metadata needs exit revocation");
    destination_runtime.lifecycle.store.save = save_clean_test;
    destination_runtime.lifecycle.store.context = &destination_runtime;
    /* Overlay allocations extend the five original entries. Cleanup must
     * still revoke capsule/shared readiness before restoring or saving CLEAN. */
    {
        uint8_t i;
        for (i = 0u; i < 4u; ++i) {
            require(s64_reu_runtime_allocate(&destination_runtime.reu_control,
                S64_REU_OWNER_RUNTIME, 16UL, 256u, 0), "reserve later overlay entries");
        }
    }
    require(destination_runtime.reu_control.allocation_count == 10u,
        "extended allocator control includes shared application state on the cleanup regression path");
    fail_session_store = 1u;
    require(s64_launcher_session_end(&destination_runtime) == S64_SESSION_LIFECYCLE_RESTORE_FAILED,
        "failed shared revocation blocks cleanup");
    require(clean_saves == 0u && !destination_runtime.system_state.restored
        && destination_runtime.lifecycle.journal.state == S64_SESSION_STATE_ACTIVE,
        "revocation failure retains ACTIVE without recording CLEAN");
    fail_session_store = 0u;
    fail_clean = 1u;
    require(s64_launcher_session_end(&destination_runtime) == S64_SESSION_LIFECYCLE_JOURNAL_CLEAN_FAILED,
        "failed CLEAN save remains visible after revocation and restoration");
    require(destination_runtime.module_session_invalidated && destination_runtime.system_state.restored,
        "CLEAN retry does not need to reapply a mode after restoration");
    fail_clean = 0u;
    require(s64_launcher_session_end(&destination_runtime) == S64_SESSION_LIFECYCLE_OK,
        "launcher can retry CLEAN after verified restore");
    require(clean_saves == 2u, "both CLEAN save attempts occur after revocation");
    require(destination_runtime.module_session_invalidated
        && fake_session[0] == 0u && fake_capsule[0] == 0u,
        "closed session has no shared readiness or transfer capsule");
    puts("launcher recovery runtime tests passed");
    return 0;
}
