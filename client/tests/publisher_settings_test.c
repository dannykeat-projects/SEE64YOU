#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/dos.h"
#include "publisher/destination.h"
#include "publisher/settings.h"

static char stored[512];
static char temporary[512];
static uint16_t stored_length;
static uint16_t temporary_length;
static uint16_t read_offset;
static uint8_t read_open;
static uint8_t write_open;
static uint8_t fail_write;

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void reset_fake(void)
{
    stored[0] = '\0';
    temporary[0] = '\0';
    stored_length = 0u;
    temporary_length = 0u;
    read_offset = 0u;
    read_open = 0u;
    write_open = 0u;
    fail_write = 0u;
}

S64DosResult s64_dos_file_exists(S64Dos* dos, const char* filename, uint8_t* exists)
{
    (void)dos;
    if (exists == 0) return S64_DOS_INVALID_ARGUMENT;
    if (strcmp(filename, S64_PUBLISH_SETTINGS_FILENAME) == 0) {
        *exists = stored_length != 0u;
        return S64_DOS_OK;
    }
    if (strcmp(filename, "SEE64YOU.CFG.TMP") == 0) {
        *exists = temporary_length != 0u;
        return S64_DOS_OK;
    }
    *exists = 0u;
    return S64_DOS_OK;
}

S64DosResult s64_dos_open_read(S64Dos* dos, const char* filename)
{
    (void)dos;
    if (strcmp(filename, S64_PUBLISH_SETTINGS_FILENAME) != 0 || stored_length == 0u) {
        return S64_DOS_NOT_FOUND;
    }
    read_offset = 0u;
    read_open = 1u;
    return S64_DOS_OK;
}

S64DosResult s64_dos_open_write(S64Dos* dos, const char* filename)
{
    (void)dos;
    if (strcmp(filename, "SEE64YOU.CFG.TMP") != 0) return S64_DOS_INVALID_ARGUMENT;
    temporary_length = 0u;
    temporary[0] = '\0';
    write_open = 1u;
    return S64_DOS_OK;
}

S64DosResult s64_dos_read(
    S64Dos* dos,
    uint16_t requested,
    S64UciDataSink sink,
    void* context,
    uint16_t* bytes_read
)
{
    uint16_t remaining;
    uint16_t count;

    (void)dos;
    if (bytes_read != 0) *bytes_read = 0u;
    if (!read_open || sink == 0) return S64_DOS_NO_FILE_OPEN;

    remaining = (uint16_t)(stored_length - read_offset);
    count = remaining < requested ? remaining : requested;
    if (count != 0u && !sink((const uint8_t*)stored + read_offset, count, context)) {
        return S64_DOS_CONSUMER_ABORTED;
    }
    read_offset = (uint16_t)(read_offset + count);
    if (bytes_read != 0) *bytes_read = count;
    return S64_DOS_OK;
}

S64DosResult s64_dos_write(S64Dos* dos, const uint8_t* data, uint16_t length)
{
    (void)dos;
    if (!write_open) return S64_DOS_NO_FILE_OPEN;
    if (fail_write) return S64_DOS_TRANSPORT_ERROR;
    if ((uint32_t)temporary_length + length + 1u > sizeof(temporary)) {
        return S64_DOS_TARGET_ERROR;
    }
    memcpy(temporary + temporary_length, data, length);
    temporary_length = (uint16_t)(temporary_length + length);
    temporary[temporary_length] = '\0';
    return S64_DOS_OK;
}

S64DosResult s64_dos_close(S64Dos* dos)
{
    (void)dos;
    read_open = 0u;
    write_open = 0u;
    return S64_DOS_OK;
}

S64DosResult s64_dos_delete_file(S64Dos* dos, const char* filename)
{
    (void)dos;
    if (strcmp(filename, S64_PUBLISH_SETTINGS_FILENAME) == 0) {
        stored_length = 0u;
        stored[0] = '\0';
        return S64_DOS_OK;
    }
    if (strcmp(filename, "SEE64YOU.CFG.TMP") == 0) {
        temporary_length = 0u;
        temporary[0] = '\0';
        return S64_DOS_OK;
    }
    return S64_DOS_NOT_FOUND;
}

S64DosResult s64_dos_rename_file(S64Dos* dos, const char* filename, const char* new_name)
{
    (void)dos;
    if (strcmp(filename, "SEE64YOU.CFG.TMP") != 0
        || strcmp(new_name, S64_PUBLISH_SETTINGS_FILENAME) != 0
        || temporary_length == 0u) {
        return S64_DOS_INVALID_ARGUMENT;
    }
    memcpy(stored, temporary, temporary_length + 1u);
    stored_length = temporary_length;
    temporary_length = 0u;
    temporary[0] = '\0';
    return S64_DOS_OK;
}

