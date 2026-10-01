#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/session_lifecycle.h"

typedef struct {
    char events[32];
    uint8_t event_count;
    uint8_t capture_ok;
    uint8_t apply_ok;
    uint8_t restore_ok;
    uint8_t verify_ok;
    uint8_t save_active_ok;
    uint8_t save_clean_ok;
    uint8_t turbo_control;
    uint8_t active_saves;
    uint8_t clean_saves;
} Mock;

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void event(Mock* mock, char value)
{
    require(mock->event_count + 1u < sizeof(mock->events), "event capacity");
    mock->events[mock->event_count++] = value;
    mock->events[mock->event_count] = '\0';
}

static Mock ready_mock(void)
{
    Mock mock;
    memset(&mock, 0, sizeof(mock));
    mock.capture_ok = 1u;
    mock.apply_ok = 1u;
    mock.restore_ok = 1u;
    mock.verify_ok = 1u;
    mock.save_active_ok = 1u;
    mock.save_clean_ok = 1u;
    mock.turbo_control = 0x25u;
    return mock;
}

static uint8_t capture(void* context, S64SystemSnapshot* snapshot)
{
    Mock* mock = (Mock*)context;
    event(mock, 'C');
    if (!mock->capture_ok) return 0u;
    snapshot->turbo_control = mock->turbo_control;
    snapshot->turbo_control_valid = 1u;
    return 1u;
}

static uint8_t apply(void* context, S64SystemMode mode)
{
    Mock* mock = (Mock*)context;
    event(mock, 'A');
    require(mode == S64_SYSTEM_MODE_INTERACTIVE, "expected initial mode");
    return mock->apply_ok;
}

static uint8_t restore(void* context, const S64SystemSnapshot* snapshot)
{
    Mock* mock = (Mock*)context;
    event(mock, 'R');
    require(snapshot->turbo_control_valid, "restore has captured turbo");
    require(snapshot->turbo_control == mock->turbo_control,
        "restore gets original turbo");
    return mock->restore_ok;
}

static uint8_t verify(void* context, const S64SystemSnapshot* snapshot)
{
    Mock* mock = (Mock*)context;
    event(mock, 'V');
    require(snapshot->turbo_control == mock->turbo_control,
        "verify gets original turbo");
    return mock->verify_ok;
}

static S64SessionJournalResult save_journal(
    void* context,
    S64SessionJournal* journal
)
{
    Mock* mock = (Mock*)context;

    if (journal->state == S64_SESSION_STATE_ACTIVE) {
        event(mock, 'J');
        ++mock->active_saves;
        return mock->save_active_ok
            ? S64_SESSION_JOURNAL_OK
            : S64_SESSION_JOURNAL_IO_ERROR;
    }

    if (journal->state == S64_SESSION_STATE_CLEAN) {
        event(mock, 'K');
        ++mock->clean_saves;
        return mock->save_clean_ok
            ? S64_SESSION_JOURNAL_OK
            : S64_SESSION_JOURNAL_IO_ERROR;
    }

    return S64_SESSION_JOURNAL_INVALID;
}

static void init(
    Mock* mock,
    S64SystemState* system,
    S64SessionLifecycle* lifecycle
)
{
    S64SystemBackend system_backend;
    S64SessionLifecycleStore store;

    system_backend.capture = capture;
    system_backend.apply_mode = apply;
    system_backend.restore = restore;
    system_backend.verify = verify;
    s64_system_state_init(system, &system_backend, mock);

    store.save = save_journal;
    store.context = mock;
    s64_session_lifecycle_init(lifecycle, system, &store);
}

