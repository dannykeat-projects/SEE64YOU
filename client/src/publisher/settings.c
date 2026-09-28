#include "publisher/settings.h"

#include "publisher/relay.h"
#include "publisher/workspace.h"

#include <string.h>

#define S64_SETTINGS_TEMP_FILENAME "SEE64YOU.CFG.TMP"
#define S64_SETTINGS_LINE_CAPACITY 256u

#define SETTINGS_LINE ((char*)&s64_publisher_workspace.bytes[0])

typedef struct {
    S64PublishDestination* destination;
    char* relay_key;
    uint16_t relay_capacity;
    uint16_t line_length;
    uint8_t line_number;
    uint8_t type_seen;
    uint8_t url_seen;
    uint8_t allow_seen;
    uint8_t relay_seen;
    S64DestinationType type;
    uint8_t allow_insecure;
    uint8_t invalid;
} S64SettingsParser;

static uint8_t relay_key_valid(const char* value)
{
    uint16_t i;

    if (value == 0) return 0u;
    if (value[0] == '\0') return 1u;

    for (i = 0u; i < 64u; ++i) {
        char c = value[i];
        if (c == '\0'
            || !((c >= '0' && c <= '9')
                || (c >= 'a' && c <= 'f')
                || (c >= 'A' && c <= 'F'))) {
            return 0u;
        }
    }
    return value[64] == '\0';
}

static uint8_t copy_value(
    char* destination,
    uint16_t capacity,
    const char* source
)
{
    uint16_t length = 0u;

    if (destination == 0 || source == 0 || capacity == 0u) return 0u;
    while (source[length] != '\0') {
        if (length + 1u >= capacity) return 0u;
        destination[length] = source[length];
        ++length;
    }
    destination[length] = '\0';
    return 1u;
}

static uint8_t parse_line(S64SettingsParser* parser)
{
    char* value;

    SETTINGS_LINE[parser->line_length] = '\0';

    if (parser->line_number == 0u) {
        if (strcmp(SETTINGS_LINE, "SEE64YOU-PUBLISHER/1") != 0) {
            return 0u;
        }
        ++parser->line_number;
        return 1u;
    }

    value = strchr(SETTINGS_LINE, ':');
    if (value == 0 || value[1] != ' ') return 0u;
    *value++ = '\0';
    ++value;

    if (strcmp(SETTINGS_LINE, "Type") == 0) {
        if (parser->type_seen) return 0u;
        parser->type_seen = 1u;
        if (strcmp(value, "none") == 0) {
            parser->type = S64_DESTINATION_NONE;
        } else if (strcmp(value, "standalone") == 0) {
            parser->type = S64_DESTINATION_STANDALONE;
        } else if (strcmp(value, "wordpress") == 0) {
            parser->type = S64_DESTINATION_WORDPRESS;
        } else {
            return 0u;
        }
    } else if (strcmp(SETTINGS_LINE, "URL") == 0) {
        if (parser->url_seen
            || !copy_value(
                parser->destination->base_url,
                (uint16_t)sizeof(parser->destination->base_url),
                value
            )) {
            return 0u;
        }
        parser->url_seen = 1u;
    } else if (strcmp(SETTINGS_LINE, "AllowHttp") == 0) {
        if (parser->allow_seen || (strcmp(value, "0") != 0 && strcmp(value, "1") != 0)) {
            return 0u;
        }
        parser->allow_seen = 1u;
        parser->allow_insecure = value[0] == '1' ? 1u : 0u;
    } else if (strcmp(SETTINGS_LINE, "RelayKey") == 0) {
        if (parser->relay_seen
            || !copy_value(parser->relay_key, parser->relay_capacity, value)
            || !relay_key_valid(parser->relay_key)) {
            return 0u;
        }
        parser->relay_seen = 1u;
    } else {
        return 0u;
    }

    ++parser->line_number;
    return 1u;
}

