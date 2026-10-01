#ifndef SEE64YOU_PUBLISHER_SETTINGS_EXCHANGE_H
#define SEE64YOU_PUBLISHER_SETTINGS_EXCHANGE_H

#include <stdint.h>

#include "publisher/destination.h"
#include "publisher/relay.h"

#ifndef S64_REU_BUFFER
#error "publisher settings exchange requires S64_REU_BUFFER"
#endif

#define S64_SETTINGS_VERSION 2u

#define S64_SETTINGS_STATE_LOAD_REQUEST 1u
#define S64_SETTINGS_STATE_EDIT_REQUEST 2u
#define S64_SETTINGS_STATE_READY 3u

#define S64_SETTINGS_RESULT_OK 0u
#define S64_SETTINGS_RESULT_NOT_FOUND 1u
#define S64_SETTINGS_RESULT_IO_ERROR 2u
#define S64_SETTINGS_RESULT_INVALID_FILE 3u

typedef struct {
    char magic[4];
    uint8_t version;
    uint8_t state;
    uint8_t result;
    uint8_t destination_type;
    uint8_t allow_insecure_http;
    char base_url[S64_DESTINATION_BASE_URL_CAPACITY];
    char relay_key[S64_RELAY_KEY_CAPACITY];
    char direct_key[65];
    uint16_t checksum;
} S64SettingsExchange;

void s64_settings_exchange_init(S64SettingsExchange* settings);
int s64_settings_exchange_values_valid(const S64SettingsExchange* settings);
int s64_settings_exchange_validate(const S64SettingsExchange* settings);
int s64_settings_exchange_load(S64SettingsExchange* settings);
/* Seal a record without publishing it to the volatile REU handoff. */
int s64_settings_exchange_prepare(S64SettingsExchange* settings);
int s64_settings_exchange_store(S64SettingsExchange* settings);

const char* s64_settings_exchange_result_message(uint8_t result);

#endif
