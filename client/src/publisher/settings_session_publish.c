#include "publisher/settings_session.h"

#include "platform/reu.h"
#include "platform/reu_layout.h"

#include <string.h>

#define SESSION_CHECKSUM_OFFSET 34u

static void put16(uint8_t* out, uint16_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
}

static uint16_t checksum(const uint8_t* record)
{
    uint16_t value = 0x64a5u;
    uint8_t i;

    for (i = 0u; i < SESSION_CHECKSUM_OFFSET; ++i) {
        value = (uint16_t)((value << 5) | (value >> 11));
        value ^= record[i];
    }
    return value ? value : 1u;
}

uint8_t s64_settings_session_publish(
    const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal,
    S64SettingsExchange* settings
)
{
    uint8_t record[S64_SETTINGS_SESSION_SIZE];
    uint8_t readback[S64_SETTINGS_SESSION_SIZE];
    uint16_t settings_checksum;

    /* Editor PREP revokes old authority before DOS I/O, streams the exact
     * persistent settings bytes into the fixed REU region, reads them back
     * into this object and validates them canonically before calling here.
     * Publish only the ACTIVE binding; do not rewrite/re-read identical data. */
    if (!settings
        || settings->state != S64_SETTINGS_STATE_READY
        || !s64_settings_exchange_validate(settings)
        || !control
        || !journal
        || journal->state != S64_SESSION_STATE_ACTIVE
        || !journal->sequence
        || journal->session_id != control->session_id
        || journal->package_version != control->app_version) {
        return 0u;
    }

    settings_checksum = settings->checksum;
    if (!settings_checksum) return 0u;

    memset(record, 0, sizeof(record));
    record[0] = 0x53u;
    record[1] = 0x36u;
    record[2] = 0x34u;
    record[3] = 0x54u;
    record[4] = S64_SETTINGS_SESSION_FORMAT_VERSION;
    put16(record + 6u, (uint16_t)sizeof(*settings));
    if (!s64_session_journal_encode(journal, record + 8u)) return 0u;
    put16(record + 32u, settings_checksum);
    put16(record + SESSION_CHECKSUM_OFFSET, checksum(record));

    return s64_reu_store(
            S64_REU_SETTINGS_BANK,
            S64_REU_SETTINGS_SESSION_BASE,
            record,
            sizeof(record)
        )
        && s64_reu_load(
            S64_REU_SETTINGS_BANK,
            S64_REU_SETTINGS_SESSION_BASE,
            readback,
            sizeof(readback)
        )
        && memcmp(record, readback, sizeof(record)) == 0;
}