static uint8_t settings_sink(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64SettingsParser* parser = (S64SettingsParser*)context;
    uint16_t i;
    uint8_t value;

    for (i = 0u; i < length; ++i) {
        value = data[i];
        if (value == '\r') continue;
        if (value == '\n') {
            if (!parse_line(parser)) {
                parser->invalid = 1u;
                return 0u;
            }
            parser->line_length = 0u;
            continue;
        }
        if (parser->line_length + 1u >= S64_SETTINGS_LINE_CAPACITY) {
            parser->invalid = 1u;
            return 0u;
        }
        SETTINGS_LINE[parser->line_length++] = (char)value;
    }
    return 1u;
}

static S64PublishSettingsResult finish_parser(S64SettingsParser* parser)
{
    S64DestinationResult result;

    if (parser->invalid) return S64_PUBLISH_SETTINGS_INVALID_FILE;
    if (parser->line_length != 0u) {
        if (!parse_line(parser)) return S64_PUBLISH_SETTINGS_INVALID_FILE;
        parser->line_length = 0u;
    }

    if (!parser->type_seen || !parser->url_seen
        || !parser->allow_seen || !parser->relay_seen) {
        return S64_PUBLISH_SETTINGS_INVALID_FILE;
    }

    if (parser->type == S64_DESTINATION_NONE) {
        if (parser->destination->base_url[0] != '\0') {
            return S64_PUBLISH_SETTINGS_INVALID_FILE;
        }
        s64_destination_clear(parser->destination);
        return S64_PUBLISH_SETTINGS_OK;
    }

    if (!copy_value(
            SETTINGS_LINE,
            S64_SETTINGS_LINE_CAPACITY,
            parser->destination->base_url
        )) {
        return S64_PUBLISH_SETTINGS_INVALID_FILE;
    }

    result = s64_destination_configure(
        parser->destination,
        parser->type,
        SETTINGS_LINE,
        parser->allow_insecure
    );
    return result == S64_DESTINATION_OK
        ? S64_PUBLISH_SETTINGS_OK
        : S64_PUBLISH_SETTINGS_INVALID_FILE;
}

const char* s64_publish_settings_result_message(S64PublishSettingsResult result)
{
    switch (result) {
        case S64_PUBLISH_SETTINGS_OK:
            return "OK";
        case S64_PUBLISH_SETTINGS_INVALID_ARGUMENT:
            return "Invalid settings request";
        case S64_PUBLISH_SETTINGS_INVALID_FILE:
            return "Publisher settings invalid";
        case S64_PUBLISH_SETTINGS_IO_ERROR:
        default:
            return "Publisher settings I/O error";
    }
}

S64PublishSettingsResult s64_publish_settings_load(
    S64Dos* dos,
    S64PublishDestination* destination,
    char* relay_key,
    uint16_t relay_capacity,
    uint8_t* found
)
{
    S64SettingsParser parser;
    S64DosResult result;
    S64DosResult close_result;
    uint16_t bytes_read = 0u;
    uint8_t exists = 0u;
    S64PublishSettingsResult finish_result;

    if (found != 0) *found = 0u;
    if (dos == 0 || destination == 0 || relay_key == 0
        || relay_capacity < S64_PUBLISH_SETTINGS_RELAY_CAPACITY) {
        return S64_PUBLISH_SETTINGS_INVALID_ARGUMENT;
    }

    relay_key[0] = '\0';
    s64_destination_clear(destination);

    result = s64_dos_file_exists(dos, S64_PUBLISH_SETTINGS_FILENAME, &exists);
    if (result != S64_DOS_OK) return S64_PUBLISH_SETTINGS_IO_ERROR;
    if (!exists) return S64_PUBLISH_SETTINGS_OK;
    if (found != 0) *found = 1u;

    result = s64_dos_open_read(dos, S64_PUBLISH_SETTINGS_FILENAME);
    if (result != S64_DOS_OK) return S64_PUBLISH_SETTINGS_IO_ERROR;

    memset(&parser, 0, sizeof(parser));
    parser.destination = destination;
    parser.relay_key = relay_key;
    parser.relay_capacity = relay_capacity;
    parser.type = S64_DESTINATION_NONE;

    do {
        bytes_read = 0u;
        result = s64_dos_read(
            dos,
            256u,
            settings_sink,
            &parser,
            &bytes_read
        );
    } while (result == S64_DOS_OK && bytes_read == 256u);

    close_result = s64_dos_close(dos);
    if (result == S64_DOS_CONSUMER_ABORTED && parser.invalid) {
        result = S64_DOS_OK;
    }
    if (result != S64_DOS_OK || close_result != S64_DOS_OK) {
        s64_destination_clear(destination);
        relay_key[0] = '\0';
        return S64_PUBLISH_SETTINGS_IO_ERROR;
    }

    finish_result = finish_parser(&parser);
    if (finish_result != S64_PUBLISH_SETTINGS_OK) {
        s64_destination_clear(destination);
        relay_key[0] = '\0';
    }
    return finish_result;
}

