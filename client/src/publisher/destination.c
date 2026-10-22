#include "destination.h"

#ifdef S64_PERSISTENT_RUNTIME_FIT
#include "publisher/persistent_services.h"
#define s64_document_slug_valid s64_persistent_document_slug_valid
#endif

#include <string.h>

static uint8_t type_valid(S64DestinationType type)
{
    return type == S64_DESTINATION_STANDALONE
        || type == S64_DESTINATION_WORDPRESS;
}

#ifndef S64_SETTINGS_CONFIG_ONLY
static uint8_t id_digits(const char* value)
{
    uint16_t i = 0u;

    if (value == 0 || value[0] == '\0') return 0u;
    while (value[i] != '\0') {
        if (i >= S64_WORDPRESS_REMOTE_ID_MAX
            || value[i] < '0' || value[i] > '9') {
            return 0u;
        }
        ++i;
    }
    return 1u;
}

#endif

static uint8_t has_forbidden_url_character(char value)
{
    return value == '\r' || value == '\n'
        || value == '\t' || value == ' '
        || value == '?' || value == '#';
}

static S64DestinationResult copy_base_url(
    char* destination,
    const char* source
)
{
    uint16_t length = 0u;

    if (source == 0 || source[0] == '\0') {
        return S64_DESTINATION_INVALID_BASE_URL;
    }

    while (source[length] != '\0') {
        if (length + 1u >= S64_DESTINATION_BASE_URL_CAPACITY
            || has_forbidden_url_character(source[length])) {
            return S64_DESTINATION_INVALID_BASE_URL;
        }
        ++length;
    }

    while (length > 0u && source[length - 1u] == '/') {
        --length;
    }
    if (length == 0u) {
        return S64_DESTINATION_INVALID_BASE_URL;
    }

    memcpy(destination, source, length);
    destination[length] = '\0';
    return S64_DESTINATION_OK;
}

#ifndef S64_SETTINGS_CONFIG_ONLY
static S64DestinationResult append_text(
    char* output,
    uint16_t capacity,
    uint16_t* length,
    const char* text
)
{
    uint16_t i = 0u;

    if (output == 0 || length == 0 || text == 0 || capacity == 0u) {
        return S64_DESTINATION_INVALID_ARGUMENT;
    }

    while (text[i] != '\0') {
        if (*length + 1u >= capacity) {
            return S64_DESTINATION_URL_TOO_LONG;
        }
        output[*length] = text[i];
        ++(*length);
        ++i;
    }
    output[*length] = '\0';
    return S64_DESTINATION_OK;
}

static S64DestinationResult build(
    const S64PublishDestination* destination,
    const char* suffix,
    char* output,
    uint16_t capacity
)
{
    uint16_t length = 0u;
    S64DestinationResult result;

    if (destination == 0 || output == 0 || capacity == 0u
        || !type_valid(destination->type) || destination->base_url[0] == '\0') {
        return S64_DESTINATION_INVALID_ARGUMENT;
    }

    output[0] = '\0';
    result = append_text(output, capacity, &length, destination->base_url);
    if (result != S64_DESTINATION_OK) return result;
    return append_text(output, capacity, &length, suffix);
}

static const char* document_type_name(S64DocumentType type)
{
    if (type == S64_DOCUMENT_PAGE) return "page";
    if (type == S64_DOCUMENT_POST) return "post";
    return 0;
}

#endif

void s64_destination_clear(S64PublishDestination* destination)
{
    uint16_t i;

    if (destination == 0) return;

    destination->type = S64_DESTINATION_NONE;
    destination->allow_insecure_http = 0u;
    for (i = 0u; i < S64_DESTINATION_BASE_URL_CAPACITY; ++i) {
        destination->base_url[i] = '\0';
    }
}