static void copy_stored(const char* text)
{
    stored_length = (uint16_t)strlen(text);
    require(stored_length + 1u <= sizeof(stored), "fixture fits");
    memcpy(stored, text, stored_length + 1u);
}

int main(void)
{
    S64Dos dos;
    S64PublishDestination destination;
    S64PublishDestination loaded;
    char relay[S64_PUBLISH_SETTINGS_RELAY_CAPACITY];
    uint8_t found;
    S64PublishSettingsResult result;
    const char* key =
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

    memset(&dos, 0, sizeof(dos));
    reset_fake();
    s64_destination_clear(&loaded);

    result = s64_publish_settings_load(
        &dos, &loaded, relay, (uint16_t)sizeof(relay), &found
    );
    require(result == S64_PUBLISH_SETTINGS_OK, "missing settings is okay");
    require(found == 0u, "missing settings not found");
    require(loaded.type == S64_DESTINATION_NONE, "missing settings clear destination");
    require(relay[0] == '\0', "missing settings clear relay");

    require(
        s64_destination_configure(
            &destination,
            S64_DESTINATION_STANDALONE,
            "http://relay.local",
            1u
        ) == S64_DESTINATION_OK,
        "configure standalone"
    );
    result = s64_publish_settings_save(&dos, &destination, key);
    require(result == S64_PUBLISH_SETTINGS_OK, "save standalone settings");
    require(strstr(stored, "Type: standalone\n") != 0, "stored standalone type");
    require(strstr(stored, "RelayKey: 0123456789abcdef") != 0, "stored relay key");

    s64_destination_clear(&loaded);
    relay[0] = '\0';
    found = 0u;
    result = s64_publish_settings_load(
        &dos, &loaded, relay, (uint16_t)sizeof(relay), &found
    );
    require(result == S64_PUBLISH_SETTINGS_OK, "load standalone settings");
    require(found == 1u, "saved settings found");
    require(loaded.type == S64_DESTINATION_STANDALONE, "standalone type round trip");
    require(strcmp(loaded.base_url, "http://relay.local") == 0, "URL round trip");
    require(loaded.allow_insecure_http == 1u, "HTTP opt-in round trip");
    require(strcmp(relay, key) == 0, "relay round trip");

    require(
        s64_destination_configure(
            &destination,
            S64_DESTINATION_WORDPRESS,
            "http://wordpress.local",
            0u
        ) == S64_DESTINATION_OK,
        "configure WordPress"
    );
    result = s64_publish_settings_save(&dos, &destination, "");
    require(result == S64_PUBLISH_SETTINGS_OK, "save WordPress settings");
    s64_destination_clear(&loaded);
    relay[0] = 'x';
    result = s64_publish_settings_load(
        &dos, &loaded, relay, (uint16_t)sizeof(relay), &found
    );
    require(result == S64_PUBLISH_SETTINGS_OK, "load WordPress settings");
    require(loaded.type == S64_DESTINATION_WORDPRESS, "WordPress type round trip");
    require(loaded.allow_insecure_http == 0u, "secure preference round trip");
    require(relay[0] == '\0', "disabled relay round trip");

    copy_stored(
        "SEE64YOU-PUBLISHER/1\n"
        "Type: standalone\n"
        "URL: http://example.local\n"
        "AllowHttp: 2\n"
        "RelayKey: \n"
    );
    result = s64_publish_settings_load(
        &dos, &loaded, relay, (uint16_t)sizeof(relay), &found
    );
    require(result == S64_PUBLISH_SETTINGS_INVALID_FILE, "invalid settings rejected");
    require(loaded.type == S64_DESTINATION_NONE, "invalid settings clear destination");
    require(relay[0] == '\0', "invalid settings clear relay");

    copy_stored("old settings");
    require(
        s64_destination_configure(
            &destination,
            S64_DESTINATION_STANDALONE,
            "http://new.local",
            1u
        ) == S64_DESTINATION_OK,
        "configure failed-write fixture"
    );
    fail_write = 1u;
    result = s64_publish_settings_save(&dos, &destination, "");
    require(result == S64_PUBLISH_SETTINGS_IO_ERROR, "failed write reported");
    require(strcmp(stored, "old settings") == 0, "failed temp write preserves old file");
    require(temporary_length == 0u, "failed temp write cleaned up");

    puts("Publisher settings tests passed");
    return 0;
}
