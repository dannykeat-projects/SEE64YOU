#ifndef SEE64YOU_PUBLISHER_PERSISTENT_SETTINGS_CONTROL_H
#define SEE64YOU_PUBLISHER_PERSISTENT_SETTINGS_CONTROL_H

#include <stdint.h>

#include "publisher/settings_exchange.h"

typedef enum {
    S64_SETTINGS_CONTROL_OK = 0,
    S64_SETTINGS_CONTROL_INVALID_ARGUMENT,
    S64_SETTINGS_CONTROL_INVALID_DESTINATION,
    S64_SETTINGS_CONTROL_INVALID_VALUES
} S64PersistentSettingsControlResult;

S64PersistentSettingsControlResult s64_persistent_settings_validate_values(
    S64SettingsExchange* settings
);

#endif
