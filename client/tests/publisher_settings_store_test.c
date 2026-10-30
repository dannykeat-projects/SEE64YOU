#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/reu_layout.h"
#include "platform/package_paths.h"
#include "publisher/settings_store.h"

static unsigned char reu_settings[S64_REU_SETTINGS_CAPACITY];
static unsigned char stored[512];
static unsigned char legacy_stored[512];
static unsigned char profile_one[512];
static unsigned char temporary[512];
static unsigned char direct_key_file[128];
static unsigned char publish_key_file[514];
static uint16_t publish_key_file_length;
static uint16_t stored_length;
static uint16_t legacy_stored_length;
static uint16_t profile_one_length;
static uint16_t temporary_length;
static uint16_t direct_key_file_length;
static const unsigned char* read_source;
static uint16_t read_length;
static uint16_t read_offset;
static uint8_t read_open;
static uint8_t write_open;
static uint8_t fail_write;
static uint8_t bad_final;
static uint8_t bad_temporary;
static uint8_t fail_reopen;
static uint8_t fail_read_close;
static uint8_t fail_temp_reopen;
static uint8_t fail_temp_close;
static uint8_t reu_writes;
static uint8_t create_new_opened;
static uint8_t fail_change_dir;
static uint16_t change_dir_calls;
static uint8_t current_directory;

static S64UciResult mock_execute(
    const S64UciCommandPart* parts,
    uint8_t part_count,
    S64UciReply* reply
)
{
    const char* filename;
    uint16_t i;
    uint16_t length;

    if (parts == 0 || part_count != 2u
        || parts[0].length != 3u
        || parts[0].data[0] != S64_DOS_TARGET_PRIMARY
        || parts[0].data[1] != 0x02u
        || parts[0].data[2] != 0x06u) {
        return S64_UCI_PROTOCOL_ERROR;
    }

    filename = (const char*)parts[1].data;
    length = parts[1].length;
    if (length != strlen("SEE64YOU.CFG.TMP")) return S64_UCI_PROTOCOL_ERROR;
    for (i = 0u; i < length; ++i) {
        if (filename[i] != "SEE64YOU.CFG.TMP"[i]) return S64_UCI_PROTOCOL_ERROR;
    }

    temporary_length = 0u;
    write_open = 1u;
    create_new_opened = 1u;

    if (reply != 0) {
        if (reply->status != 0 && reply->status_capacity >= 6u) {
            memcpy(reply->status, "00,OK", 6u);
        }
        reply->status_length = 5u;
    }
    return S64_UCI_OK;
}

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static uint16_t legacy_checksum_bytes(const unsigned char* bytes, uint16_t length)
{
    uint8_t sum1 = 0x53u;
    uint8_t sum2 = 0x36u;
    uint16_t i;

    for (i = 0u; i < length; ++i) {
        sum1 = (uint8_t)(sum1 + bytes[i]);
        sum2 = (uint8_t)(sum2 + sum1);
    }
    return (uint16_t)(((uint16_t)sum2 << 8) | sum1);
}

static void store_legacy_v1_fixture(void)
{
    S64SettingsExchange legacy;
    uint16_t checksum;
    uint16_t checksum_offset = (uint16_t)offsetof(S64SettingsExchange, direct_key);

    s64_settings_exchange_init(&legacy);
    legacy.version = 1u;
    legacy.state = S64_SETTINGS_STATE_READY;
    legacy.destination_type = (uint8_t)S64_DESTINATION_STANDALONE;
    legacy.allow_insecure_http = 1u;
    strcpy(legacy.base_url, "http://legacy.local:6464");
    strcpy(
        legacy.relay_key,
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
    );

    memcpy(legacy_stored, &legacy, checksum_offset);
    checksum = legacy_checksum_bytes(legacy_stored, checksum_offset);
    legacy_stored[checksum_offset] = (unsigned char)(checksum & 0xffu);
    legacy_stored[checksum_offset + 1u] = (unsigned char)(checksum >> 8);
    legacy_stored_length = (uint16_t)(checksum_offset + 2u);
}

uint8_t s64_reu_store(uint8_t bank, uint16_t address, const void* source, uint16_t length)
{
    if (bank != S64_REU_SETTINGS_BANK
        || address < S64_REU_SETTINGS_BASE
        || (uint32_t)(address - S64_REU_SETTINGS_BASE) + length > sizeof(reu_settings)) {
        return 0u;
    }
    ++reu_writes;
    memcpy(reu_settings + (address - S64_REU_SETTINGS_BASE), source, length);
    return 1u;
}