S64DestinationResult s64_destination_configure(
    S64PublishDestination* destination,
    S64DestinationType type,
    const char* base_url,
    uint8_t allow_insecure_http
)
{
    S64DestinationResult result;

    if (destination == 0 || !type_valid(type)) {
        return S64_DESTINATION_INVALID_ARGUMENT;
    }

    s64_destination_clear(destination);
    result = copy_base_url(destination->base_url, base_url);
    if (result != S64_DESTINATION_OK) {
        return result;
    }

    destination->type = type;
    destination->allow_insecure_http = allow_insecure_http ? 1u : 0u;
    return S64_DESTINATION_OK;
}

const char* s64_destination_result_message(S64DestinationResult result)
{
    switch (result) {
        case S64_DESTINATION_OK:
            return "OK";
        case S64_DESTINATION_INVALID_ARGUMENT:
            return "Invalid destination";
        case S64_DESTINATION_INVALID_BASE_URL:
            return "Invalid site URL";
        case S64_DESTINATION_URL_TOO_LONG:
            return "Site URL too long";
        case S64_DESTINATION_INVALID_ID:
        default:
            return "Invalid document identity";
    }
}

#ifndef S64_SETTINGS_CONFIG_ONLY
S64DestinationResult s64_destination_connection_url(
    const S64PublishDestination* destination,
    char* output,
    uint16_t capacity
)
{
    if (destination == 0) {
        return S64_DESTINATION_INVALID_ARGUMENT;
    }

    if (destination->type == S64_DESTINATION_STANDALONE) {
        return build(destination, "/api.php?action=login", output, capacity);
    }
    if (destination->type == S64_DESTINATION_WORDPRESS) {
        return build(
            destination,
            "/wp-json/see64you/v1/connection",
            output,
            capacity
        );
    }
    return S64_DESTINATION_INVALID_ARGUMENT;
}

S64DestinationResult s64_destination_list_url(
    const S64PublishDestination* destination,
    S64DocumentType type,
    char* output,
    uint16_t capacity
)
{
    const char* type_name = document_type_name(type);
    uint16_t length;
    S64DestinationResult result;

    if (destination == 0 || type_name == 0) {
        return S64_DESTINATION_INVALID_ARGUMENT;
    }

    if (destination->type == S64_DESTINATION_STANDALONE) {
        result = build(destination, "/api.php?action=list&type=", output, capacity);
    } else if (destination->type == S64_DESTINATION_WORDPRESS) {
        result = build(
            destination,
            "/wp-json/see64you/v1/documents?type=",
            output,
            capacity
        );
    } else {
        return S64_DESTINATION_INVALID_ARGUMENT;
    }
    if (result != S64_DESTINATION_OK) return result;

    length = (uint16_t)strlen(output);
    result = append_text(output, capacity, &length, type_name);
    if (result != S64_DESTINATION_OK) return result;

    if (destination->type == S64_DESTINATION_WORDPRESS) {
        return append_text(output, capacity, &length, "&managed=1");
    }
    return S64_DESTINATION_OK;
}

S64DestinationResult s64_destination_open_url(
    const S64PublishDestination* destination,
    S64DocumentType type,
    const char* identity,
    char* output,
    uint16_t capacity
)
{
    const char* type_name = document_type_name(type);
    uint16_t length;
    S64DestinationResult result;

    if (destination == 0 || identity == 0 || type_name == 0) {
        return S64_DESTINATION_INVALID_ARGUMENT;
    }

    if (destination->type == S64_DESTINATION_STANDALONE) {
        if (!s64_document_slug_valid(identity)) {
            return S64_DESTINATION_INVALID_ID;
        }

        result = build(destination, "/api.php?action=get&type=", output, capacity);
        if (result != S64_DESTINATION_OK) return result;
        length = (uint16_t)strlen(output);

        result = append_text(output, capacity, &length, type_name);
        if (result != S64_DESTINATION_OK) return result;
        result = append_text(output, capacity, &length, "&slug=");
        if (result != S64_DESTINATION_OK) return result;
        result = append_text(output, capacity, &length, identity);
        if (result != S64_DESTINATION_OK) return result;
#ifdef S64_INDEPENDENT_PUBLISHER
        return append_text(output, capacity, &length, "&format=8tml");
#else
        return append_text(output, capacity, &length, "&format=s64");
#endif
    }

    if (destination->type == S64_DESTINATION_WORDPRESS) {
        if (!id_digits(identity)) {
            return S64_DESTINATION_INVALID_ID;
        }

        result = build(
            destination,
            "/wp-json/see64you/v1/documents/",
            output,
            capacity
        );
        if (result != S64_DESTINATION_OK) return result;
        length = (uint16_t)strlen(output);
        result = append_text(output, capacity, &length, identity);
        if (result != S64_DESTINATION_OK) return result;
        return append_text(output, capacity, &length, "?chunks=1");
    }

    return S64_DESTINATION_INVALID_ARGUMENT;
}

