#include <stdio.h>
#include <string.h>

#include "platform/reu.h"
#include "platform/reu_layout.h"
#include "platform/reu_runtime.h"
#include "platform/session_journal.h"
#include "publisher/settings_exchange.h"
#include "publisher/settings_session.h"

static uint8_t reu_bank1[65536u];
static unsigned failures;

static void check(const char* label, int okay)
{
    if (!okay) {
        fprintf(stderr, "FAIL: %s\n", label);
        ++failures;
    }
}

uint8_t s64_reu_store(
    uint8_t bank,
    uint16_t address,
    const void* source,
    uint16_t length
)
{
    uint32_t end = (uint32_t)address + length;
    if (bank != 1u || !source || end > sizeof(reu_bank1)) return 0u;
    memcpy(reu_bank1 + address, source, length);
    return 1u;
}

uint8_t s64_reu_load(
    uint8_t bank,
    uint16_t address,
    void* destination,
    uint16_t length
)
{
    uint32_t end = (uint32_t)address + length;
    if (bank != 1u || !destination || end > sizeof(reu_bank1)) return 0u;
    memcpy(destination, reu_bank1 + address, length);
    return 1u;
}

uint8_t s64_reu_runtime_valid(const S64ReuRuntimeControl* control)
{
    return control != 0
        && control->format_version == S64_REU_RUNTIME_FORMAT_VERSION
        && control->abi_version == S64_REU_RUNTIME_ABI_VERSION
        && control->app_version != 0u
        && control->session_id != 0UL;
}

static void put16(uint8_t* out, uint16_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
}

static void put32(uint8_t* out, uint32_t value)
{
    put16(out, (uint16_t)value);
    put16(out + 2u, (uint16_t)(value >> 16));
}

uint8_t s64_session_journal_encode(
    const S64SessionJournal* journal,
    uint8_t wire[S64_SESSION_JOURNAL_WIRE_SIZE]
)
{
    if (!journal || !wire || !journal->sequence) return 0u;
    memset(wire, 0, S64_SESSION_JOURNAL_WIRE_SIZE);
    wire[0] = S64_SESSION_JOURNAL_FORMAT_VERSION;
    wire[1] = journal->state;
    put16(wire + 2u, journal->package_version);
    put32(wire + 4u, journal->session_id);
    put32(wire + 8u, journal->sequence);
    put32(wire + 12u, journal->recovery_ref);
    wire[16] = journal->restore_flags;
    wire[17] = journal->turbo_control;
    /* Give the final bytes deterministic session-specific data too. */
    put16(wire + 18u, (uint16_t)(journal->session_id ^ journal->sequence));
    put32(wire + 20u, journal->session_id ^ journal->sequence);
    return 1u;
}

static void init_active(
    S64ReuRuntimeControl* control,
    S64SessionJournal* journal
)
{
    memset(control, 0, sizeof(*control));
    control->format_version = S64_REU_RUNTIME_FORMAT_VERSION;
    control->abi_version = S64_REU_RUNTIME_ABI_VERSION;
    control->app_version = 514u;
    control->session_id = 0x12345678UL;

    memset(journal, 0, sizeof(*journal));
    journal->state = S64_SESSION_STATE_ACTIVE;
    journal->package_version = control->app_version;
    journal->session_id = control->session_id;
    journal->sequence = 7UL;
    journal->recovery_ref = 0x11223344UL;
}

static void init_settings(S64SettingsExchange* settings)
{
    s64_settings_exchange_init(settings);
    settings->destination_type = (uint8_t)S64_DESTINATION_STANDALONE;
    strcpy(settings->base_url, "http://example.invalid");
    settings->allow_insecure_http = 1u;
    strcpy(settings->direct_key,
        "0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef");
    settings->state = S64_SETTINGS_STATE_READY;
    settings->result = S64_SETTINGS_RESULT_OK;
    settings->checksum = s64_settings_exchange_checksum(settings);
}

static int all_zero(const void* bytes, size_t length)
{
    const uint8_t* p = (const uint8_t*)bytes;
    size_t i;
    for (i = 0u; i < length; ++i) {
        if (p[i]) return 0;
    }
    return 1;
}

