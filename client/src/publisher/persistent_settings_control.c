#include "publisher/persistent_settings_control.h"

#include "publisher/destination.h"

#include <string.h>

S64PersistentSettingsControlResult s64_persistent_settings_validate_values(
    S64SettingsExchange* settings)
{
    S64PublishDestination checked;
    S64DestinationResult result;

    if (!settings) return S64_SETTINGS_CONTROL_INVALID_ARGUMENT;

    if (settings->destination_type == (uint8_t)S64_DESTINATION_NONE) {
        settings->base_url[0] = 0;
        settings->allow_insecure_http = 0u;
        return s64_settings_exchange_values_valid(settings)
            ? S64_SETTINGS_CONTROL_OK
            : S64_SETTINGS_CONTROL_INVALID_VALUES;
    }

    result = s64_destination_configure(
        &checked,
        (S64DestinationType)settings->destination_type,
        settings->base_url,
        settings->allow_insecure_http
    );
    if (result != S64_DESTINATION_OK) {
        return S64_SETTINGS_CONTROL_INVALID_DESTINATION;
    }

    memcpy(settings->base_url, checked.base_url, sizeof(settings->base_url));
    settings->allow_insecure_http = checked.allow_insecure_http;
    return s64_settings_exchange_values_valid(settings)
        ? S64_SETTINGS_CONTROL_OK
        : S64_SETTINGS_CONTROL_INVALID_VALUES;
}
