#include "publisher/settings_session.h"

#include "platform/reu.h"
#include "platform/reu_layout.h"

#include <string.h>

#define S64_SETTINGS_SESSION_MAGIC_0 0x53u
#define S64_SETTINGS_SESSION_MAGIC_1 0x36u
#define S64_SETTINGS_SESSION_MAGIC_2 0x34u
#define S64_SETTINGS_SESSION_MAGIC_3 0x54u
#define S64_SETTINGS_SESSION_SETTINGS_CHECKSUM_OFFSET 32u
#define S64_SETTINGS_SESSION_RECORD_CHECKSUM_OFFSET 34u

static void put16(uint8_t* out, uint16_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t* in)
{
    return (uint16_t)(in[0] | ((uint16_t)in[1] << 8));
}

static uint16_t record_checksum(const uint8_t* record)
{
    uint16_t value = 0x64a5u;
    uint8_t i;

    for (i = 0u; i < S64_SETTINGS_SESSION_RECORD_CHECKSUM_OFFSET; ++i) {
        value = (uint16_t)((value << 5) | (value >> 11));
        value ^= record[i];
    }
    return value ? value : 1u;
}

static uint8_t active_binding(
    const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal,
    uint8_t wire[S64_SESSION_JOURNAL_WIRE_SIZE]
)
{
    return s64_reu_runtime_valid(control)
        && journal != 0
        && journal->state == S64_SESSION_STATE_ACTIVE
        && journal->sequence != 0UL
        && journal->session_id == control->session_id
        && journal->package_version == control->app_version
        && s64_session_journal_encode(journal, wire);
}

static uint8_t record_valid(
    const uint8_t record[S64_SETTINGS_SESSION_SIZE],
    const uint8_t journal_wire[S64_SESSION_JOURNAL_WIRE_SIZE]
)
{
    return record[0] == S64_SETTINGS_SESSION_MAGIC_0
        && record[1] == S64_SETTINGS_SESSION_MAGIC_1
        && record[2] == S64_SETTINGS_SESSION_MAGIC_2
        && record[3] == S64_SETTINGS_SESSION_MAGIC_3
        && record[4] == S64_SETTINGS_SESSION_FORMAT_VERSION
        && record[5] == 0u
        && get16(record + 6u) == (uint16_t)sizeof(S64SettingsExchange)
        && memcmp(record + 8u, journal_wire,
            S64_SESSION_JOURNAL_WIRE_SIZE) == 0
        && get16(record + S64_SETTINGS_SESSION_RECORD_CHECKSUM_OFFSET)
            == record_checksum(record);
}

uint8_t s64_settings_session_revoke(void)
{
    uint8_t record[S64_SETTINGS_SESSION_SIZE];
    uint8_t i;

    memset(record, 0, sizeof(record));
    if (!s64_reu_store(
            S64_REU_SETTINGS_BANK,
            S64_REU_SETTINGS_SESSION_BASE,
            record,
            sizeof(record)
        )
        || !s64_reu_load(
            S64_REU_SETTINGS_BANK,
            S64_REU_SETTINGS_SESSION_BASE,
            record,
            sizeof(record)
        )) {
        return 0u;
    }
    for (i = 0u; i < sizeof(record); ++i) {
        if (record[i] != 0u) return 0u;
    }
    return 1u;
}

uint8_t s64_settings_session_publish(
    const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal,
    S64SettingsExchange* settings
)
{
    uint8_t record[S64_SETTINGS_SESSION_SIZE];
    uint8_t journal_wire[S64_SESSION_JOURNAL_WIRE_SIZE];
    uint16_t settings_checksum;

    if (!settings
        || settings->state != S64_SETTINGS_STATE_READY
        || !s64_settings_exchange_validate(settings)
        || !active_binding(control, journal, journal_wire)) {
        return 0u;
    }

    settings_checksum = settings->checksum;
    if (!settings_checksum || !s64_settings_session_revoke()
        || !s64_reu_store(
            S64_REU_SETTINGS_BANK,
            S64_REU_SETTINGS_BASE,
            settings,
            (uint16_t)sizeof(*settings)
        )
        || !s64_reu_load(
            S64_REU_SETTINGS_BANK,
            S64_REU_SETTINGS_BASE,
            settings,
            (uint16_t)sizeof(*settings)
        )
        || settings->state != S64_SETTINGS_STATE_READY
        || !s64_settings_exchange_validate(settings)
        || settings->checksum != settings_checksum) {
        return 0u;
    }

    memset(record, 0, sizeof(record));
    record[0] = S64_SETTINGS_SESSION_MAGIC_0;
    record[1] = S64_SETTINGS_SESSION_MAGIC_1;
    record[2] = S64_SETTINGS_SESSION_MAGIC_2;
    record[3] = S64_SETTINGS_SESSION_MAGIC_3;
    record[4] = S64_SETTINGS_SESSION_FORMAT_VERSION;
    put16(record + 6u, (uint16_t)sizeof(*settings));
    memcpy(record + 8u, journal_wire, sizeof(journal_wire));
    put16(record + S64_SETTINGS_SESSION_SETTINGS_CHECKSUM_OFFSET,
        settings_checksum);
    put16(record + S64_SETTINGS_SESSION_RECORD_CHECKSUM_OFFSET,
        record_checksum(record));

    if (!s64_reu_store(
            S64_REU_SETTINGS_BANK,
            S64_REU_SETTINGS_SESSION_BASE,
            record,
            sizeof(record)
        )) {
        return 0u;
    }
    memset(record, 0, sizeof(record));
    return s64_reu_load(
            S64_REU_SETTINGS_BANK,
            S64_REU_SETTINGS_SESSION_BASE,
            record,
            sizeof(record)
        )
        && record_valid(record, journal_wire)
        && get16(record + S64_SETTINGS_SESSION_SETTINGS_CHECKSUM_OFFSET)
            == settings_checksum;
}

uint8_t s64_settings_session_load(
    const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal,
    S64SettingsExchange* settings
)
{
    uint8_t record[S64_SETTINGS_SESSION_SIZE];
    uint8_t journal_wire[S64_SESSION_JOURNAL_WIRE_SIZE];
    uint16_t settings_checksum;

    if (!settings || !active_binding(control, journal, journal_wire)
        || !s64_reu_load(
            S64_REU_SETTINGS_BANK,
            S64_REU_SETTINGS_SESSION_BASE,
            record,
            sizeof(record)
        )
        || !record_valid(record, journal_wire)) {
        return 0u;
    }

    settings_checksum =
        get16(record + S64_SETTINGS_SESSION_SETTINGS_CHECKSUM_OFFSET);
    if (!settings_checksum
        || !s64_reu_load(
            S64_REU_SETTINGS_BANK,
            S64_REU_SETTINGS_BASE,
            settings,
            (uint16_t)sizeof(*settings)
        )
        || settings->state != S64_SETTINGS_STATE_READY
        || !s64_settings_exchange_validate(settings)
        || settings->checksum != settings_checksum) {
        memset(settings, 0, sizeof(*settings));
        return 0u;
    }
    return 1u;
}