S64DestinationResult s64_destination_save_url(
    const S64PublishDestination* destination,
    const char* identity,
    uint8_t updating,
    char* output,
    uint16_t capacity
)
{
    uint16_t length;
    S64DestinationResult result;

    if (destination == 0) {
        return S64_DESTINATION_INVALID_ARGUMENT;
    }

    if (destination->type == S64_DESTINATION_STANDALONE) {
        if (updating && (identity == 0 || !s64_document_slug_valid(identity))) {
            return S64_DESTINATION_INVALID_ID;
        }
#ifdef S64_INDEPENDENT_PUBLISHER
        result = build(destination, updating
            ? "/api.php?action=save&ack=1&intent=update&identity="
            : "/api.php?action=save&ack=1&intent=create", output, capacity);
        if (result != S64_DESTINATION_OK || !updating) return result;
        length = (uint16_t)strlen(output);
        return append_text(output, capacity, &length, identity);
#else
        return build(destination, "/api.php?action=save&ack=1", output, capacity);
#endif
    }

    if (destination->type == S64_DESTINATION_WORDPRESS) {
        if (!updating) {
            return build(
                destination,
                "/wp-json/see64you/v1/documents?ack=1",
                output,
                capacity
            );
        }
        if (!id_digits(identity)) {
            return S64_DESTINATION_INVALID_ID;
        }

        result = build(
            destination,
            "/wp-json/see64you/v1/documents/",
            output,
            capacity
        );
        if (result != S64_DESTINATION_OK) return result;
        length = (uint16_t)strlen(output);
        result = append_text(output, capacity, &length, identity);
        if (result != S64_DESTINATION_OK) return result;
        return append_text(output, capacity, &length, "?ack=1");
    }

    return S64_DESTINATION_INVALID_ARGUMENT;
}

S64DestinationResult s64_destination_delete_url(
    const S64PublishDestination* destination,
    S64DocumentType type,
    const char* identity,
    char* output,
    uint16_t capacity
)
{
    uint16_t length;
    S64DestinationResult result;

    if (destination == 0 || document_type_name(type) == 0) {
        return S64_DESTINATION_INVALID_ARGUMENT;
    }

    if (destination->type == S64_DESTINATION_STANDALONE) {
        if (!s64_document_slug_valid(identity)) {
            return S64_DESTINATION_INVALID_ID;
        }
        return build(destination, "/api.php?action=delete", output, capacity);
    }

    if (destination->type == S64_DESTINATION_WORDPRESS) {
        if (!id_digits(identity)) {
            return S64_DESTINATION_INVALID_ID;
        }
        result = build(
            destination,
            "/wp-json/see64you/v1/documents/",
            output,
            capacity
        );
        if (result != S64_DESTINATION_OK) return result;
        length = (uint16_t)strlen(output);
        return append_text(output, capacity, &length, identity);
    }

    return S64_DESTINATION_INVALID_ARGUMENT;
}

#endif
