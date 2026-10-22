#include "publisher/settings_session.h"

#include "platform/reu.h"
#include "platform/reu_layout.h"

#include <string.h>

uint8_t s64_settings_session_revoke(void)
{
    uint8_t record[S64_SETTINGS_SESSION_SIZE];
    uint8_t readback[S64_SETTINGS_SESSION_SIZE];

    memset(record, 0, sizeof(record));
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