static S64DosResult write_text(S64Dos* dos, const char* text)
{
    return s64_dos_write(dos, (const uint8_t*)text, (uint16_t)strlen(text));
}

static S64DosResult write_settings_body(
    S64Dos* dos,
    const S64PublishDestination* destination,
    const char* relay_key
)
{
    const char* type_name;

    if (destination->type == S64_DESTINATION_NONE) {
        type_name = "none";
    } else if (destination->type == S64_DESTINATION_STANDALONE) {
        type_name = "standalone";
    } else if (destination->type == S64_DESTINATION_WORDPRESS) {
        type_name = "wordpress";
    } else {
        return S64_DOS_INVALID_ARGUMENT;
    }

    if (write_text(dos, "SEE64YOU-PUBLISHER/1\nType: ") != S64_DOS_OK
        || write_text(dos, type_name) != S64_DOS_OK
        || write_text(dos, "\nURL: ") != S64_DOS_OK
        || write_text(dos, destination->base_url) != S64_DOS_OK
        || write_text(dos, "\nAllowHttp: ") != S64_DOS_OK
        || write_text(dos, destination->allow_insecure_http ? "1" : "0") != S64_DOS_OK
        || write_text(dos, "\nRelayKey: ") != S64_DOS_OK
        || write_text(dos, relay_key) != S64_DOS_OK
        || write_text(dos, "\n") != S64_DOS_OK) {
        return S64_DOS_TRANSPORT_ERROR;
    }
    return S64_DOS_OK;
}

S64PublishSettingsResult s64_publish_settings_save(
    S64Dos* dos,
    const S64PublishDestination* destination,
    const char* relay_key
)
{
    S64DosResult result;
    S64DosResult close_result;
    uint8_t exists = 0u;

    if (dos == 0 || destination == 0 || relay_key == 0
        || !relay_key_valid(relay_key)) {
        return S64_PUBLISH_SETTINGS_INVALID_ARGUMENT;
    }

    result = s64_dos_open_write(dos, S64_SETTINGS_TEMP_FILENAME);
    if (result != S64_DOS_OK) return S64_PUBLISH_SETTINGS_IO_ERROR;

    result = write_settings_body(dos, destination, relay_key);
    close_result = s64_dos_close(dos);
    if (result != S64_DOS_OK || close_result != S64_DOS_OK) {
        (void)s64_dos_delete_file(dos, S64_SETTINGS_TEMP_FILENAME);
        return S64_PUBLISH_SETTINGS_IO_ERROR;
    }

    result = s64_dos_file_exists(dos, S64_PUBLISH_SETTINGS_FILENAME, &exists);
    if (result != S64_DOS_OK) {
        (void)s64_dos_delete_file(dos, S64_SETTINGS_TEMP_FILENAME);
        return S64_PUBLISH_SETTINGS_IO_ERROR;
    }
    if (exists) {
        result = s64_dos_delete_file(dos, S64_PUBLISH_SETTINGS_FILENAME);
        if (result != S64_DOS_OK) {
            (void)s64_dos_delete_file(dos, S64_SETTINGS_TEMP_FILENAME);
            return S64_PUBLISH_SETTINGS_IO_ERROR;
        }
    }

    result = s64_dos_rename_file(
        dos,
        S64_SETTINGS_TEMP_FILENAME,
        S64_PUBLISH_SETTINGS_FILENAME
    );
    if (result != S64_DOS_OK) {
        return S64_PUBLISH_SETTINGS_IO_ERROR;
    }
    return S64_PUBLISH_SETTINGS_OK;
}