uint8_t s64_reu_load(uint8_t bank, uint16_t address, void* destination, uint16_t length)
{
    if (bank != S64_REU_SETTINGS_BANK
        || address < S64_REU_SETTINGS_BASE
        || (uint32_t)(address - S64_REU_SETTINGS_BASE) + length > sizeof(reu_settings)) {
        return 0u;
    }
    memcpy(destination, reu_settings + (address - S64_REU_SETTINGS_BASE), length);
    return 1u;
}

S64DosResult s64_dos_change_dir(S64Dos* dos, const char* directory)
{
    (void)dos;
    ++change_dir_calls;
    if (fail_change_dir) return S64_DOS_TARGET_ERROR;
    if (strcmp(directory, S64_PACKAGE_DIRECTORY) == 0) {
        current_directory = 1u;
        return S64_DOS_OK;
    }
    if (strcmp(directory, S64_SD_ROOT_DIRECTORY) == 0) {
        current_directory = 0u;
        return S64_DOS_OK;
    }
    return S64_DOS_INVALID_ARGUMENT;
}

S64DosResult s64_dos_file_exists(S64Dos* dos, const char* filename, uint8_t* exists)
{
    (void)dos;
    if (exists == 0) return S64_DOS_INVALID_ARGUMENT;
    if (strcmp(filename, S64_SETTINGS_FILENAME) == 0) {
        *exists = current_directory
            ? stored_length != 0u
            : legacy_stored_length != 0u;
    } else if (strcmp(filename, "S64DEST1.CFG") == 0) {
        *exists = profile_one_length != 0u;
    } else if (strcmp(filename, "SEE64YOU.CFG.TMP") == 0) {
        *exists = temporary_length != 0u;
    } else if (strcmp(filename, S64_DIRECT_KEY_FILENAME) == 0) {
        *exists = direct_key_file_length != 0u;
    } else {
        *exists = 0u;
    }
    return S64_DOS_OK;
}

