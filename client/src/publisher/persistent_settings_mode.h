#ifndef SEE64YOU_PUBLISHER_PERSISTENT_SETTINGS_MODE_H
#define SEE64YOU_PUBLISHER_PERSISTENT_SETTINGS_MODE_H

#include <stdint.h>

#include "platform/dos.h"
#include "publisher/persistent_settings_control.h"
#include "publisher/persistent_settings_ui.h"
#include "publisher/settings_exchange.h"

S64PersistentSettingsAction s64_persistent_settings_dispatch_key(
    S64SettingsExchange* settings,
    S64Dos* dos,
    uint8_t* advanced_view,
    uint8_t key,
    S64PersistentSettingsCommitResult* transaction_result
);

#endif
