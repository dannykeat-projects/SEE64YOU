#ifndef SEE64YOU_PUBLISHER_SETTINGS_H
#define SEE64YOU_PUBLISHER_SETTINGS_H

#include <stdint.h>

#include "platform/dos.h"
#include "publisher/destination.h"

#define S64_PUBLISH_SETTINGS_FILENAME "SEE64YOU.CFG"
#define S64_PUBLISH_SETTINGS_RELAY_CAPACITY 65u

typedef enum {
    S64_PUBLISH_SETTINGS_OK = 0,
    S64_PUBLISH_SETTINGS_INVALID_ARGUMENT,
    S64_PUBLISH_SETTINGS_INVALID_FILE,
    S64_PUBLISH_SETTINGS_IO_ERROR
} S64PublishSettingsResult;

const char* s64_publish_settings_result_message(S64PublishSettingsResult result);

S64PublishSettingsResult s64_publish_settings_load(
    S64Dos* dos,
    S64PublishDestination* destination,
    char* relay_key,
    uint16_t relay_capacity,
    uint8_t* found
);

S64PublishSettingsResult s64_publish_settings_save(
    S64Dos* dos,
    const S64PublishDestination* destination,
    const char* relay_key
);

#endif
