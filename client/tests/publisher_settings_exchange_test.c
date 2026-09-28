#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/reu_layout.h"
#include "publisher/settings_exchange.h"

static unsigned char reu_settings[S64_REU_SETTINGS_CAPACITY];

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

uint8_t s64_reu_store(uint8_t bank, uint16_t address, const void* source, uint16_t length)
{
    if (bank != S64_REU_SETTINGS_BANK
        || address < S64_REU_SETTINGS_BASE
        || (uint32_t)(address - S64_REU_SETTINGS_BASE) + length > S64_REU_SETTINGS_CAPACITY) return 0u;
    memcpy(reu_settings + (address - S64_REU_SETTINGS_BASE), source, length);
    return 1u;
}

uint8_t s64_reu_load(uint8_t bank, uint16_t address, void* destination, uint16_t length)
{
    if (bank != S64_REU_SETTINGS_BANK
        || address < S64_REU_SETTINGS_BASE
        || (uint32_t)(address - S64_REU_SETTINGS_BASE) + length > S64_REU_SETTINGS_CAPACITY) return 0u;
    memcpy(destination, reu_settings + (address - S64_REU_SETTINGS_BASE), length);
    return 1u;
}

int main(void)
{
    S64SettingsExchange settings;
    S64SettingsExchange loaded;

    memset(reu_settings, 0, sizeof(reu_settings));
    s64_settings_exchange_init(&settings);
    require(s64_settings_exchange_validate(&settings), "default validates");

    settings.state = S64_SETTINGS_STATE_READY;
    settings.destination_type = (uint8_t)S64_DESTINATION_STANDALONE;
    settings.allow_insecure_http = 1u;
    strcpy(settings.base_url, "http://relay.local:6464");
    strcpy(settings.relay_key,
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
    require(s64_settings_exchange_store(&settings), "settings store");
    require(s64_settings_exchange_load(&loaded), "settings load");
    require(loaded.destination_type == (uint8_t)S64_DESTINATION_STANDALONE, "destination round trip");
    require(strcmp(loaded.base_url, "http://relay.local:6464") == 0, "URL round trip");
    require(strcmp(loaded.relay_key, settings.relay_key) == 0, "relay round trip");

    reu_settings[10] ^= 0x40u;
    require(!s64_settings_exchange_load(&loaded), "checksum corruption rejected");

    s64_settings_exchange_init(&settings);
    settings.destination_type = (uint8_t)S64_DESTINATION_STANDALONE;
    strcpy(settings.base_url, "http://relay.local");
    strcpy(settings.relay_key, "bad");
    require(!s64_settings_exchange_store(&settings), "short relay key rejected");

    puts("publisher settings exchange tests passed");
    return 0;
}
