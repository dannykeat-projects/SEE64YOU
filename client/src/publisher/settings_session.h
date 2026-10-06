#ifndef SEE64YOU_PUBLISHER_SETTINGS_SESSION_H
#define SEE64YOU_PUBLISHER_SETTINGS_SESSION_H

#include <stdint.h>

#include "platform/reu_runtime.h"
#include "platform/session_journal.h"
#include "publisher/settings_exchange.h"

#define S64_SETTINGS_SESSION_FORMAT_VERSION 1u
#define S64_SETTINGS_SESSION_SIZE 40u

/* Remove managed-session authority without changing persistent settings. */
uint8_t s64_settings_session_revoke(void);

/* Publish canonical READY settings into the existing fixed REU settings region,
 * then publish the ACTIVE-bound authority record last. The settings object is
 * reused for verified readback. */
uint8_t s64_settings_session_publish(
    const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal,
    S64SettingsExchange* settings
);

/* Consume only settings bound to this exact ACTIVE runtime/journal. */
uint8_t s64_settings_session_load(
    const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal,
    S64SettingsExchange* settings
);

#endif