int main(void)
{
    S64ReuRuntimeControl control;
    S64SessionJournal journal;
    S64SessionJournal stale;
    S64SettingsExchange settings;
    S64SettingsExchange loaded;
    S64SettingsExchange changed;
    uint8_t sentinel[8];
    uint8_t record[S64_SETTINGS_SESSION_SIZE];
    unsigned cycle;

    memset(reu_bank1, 0, sizeof(reu_bank1));
    init_active(&control, &journal);
    init_settings(&settings);

    memset(sentinel, 0xa5, sizeof(sentinel));
    memcpy(reu_bank1 + S64_REU_HANDOFF_BASE, sentinel, sizeof(sentinel));

    check("canonical settings valid", s64_settings_exchange_validate(&settings));
    check("publish valid ACTIVE settings",
        s64_settings_session_publish(&control, &journal, &settings));

    memset(&loaded, 0, sizeof(loaded));
    check("load exact ACTIVE settings",
        s64_settings_session_load(&control, &journal, &loaded));
    check("round trip exact",
        memcmp(&settings, &loaded, sizeof(settings)) == 0);
    check("legacy handoff untouched",
        memcmp(reu_bank1 + S64_REU_HANDOFF_BASE,
            sentinel, sizeof(sentinel)) == 0);

    stale = journal;
    ++stale.sequence;
    check("stale sequence rejected",
        !s64_settings_session_load(&control, &stale, &loaded));

    stale = journal;
    ++stale.session_id;
    check("wrong session rejected",
        !s64_settings_session_load(&control, &stale, &loaded));

    memcpy(record,
        reu_bank1 + S64_REU_SETTINGS_SESSION_BASE,
        sizeof(record));
    reu_bank1[S64_REU_SETTINGS_SESSION_BASE + 10u] ^= 0x40u;
    check("corrupt authority record rejected",
        !s64_settings_session_load(&control, &journal, &loaded));
    memcpy(reu_bank1 + S64_REU_SETTINGS_SESSION_BASE,
        record, sizeof(record));

    reu_bank1[S64_REU_SETTINGS_BASE + 12u] ^= 0x01u;
    memset(&loaded, 0x5a, sizeof(loaded));
    check("corrupt canonical settings rejected",
        !s64_settings_session_load(&control, &journal, &loaded));
    check("corrupt settings output cleared",
        all_zero(&loaded, sizeof(loaded)));

    init_settings(&settings);
    check("republish after corruption",
        s64_settings_session_publish(&control, &journal, &settings));

    changed = settings;
    strcpy(changed.base_url, "http://changed.invalid");
    changed.checksum = s64_settings_exchange_checksum(&changed);
    check("changed settings remain canonical",
        s64_settings_exchange_validate(&changed));
    check("simulate helper settings update",
        s64_reu_store(S64_REU_SETTINGS_BANK, S64_REU_SETTINGS_BASE,
            &changed, (uint16_t)sizeof(changed)));
    memset(&loaded, 0x5a, sizeof(loaded));
    check("old authority rejects helper update",
        !s64_settings_session_load(&control, &journal, &loaded));
    check("helper-update rejection clears output",
        all_zero(&loaded, sizeof(loaded)));

    check("republish changed settings",
        s64_settings_session_publish(&control, &journal, &changed));
    memset(&loaded, 0, sizeof(loaded));
    check("changed settings accepted after republish",
        s64_settings_session_load(&control, &journal, &loaded)
        && memcmp(&changed, &loaded, sizeof(changed)) == 0);

    /* Repeated Settings round trips in one ACTIVE session must replace
     * authority cleanly rather than accumulating stale session state. */
    for (cycle = 0u; cycle < 8u; ++cycle) {
        changed = settings;
        strcpy(changed.base_url, cycle & 1u
            ? "http://cycle-a.invalid"
            : "http://cycle-b.invalid");
        changed.checksum = s64_settings_exchange_checksum(&changed);
        check("cycle settings canonical",
            s64_settings_exchange_validate(&changed));
        check("cycle settings publish",
            s64_settings_session_publish(&control, &journal, &changed));
        memset(&loaded, 0, sizeof(loaded));
        check("cycle settings load",
            s64_settings_session_load(&control, &journal, &loaded));
        check("cycle settings exact",
            memcmp(&changed, &loaded, sizeof(changed)) == 0);
    }

    check("revoke authority", s64_settings_session_revoke());
    memset(&loaded, 0, sizeof(loaded));
    check("revoked authority rejected",
        !s64_settings_session_load(&control, &journal, &loaded));
    check("authority bytes zero after revoke",
        all_zero(reu_bank1 + S64_REU_SETTINGS_SESSION_BASE,
            S64_SETTINGS_SESSION_SIZE));
    check("revoke leaves canonical settings intact",
        memcmp(reu_bank1 + S64_REU_SETTINGS_BASE,
            &changed, sizeof(changed)) == 0);

    if (failures) {
        fprintf(stderr, "%u settings-session tests failed\n", failures);
        return 1;
    }
    puts("settings session tests passed");
    return 0;
}
