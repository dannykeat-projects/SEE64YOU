#ifndef SEE64YOU_PUBLISHER_SETTINGS_BRIDGE_H
#define SEE64YOU_PUBLISHER_SETTINGS_BRIDGE_H

#include "publisher/settings_exchange.h"

int s64_settings_bridge_load_ready(S64SettingsExchange* settings);
int s64_settings_bridge_request_load(S64SettingsExchange* settings);
int s64_settings_bridge_request_edit(void);

#endif