int main(void)
{
    S64SystemState system;
    S64SessionLifecycle lifecycle;
    Mock mock = ready_mock();

    init(&mock, &system, &lifecycle);
    require(
        s64_session_lifecycle_begin(
            &lifecycle,
            0x11223344UL,
            0x0201u,
            0x55667788UL,
            S64_SYSTEM_MODE_INTERACTIVE
        ) == S64_SESSION_LIFECYCLE_OK,
        "normal begin succeeds"
    );
    require(strcmp(mock.events, "CJA") == 0,
        "capture then ACTIVE journal then apply");
    require(lifecycle.active_record_written && !lifecycle.clean_record_written,
        "active journal remains during session");
    require(lifecycle.journal.session_id == 0x11223344UL
        && lifecycle.journal.package_version == 0x0201u
        && lifecycle.journal.recovery_ref == 0x55667788UL,
        "begin records caller session metadata");
    require(
        (lifecycle.journal.restore_flags
            & S64_SESSION_JOURNAL_RESTORE_TURBO) != 0u
        && lifecycle.journal.turbo_control == 0x25u,
        "ACTIVE journal contains captured restorable turbo"
    );

    require(
        s64_session_lifecycle_end(&lifecycle) == S64_SESSION_LIFECYCLE_OK,
        "normal end succeeds"
    );
    require(strcmp(mock.events, "CJARVK") == 0,
        "restore verify precedes CLEAN journal");
    require(!lifecycle.active_record_written && lifecycle.clean_record_written,
        "clean journal completes lifecycle");

    {
        Mock fail = ready_mock();
        S64SystemState fail_system;
        S64SessionLifecycle fail_lifecycle;

        fail.save_active_ok = 0u;
        init(&fail, &fail_system, &fail_lifecycle);
        require(
            s64_session_lifecycle_begin(
                &fail_lifecycle, 1UL, 1u, 0UL,
                S64_SYSTEM_MODE_INTERACTIVE
            ) == S64_SESSION_LIFECYCLE_JOURNAL_ACTIVE_FAILED,
            "ACTIVE journal failure blocks startup"
        );
        require(strcmp(fail.events, "CJ") == 0,
            "no apply occurs after ACTIVE journal failure");
        require(!fail_lifecycle.active_record_written,
            "failed ACTIVE persistence is not claimed");
        require(fail_system.restored,
            "capture-only state is closed after journal failure");
    }

    {
        Mock fail = ready_mock();
        S64SystemState fail_system;
        S64SessionLifecycle fail_lifecycle;

        fail.apply_ok = 0u;
        init(&fail, &fail_system, &fail_lifecycle);
        require(
            s64_session_lifecycle_begin(
                &fail_lifecycle, 2UL, 1u, 0UL,
                S64_SYSTEM_MODE_INTERACTIVE
            ) == S64_SESSION_LIFECYCLE_APPLY_FAILED,
            "apply failure is reported"
        );
        require(strcmp(fail.events, "CJAK") == 0,
            "failed apply is followed by CLEAN journal");
        require(fail_lifecycle.clean_record_written,
            "failed startup cleaned durable journal");
    }

    {
        Mock fail = ready_mock();
        S64SystemState fail_system;
        S64SessionLifecycle fail_lifecycle;

        init(&fail, &fail_system, &fail_lifecycle);
        require(
            s64_session_lifecycle_begin(
                &fail_lifecycle, 3UL, 1u, 0UL,
                S64_SYSTEM_MODE_INTERACTIVE
            ) == S64_SESSION_LIFECYCLE_OK,
            "restore-failure setup begins"
        );
        fail.restore_ok = 0u;
        require(
            s64_session_lifecycle_end(&fail_lifecycle)
                == S64_SESSION_LIFECYCLE_RESTORE_FAILED,
            "restore failure blocks CLEAN record"
        );
        require(strcmp(fail.events, "CJAR") == 0,
            "CLEAN not written when restore fails");
        require(fail_lifecycle.active_record_written
            && !fail_lifecycle.clean_record_written,
            "ACTIVE journal remains after restore failure");

        fail.restore_ok = 1u;
        require(
            s64_session_lifecycle_end(&fail_lifecycle)
                == S64_SESSION_LIFECYCLE_OK,
            "restore failure can be retried"
        );
        require(strcmp(fail.events, "CJARRVK") == 0,
            "retry restores verifies then cleans");
    }

    {
        Mock fail = ready_mock();
        S64SystemState fail_system;
        S64SessionLifecycle fail_lifecycle;

        init(&fail, &fail_system, &fail_lifecycle);
        require(
            s64_session_lifecycle_begin(
                &fail_lifecycle, 4UL, 1u, 0UL,
                S64_SYSTEM_MODE_INTERACTIVE
            ) == S64_SESSION_LIFECYCLE_OK,
            "clean-write-failure setup begins"
        );
        fail.save_clean_ok = 0u;
        require(
            s64_session_lifecycle_end(&fail_lifecycle)
                == S64_SESSION_LIFECYCLE_JOURNAL_CLEAN_FAILED,
            "clean journal failure is reported"
        );
        require(strcmp(fail.events, "CJARVK") == 0,
            "restore verification occurs before failed CLEAN write");
        require(fail_lifecycle.active_record_written,
            "durable state remains logically ACTIVE after clean write failure");

        fail.save_clean_ok = 1u;
        require(
            s64_session_lifecycle_end(&fail_lifecycle)
                == S64_SESSION_LIFECYCLE_OK,
            "clean journal write can be retried after restored state"
        );
        require(strcmp(fail.events, "CJARVKK") == 0,
            "retry only needs idempotent restore plus CLEAN write");
    }

    {
        Mock fail = ready_mock();
        S64SystemState fail_system;
        S64SessionLifecycle fail_lifecycle;

        fail.capture_ok = 0u;
        init(&fail, &fail_system, &fail_lifecycle);
        require(
            s64_session_lifecycle_begin(
                &fail_lifecycle, 5UL, 1u, 0UL,
                S64_SYSTEM_MODE_INTERACTIVE
            ) == S64_SESSION_LIFECYCLE_CAPTURE_FAILED,
            "capture failure stops before persistence"
        );
        require(strcmp(fail.events, "C") == 0,
            "capture failure writes no journal and applies nothing");
    }

    puts("session lifecycle tests passed");
    return 0;
}