S64DosResult s64_dos_open_read(S64Dos* dos, const char* filename)
{
    (void)dos;
    if (fail_temp_reopen && strcmp(filename, "SEE64YOU.CFG.TMP") == 0)
        return S64_DOS_TARGET_ERROR;
    if (fail_reopen && strcmp(filename, "SEE64YOU.CFG.TMP") != 0)
        return S64_DOS_TARGET_ERROR;
    if (strcmp(filename, S64_SETTINGS_FILENAME) == 0
        && current_directory && stored_length != 0u) {
        read_source = stored;
        read_length = stored_length;
    } else if (strcmp(filename, S64_SETTINGS_FILENAME) == 0
        && !current_directory && legacy_stored_length != 0u) {
        read_source = legacy_stored;
        read_length = legacy_stored_length;
    } else if (strcmp(filename, "S64DEST1.CFG") == 0
        && profile_one_length != 0u) {
        read_source = profile_one;
        read_length = profile_one_length;
    } else if (strcmp(filename, "SEE64YOU.CFG.TMP") == 0
        && temporary_length != 0u) {
        read_source = temporary;
        read_length = temporary_length;
    } else if (strcmp(filename, S64_DIRECT_KEY_FILENAME) == 0
        && !current_directory && direct_key_file_length != 0u) {
        read_source = direct_key_file;
        read_length = direct_key_file_length;
    } else if (strcmp(filename, S64_PUBLISH_KEY_FILENAME) == 0
        && current_directory && publish_key_file_length) {
        read_source = publish_key_file;
        read_length = publish_key_file_length;
    } else {
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

    remaining = (uint16_t)(read_length - read_offset);
    count = remaining < requested ? remaining : requested;
    if (count != 0u && !sink(read_source + read_offset, count, context)) {
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
    if ((uint32_t)temporary_length + length > sizeof(temporary)) return S64_DOS_TARGET_ERROR;
    memcpy(temporary + temporary_length, data, length);
    temporary_length = (uint16_t)(temporary_length + length);
    if (bad_temporary == 1u) --temporary_length;
    if (bad_temporary == 2u) temporary[12] ^= 1u;
    if (bad_temporary == 3u) {
        S64SettingsExchange stale;
        memcpy(&stale, temporary, sizeof(stale));
        strcpy(stale.base_url, "http://stale.local");
        require(s64_settings_exchange_prepare(&stale), "seal stale temp fixture");
        memcpy(temporary, &stale, sizeof(stale));
    }
    if (bad_temporary == 4u) temporary[temporary_length++] = 0x55u;
    return S64_DOS_OK;
}

S64DosResult s64_dos_close(S64Dos* dos)
{
    S64DosResult result = read_open
        && (read_source == temporary ? fail_temp_close : fail_read_close)
        ? S64_DOS_TARGET_ERROR : S64_DOS_OK;
    (void)dos;
    read_open = 0u;
    write_open = 0u;
    return result;
}

S64DosResult s64_dos_delete_file(S64Dos* dos, const char* filename)
{
    (void)dos;
    if (strcmp(filename, S64_SETTINGS_FILENAME) == 0) {
        if (current_directory) stored_length = 0u;
        else legacy_stored_length = 0u;
        return S64_DOS_OK;
    }
    if (strcmp(filename, "S64DEST1.CFG") == 0) {
        profile_one_length = 0u;
        return S64_DOS_OK;
    }
    if (strcmp(filename, "SEE64YOU.CFG.TMP") == 0) {
        temporary_length = 0u;
        return S64_DOS_OK;
    }
    return S64_DOS_NOT_FOUND;
}

S64DosResult s64_dos_rename_file(S64Dos* dos, const char* filename, const char* new_name)
{
    (void)dos;
    if (strcmp(filename, "SEE64YOU.CFG.TMP") != 0
        || temporary_length == 0u) {
        return S64_DOS_INVALID_ARGUMENT;
    }
    if (strcmp(new_name, S64_SETTINGS_FILENAME) == 0
        && current_directory) {
        memcpy(stored, temporary, temporary_length);
        stored_length = temporary_length;
        if (bad_final == 1u) stored_length = 0u;
        if (bad_final == 2u) --stored_length;
        if (bad_final == 3u) stored[12] ^= 1u;
        if (bad_final == 4u) {
            S64SettingsExchange stale;
            memcpy(&stale, stored, sizeof(stale));
            strcpy(stale.base_url, "http://stale.local");
            require(s64_settings_exchange_prepare(&stale), "seal stale fixture");
            memcpy(stored, &stale, sizeof(stale));
        }
    } else if (strcmp(new_name, "S64DEST1.CFG") == 0) {
        memcpy(profile_one, temporary, temporary_length);
        profile_one_length = temporary_length;
    } else {
        return S64_DOS_INVALID_ARGUMENT;
    }
    temporary_length = 0u;
    return S64_DOS_OK;
}

int main(void)
{
    S64Dos dos;
    S64SettingsExchange settings;
    S64SettingsExchange loaded;
    S64SettingsStoreResult result;
    unsigned char previous_settings[sizeof(S64SettingsExchange)];
    const char* key =
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

    memset(&dos, 0, sizeof(dos));
    dos.execute = mock_execute;
    dos.target = S64_DOS_TARGET_PRIMARY;
    memset(reu_settings, 0, sizeof(reu_settings));
    stored_length = 0u;
    legacy_stored_length = 0u;
    profile_one_length = 0u;
    temporary_length = 0u;
    direct_key_file_length = 0u;
    read_source = stored;
    read_length = 0u;
    fail_write = 0u;
    create_new_opened = 0u;
    fail_change_dir = 0u;
    change_dir_calls = 0u;
    current_directory = 0u;

    s64_settings_exchange_init(&loaded);
    require(
        s64_settings_store_load(&dos, &loaded) == S64_SETTINGS_STORE_NOT_FOUND,
        "missing settings reported"
    );
    require(change_dir_calls == 2u,
        "missing settings check app folder then legacy root");

    store_legacy_v1_fixture();
    memset(&loaded, 0, sizeof(loaded));
    require(
        s64_settings_store_load(&dos, &loaded) == S64_SETTINGS_STORE_OK,
        "legacy v1 settings migrate"
    );
    require(loaded.version == S64_SETTINGS_VERSION, "legacy settings upgrade version");
    require(strcmp(loaded.base_url, "http://legacy.local:6464") == 0, "legacy URL preserved");
    require(loaded.direct_key[0] == '\0', "legacy settings start without direct key");
    require(stored_length == sizeof(loaded),
        "legacy settings copied to app folder");
    require(legacy_stored_length == 0u,
        "legacy root settings retired after verified migration");

    stored_length = 0u;
    store_legacy_v1_fixture();
    fail_write = 1u;
    memset(&loaded, 0, sizeof(loaded));
    require(
        s64_settings_store_load(&dos, &loaded) == S64_SETTINGS_STORE_IO_ERROR,
        "failed migration is reported"
    );
    require(stored_length == 0u,
        "failed migration does not publish app-folder config");
    require(legacy_stored_length != 0u,
        "failed migration preserves legacy root config");
    fail_write = 0u;
    legacy_stored_length = 0u;

    s64_settings_exchange_init(&settings);
    settings.destination_type = (uint8_t)S64_DESTINATION_STANDALONE;
    settings.allow_insecure_http = 1u;
    strcpy(settings.base_url, "http://relay.local:6464");
    strcpy(settings.relay_key, key);
    strcpy(settings.direct_key,
        "abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789");
    change_dir_calls = 0u;
    result = s64_settings_store_save(&dos, &settings);
    require(result == S64_SETTINGS_STORE_OK, "settings save succeeds");
    require(change_dir_calls == 3u,
        "settings save, temp verification and final readback use app folder");
    require(create_new_opened, "settings save uses create-new temp file");
    require(stored_length == sizeof(settings), "settings file size");

    stored[stored_length++] = 0x55u;
    require(s64_settings_store_load(&dos, &loaded) == S64_SETTINGS_STORE_INVALID,
        "settings with trailing bytes rejected");
    --stored_length;

    memcpy(previous_settings, stored, sizeof(previous_settings));
    for (bad_temporary = 1u; bad_temporary <= 4u; ++bad_temporary) {
        require(s64_settings_store_save(&dos, &settings) == S64_SETTINGS_STORE_VERIFY_ERROR,
            "truncated/corrupt/stale/overlong temp rejected before replacement");
        require(stored_length == sizeof(previous_settings)
            && memcmp(stored, previous_settings, sizeof(previous_settings)) == 0,
            "invalid temp preserves exact previous configuration");
        require(!read_open && !write_open, "temp verification closes handles");
    }
    bad_temporary = 0u;
    fail_temp_reopen = 1u;
    require(s64_settings_store_save(&dos, &settings) == S64_SETTINGS_STORE_VERIFY_ERROR,
        "temp reopen failure blocks replacement");
    fail_temp_reopen = 0u;
    fail_temp_close = 1u;
    require(s64_settings_store_save(&dos, &settings) == S64_SETTINGS_STORE_VERIFY_ERROR,
        "temp read-close failure blocks replacement");
    fail_temp_close = 0u;
    require(stored_length == sizeof(previous_settings)
        && memcmp(stored, previous_settings, sizeof(previous_settings)) == 0,
        "temp read failures preserve exact previous configuration");
    require(!read_open && !write_open, "temp read failures close handles");

    require(reu_writes == 0u, "disk save does not publish REU handoff");
    memset(reu_settings, 0, sizeof(reu_settings)); /* Complete power loss. */
    memset(&loaded, 0, sizeof(loaded));
    result = s64_settings_store_load(&dos, &loaded);
    require(result == S64_SETTINGS_STORE_OK, "settings load succeeds");
    require(loaded.destination_type == (uint8_t)S64_DESTINATION_STANDALONE, "type round trip");
    require(loaded.allow_insecure_http == 1u, "http flag round trip");
    require(strcmp(loaded.base_url, "http://relay.local:6464") == 0, "url round trip");
    require(strcmp(loaded.relay_key, key) == 0, "relay key round trip");
    require(strcmp(
        loaded.direct_key,
        "abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789"
    ) == 0, "direct key round trip");


    memcpy(
        direct_key_file,
        "ABCDEF0123456789abcdef0123456789abcdef0123456789abcdef0123456789\r\n",
        66u
    );
    direct_key_file_length = 66u;
    strcpy(
        loaded.direct_key,
        "1111111111111111111111111111111111111111111111111111111111111111"
    );
    change_dir_calls = 0u;
    result = s64_settings_import_direct_key(&dos, loaded.direct_key);
    require(result == S64_SETTINGS_STORE_OK, "direct key file import succeeds");
    require(change_dir_calls == 2u,
        "direct key import falls back from app folder to SD root");
    require(
        strcmp(
            loaded.direct_key,
            "ABCDEF0123456789abcdef0123456789abcdef0123456789abcdef0123456789"
        ) == 0,
        "direct key file imported exactly"
    );

    memcpy(
        direct_key_file,
        "ABCDEF0123456789abcdef0123456789abcdef0123456789abcdef012345678X",
        64u
    );
    direct_key_file_length = 64u;
    strcpy(
        loaded.direct_key,
        "2222222222222222222222222222222222222222222222222222222222222222"
    );
    result = s64_settings_import_direct_key(&dos, loaded.direct_key);
    require(result == S64_SETTINGS_STORE_INVALID, "invalid direct key file rejected");
    require(
        strcmp(
            loaded.direct_key,
            "2222222222222222222222222222222222222222222222222222222222222222"
        ) == 0,
        "invalid import preserves existing key"
    );

    direct_key_file_length = 0u;
    result = s64_settings_import_direct_key(&dos, loaded.direct_key);
    require(result == S64_SETTINGS_STORE_NOT_FOUND, "missing direct key file reported");

    strcpy(settings.base_url, "http://profile.local:6464");
    change_dir_calls = 0u;
    result = s64_settings_profile_save(&dos, 0u, &settings);
    require(result == S64_SETTINGS_STORE_OK, "profile save succeeds");
    require(change_dir_calls == 3u,
        "profile save, temp verification and final readback use app folder");
    require(profile_one_length == sizeof(settings), "profile file size");
    require(stored_length == sizeof(settings), "profile save preserves active file");

    memset(&loaded, 0, sizeof(loaded));
    result = s64_settings_store_load(&dos, &loaded);
    require(result == S64_SETTINGS_STORE_OK, "active settings still load");
    require(
        strcmp(loaded.base_url, "http://relay.local:6464") == 0,
        "profile save does not replace active URL"
    );

    memset(&loaded, 0, sizeof(loaded));
    change_dir_calls = 0u;
    result = s64_settings_profile_load(&dos, 0u, &loaded);
    require(result == S64_SETTINGS_STORE_OK, "profile load succeeds");
    require(change_dir_calls == 1u, "profile load uses app folder");
    require(
        strcmp(loaded.base_url, "http://profile.local:6464") == 0,
        "profile URL round trip"
    );
    require(
        s64_settings_profile_load(&dos, 1u, &loaded)
            == S64_SETTINGS_STORE_NOT_FOUND,
        "empty profile reported"
    );
    require(
        s64_settings_profile_load(
            &dos,
            S64_SETTINGS_PROFILE_COUNT,
            &loaded
        ) == S64_SETTINGS_STORE_INVALID,
        "out of range profile rejected"
    );

    stored[12] ^= 0x20u;
    require(
        s64_settings_store_load(&dos, &loaded) == S64_SETTINGS_STORE_INVALID,
        "corrupt settings rejected"
    );
    stored[12] ^= 0x20u;

    for (bad_final = 1u; bad_final <= 4u; ++bad_final) {
        require(s64_settings_store_save(&dos, &settings) == S64_SETTINGS_STORE_VERIFY_ERROR,
            "status-success missing/truncated/corrupt/stale final rejected");
        require(reu_writes == 0u, "failed save leaves REU unpublished");
    }
    bad_final = 0u;
    fail_reopen = 1u;
    require(s64_settings_store_save(&dos, &settings) == S64_SETTINGS_STORE_VERIFY_ERROR,
        "final reopen failure rejected");
    fail_reopen = 0u;
    fail_read_close = 1u;
    require(s64_settings_store_save(&dos, &settings) == S64_SETTINGS_STORE_VERIFY_ERROR,
        "readback close failure rejected");
    fail_read_close = 0u;
    require(s64_settings_store_save(&dos, &settings) == S64_SETTINGS_STORE_OK,
        "save recovers after readback failures");

    fail_write = 1u;
    result = s64_settings_store_save(&dos, &settings);
    require(result == S64_SETTINGS_STORE_IO_ERROR, "write failure reported");
    require(stored_length == sizeof(settings), "old settings preserved");
    require(temporary_length == 0u, "failed temp cleaned up");

    fail_write = 0u;
    fail_change_dir = 1u;
    create_new_opened = 0u;
    result = s64_settings_store_save(&dos, &settings);
    require(result == S64_SETTINGS_STORE_IO_ERROR, "SD path failure blocks settings save");
    require(!create_new_opened, "path failure stops before temp create");
    require(stored_length == sizeof(settings), "path failure preserves old settings");
    strcpy(
        loaded.direct_key,
        "3333333333333333333333333333333333333333333333333333333333333333"
    );
    result = s64_settings_import_direct_key(&dos, loaded.direct_key);
    require(result == S64_SETTINGS_STORE_IO_ERROR, "SD path failure blocks key import");
    require(
        strcmp(
            loaded.direct_key,
            "3333333333333333333333333333333333333333333333333333333333333333"
        ) == 0,
        "path failure preserves existing direct key"
    );

    fail_change_dir = 0u;
    require(s64_settings_import_publish_key(&dos, &settings) == S64_SETTINGS_STORE_NOT_FOUND,
        "standard importer never reuses legacy key file");
    strcpy((char*)publish_key_file, "8tml-publish: 1\nurl: http://example.com:8080/publish/\nid: editor-1\nkey: 000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f\n");
    publish_key_file_length = (uint16_t)strlen((char*)publish_key_file);
    require(s64_settings_import_publish_key(&dos, &settings) == S64_SETTINGS_STORE_OK,
        "standard key imports endpoint ID and secret together");
    require(settings.destination_type == S64_DESTINATION_STANDARD
        && strcmp(s64_settings_publish_id(&settings), "editor-1") == 0
        && strcmp(settings.base_url, "http://example.com:8080/publish/") == 0
        && strcmp(settings.direct_key, "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f") == 0
        && settings.allow_insecure_http,
        "standard fields exact and HTTP enabled by default");
    settings.allow_insecure_http = 1u;
    require(s64_settings_store_save(&dos, &settings) == S64_SETTINGS_STORE_OK,
        "standard settings save with existing readback gates");
    require(s64_settings_store_load(&dos, &loaded) == S64_SETTINGS_STORE_OK
        && memcmp(&loaded, &settings, sizeof(settings)) == 0,
        "standard endpoint identity secret and HTTP consent persist exactly");
    require(s64_settings_exchange_values_valid(&loaded), "standard record validates");
    require(s64_settings_publish_store_load(&dos, &loaded) == S64_SETTINGS_STORE_OK,
        "standard loader accepts tagged record without root migration");
    publish_key_file[0] = '!';
    require(s64_settings_import_publish_key(&dos, &settings) == S64_SETTINGS_STORE_INVALID
        && memcmp(&settings, &loaded, sizeof(settings)) == 0,
        "invalid standard key leaves full existing configuration untouched");
    publish_key_file[0] = '8';
    fail_read_close = 1u;
    require(s64_settings_import_publish_key(&dos, &settings) == S64_SETTINGS_STORE_IO_ERROR
        && memcmp(&settings, &loaded, sizeof(settings)) == 0,
        "failed close does not commit new credentials");
    fail_read_close = 0u;
    memset(publish_key_file, 'a', sizeof(publish_key_file));
    publish_key_file_length = sizeof(publish_key_file);
    require(s64_settings_import_publish_key(&dos, &settings) == S64_SETTINGS_STORE_INVALID
        && memcmp(&settings, &loaded, sizeof(settings)) == 0,
        "oversize standard key does not change settings");
    require(s64_settings_publish_url_valid("https://example.com/")
        && s64_settings_publish_url_valid("http://localhost:65535/publish")
        && !s64_settings_publish_url_valid("http://example.com:0/publish")
        && !s64_settings_publish_url_valid("http://example.com:65536/publish")
        && !s64_settings_publish_url_valid("http://user@example.com/publish")
        && !s64_settings_publish_url_valid("http://example.com/publish?x=1")
        && !s64_settings_publish_url_valid("http://example.com/a\x01")
        && !s64_settings_publish_url_valid("http://example.com/publish#fragment"),
        "unsafe endpoint authority port query fragment and control rejected");
    strcpy(loaded.relay_key, "invalid_id");
    require(!s64_settings_exchange_values_valid(&loaded), "invalid persisted standard ID rejected");
    loaded = settings;
    loaded.destination_type = S64_DESTINATION_STANDALONE;
    require(s64_settings_publish_id(&loaded)[0] == 0,
        "legacy record never exposes standard identity");

    puts("publisher settings store tests passed");
    return 0;
}
