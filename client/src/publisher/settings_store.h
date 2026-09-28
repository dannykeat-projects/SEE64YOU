#ifndef SEE64YOU_PUBLISHER_SETTINGS_STORE_H
#define SEE64YOU_PUBLISHER_SETTINGS_STORE_H

#include "platform/dos.h"
#include "publisher/settings_exchange.h"

#define S64_SETTINGS_FILENAME "SEE64YOU.CFG"

typedef enum {
    S64_SETTINGS_STORE_OK = 0,
    S64_SETTINGS_STORE_NOT_FOUND,
    S64_SETTINGS_STORE_INVALID,
    S64_SETTINGS_STORE_IO_ERROR
} S64SettingsStoreResult;

S64SettingsStoreResult s64_settings_store_load(
    S64Dos* dos,
    S64SettingsExchange* settings
);

S64SettingsStoreResult s64_settings_store_save(
    S64Dos* dos,
    S64SettingsExchange* settings
);

#endif
