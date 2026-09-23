#include "wordpress.h"

#include "relay.h"

#include <string.h>

#define S64_WORDPRESS_USERNAME_MAX 60u
#define S64_WORDPRESS_APP_PASSWORD_MAX 64u
#define S64_WORDPRESS_APP_PASSWORD_INPUT_MAX 96u
#define S64_WORDPRESS_RESPONSE_CAPACITY 192u

typedef struct {
    const char* username;
    const char* password;
    uint16_t username_index;
    uint16_t password_index;
    uint8_t phase;
} S64CredentialReader;

typedef struct {
    char* data;
    uint16_t capacity;
    uint16_t length;
    uint8_t overflow;
} S64WordPressTextSink;

static char response_header[S64_HTTP_RESPONSE_HEADER_CAPACITY];
static char response_body[S64_WORDPRESS_RESPONSE_CAPACITY];

static const char base64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static uint8_t next_credential_byte(S64CredentialReader* reader, uint8_t* value)
{
    char current;

    if (reader->phase == 0u) {
        current = reader->username[reader->username_index];
        if (current != '\0') {
            *value = (uint8_t)current;
            ++reader->username_index;
            return 1u;
        }
        reader->phase = 1u;
    }

    if (reader->phase == 1u) {
        *value = (uint8_t)':';
        reader->phase = 2u;
        return 1u;
    }

    if (reader->phase == 2u) {
        for (;;) {
            current = reader->password[reader->password_index];
            if (current == '\0') {
                reader->phase = 3u;
                return 0u;
            }
            ++reader->password_index;
            if (current != ' ') {
                *value = (uint8_t)current;
                return 1u;
            }
        }
    }

    return 0u;
}

static uint8_t username_valid(const char* username)
{
    uint16_t length = 0u;
    unsigned char value;

    if (username == 0 || username[0] == '\0') {
        return 0u;
    }

    while (username[length] != '\0') {
        value = (unsigned char)username[length];
        if (length >= S64_WORDPRESS_USERNAME_MAX
            || value < 0x21u || value > 0x7eu || value == ':') {
            return 0u;
        }
        ++length;
    }

    return 1u;
}

static uint8_t application_password_valid(const char* password)
{
    uint16_t input_length = 0u;
    uint16_t compact_length = 0u;
    unsigned char value;

    if (password == 0 || password[0] == '\0') {
        return 0u;
    }

    while (password[input_length] != '\0') {
        value = (unsigned char)password[input_length];
        if (input_length >= S64_WORDPRESS_APP_PASSWORD_INPUT_MAX
            || value < 0x20u || value > 0x7eu) {
            return 0u;
        }
        if (value != ' ') {
            ++compact_length;
            if (compact_length > S64_WORDPRESS_APP_PASSWORD_MAX) {
                return 0u;
            }
        }
        ++input_length;
    }

    return compact_length != 0u;
}

static uint8_t encode_authorization(
    S64WordPressSession* session,
    const char* username,
    const char* password
)
{
    static const char prefix[] = "Authorization: Basic ";
    S64CredentialReader reader;
    uint16_t output = (uint16_t)(sizeof(prefix) - 1u);
    uint8_t bytes[3];
    uint8_t count;

    memcpy(session->authorization, prefix, sizeof(prefix) - 1u);

    reader.username = username;
    reader.password = password;
    reader.username_index = 0u;
    reader.password_index = 0u;
    reader.phase = 0u;

    for (;;) {
        count = 0u;
        while (count < 3u && next_credential_byte(&reader, &bytes[count])) {
            ++count;
        }

        if (count == 0u) {
            break;
        }
        if (output + 4u + 1u > S64_WORDPRESS_AUTH_CAPACITY) {
            return 0u;
        }

        session->authorization[output++] = base64_table[bytes[0] >> 2];
        session->authorization[output++] = base64_table[
            ((bytes[0] & 0x03u) << 4) | (count > 1u ? bytes[1] >> 4 : 0u)
        ];
        session->authorization[output++] = count > 1u
            ? base64_table[
                ((bytes[1] & 0x0fu) << 2) | (count > 2u ? bytes[2] >> 6 : 0u)
            ]
            : '=';
        session->authorization[output++] = count > 2u
            ? base64_table[bytes[2] & 0x3fu]
            : '=';

        if (count < 3u) {
            break;
        }
    }

    session->authorization[output] = '\0';
    return 1u;
}

static uint8_t collect_text(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64WordPressTextSink* sink = (S64WordPressTextSink*)context;
    uint16_t i;

    for (i = 0u; i < length; ++i) {
        if (sink->length + 1u < sink->capacity) {
            sink->data[sink->length++] = (char)data[i];
            sink->data[sink->length] = '\0';
        } else {
            sink->overflow = 1u;
        }
    }

    return 1u;
}

static uint16_t http_status(const char* header)
{
    const char* cursor = header;

    if (header == 0) {
        return 0u;
    }

    if (cursor[0] >= '0' && cursor[0] <= '9'
        && cursor[1] >= '0' && cursor[1] <= '9'
        && cursor[2] >= '0' && cursor[2] <= '9') {
        return (uint16_t)(
            (uint16_t)(cursor[0] - '0') * 100u
            + (uint16_t)(cursor[1] - '0') * 10u
            + (uint16_t)(cursor[2] - '0')
        );
    }

    while (*cursor != '\0' && *cursor != ' ') ++cursor;
    while (*cursor == ' ') ++cursor;

    if (cursor[0] < '0' || cursor[0] > '9'
        || cursor[1] < '0' || cursor[1] > '9'
        || cursor[2] < '0' || cursor[2] > '9') {
        return 0u;
    }

    return (uint16_t)(
        (uint16_t)(cursor[0] - '0') * 100u
        + (uint16_t)(cursor[1] - '0') * 10u
        + (uint16_t)(cursor[2] - '0')
    );
}

static uint8_t response_ok(const char* text)
{
    static const char needle[] = "\"ok\":true";
    size_t needle_length = sizeof(needle) - 1u;

    while (*text != '\0') {
        if (strncmp(text, needle, needle_length) == 0) {
            return 1u;
        }
        ++text;
    }
    return 0u;
}

static S64WordPressResult map_http(S64HttpResult result)
{
    switch (result) {
        case S64_HTTP_OK:
            return S64_WORDPRESS_OK;
        case S64_HTTP_INVALID_ARGUMENT:
            return S64_WORDPRESS_INVALID_ARGUMENT;
        case S64_HTTP_TARGET_ERROR:
            return S64_WORDPRESS_PROTOCOL_ERROR;
        case S64_HTTP_TRANSPORT_ERROR:
        default:
            return S64_WORDPRESS_TRANSPORT_ERROR;
    }
}

static uint8_t ascii_equal_ci(char left, char right)
{
    if (left >= 'A' && left <= 'Z') left = (char)(left + ('a' - 'A'));
    if (right >= 'A' && right <= 'Z') right = (char)(right + ('a' - 'A'));
    return left == right;
}

static uint8_t url_has_https_scheme(const char* url)
{
    static const char scheme[] = "https://";
    uint8_t i;

    if (url == 0) return 0u;
    for (i = 0u; i < (uint8_t)(sizeof(scheme) - 1u); ++i) {
        if (url[i] == '\0' || !ascii_equal_ci(url[i], scheme[i])) {
            return 0u;
        }
    }
    return 1u;
}

static S64WordPressResult validate_transport(
    const char* url,
    uint8_t allow_insecure_http
)
{
    if (url == 0 || url[0] == '\0') {
        return S64_WORDPRESS_INVALID_ARGUMENT;
    }
    if (url_has_https_scheme(url)) {
        return S64_WORDPRESS_TLS_UNSUPPORTED;
    }
    return allow_insecure_http
        ? S64_WORDPRESS_OK
        : S64_WORDPRESS_INSECURE_TRANSPORT;
}

void s64_wordpress_session_clear(S64WordPressSession* session)
{
    uint16_t i;

    if (session == 0) {
        return;
    }

    for (i = 0u; i < S64_WORDPRESS_AUTH_CAPACITY; ++i) {
        session->authorization[i] = '\0';
    }
}

int s64_wordpress_session_configure(
    S64WordPressSession* session,
    const char* username,
    const char* application_password
)
{
    if (session == 0) {
        return 0;
    }

    s64_wordpress_session_clear(session);
    if (!username_valid(username)
        || !application_password_valid(application_password)) {
        return 0;
    }
    if (!encode_authorization(session, username, application_password)) {
        s64_wordpress_session_clear(session);
        return 0;
    }

    return 1;
}

S64WordPressResult s64_wordpress_add_authorization(
    S64HttpTransport* http,
    uint8_t header_handle,
    const S64WordPressSession* session
)
{
    S64HttpResult result;

    if (http == 0 || session == 0 || session->authorization[0] == '\0') {
        return S64_WORDPRESS_INVALID_ARGUMENT;
    }

    result = s64_relay_add_header(http, header_handle);
    if (result != S64_HTTP_OK) {
        return map_http(result);
    }

    result = s64_http_header_add(http, header_handle, session->authorization);
    return map_http(result);
}

const char* s64_wordpress_result_message(S64WordPressResult result)
{
    switch (result) {
        case S64_WORDPRESS_OK:
            return "OK";
        case S64_WORDPRESS_INVALID_ARGUMENT:
            return "Invalid WordPress setup";
        case S64_WORDPRESS_TRANSPORT_ERROR:
            return "Network/UCI error";
        case S64_WORDPRESS_AUTH_FAILED:
            return "WordPress login denied";
        case S64_WORDPRESS_PERMISSION_DENIED:
            return "WordPress edit denied";
        case S64_WORDPRESS_PLUGIN_UNAVAILABLE:
            return "See64You plugin not found";
        case S64_WORDPRESS_INSECURE_TRANSPORT:
            return "Plain HTTP not enabled";
        case S64_WORDPRESS_TLS_UNSUPPORTED:
            return "C64U TLS unavailable";
        case S64_WORDPRESS_SERVER_ERROR:
            return "WordPress server error";
        case S64_WORDPRESS_RESPONSE_TOO_LARGE:
            return "WordPress reply too large";
        case S64_WORDPRESS_PROTOCOL_ERROR:
        default:
            return "Unexpected WordPress reply";
    }
}

S64WordPressResult s64_wordpress_test_connection(
    S64HttpTransport* http,
    const char* connection_url,
    uint8_t allow_insecure_http,
    const S64WordPressSession* session
)
{
    S64HttpResult http_result;
    S64WordPressResult result;
    S64WordPressTextSink sink;
    uint8_t header_handle = 0u;
    uint8_t header_created = 0u;
    uint16_t status_code;

    if (http == 0 || session == 0) {
        return S64_WORDPRESS_INVALID_ARGUMENT;
    }

    result = validate_transport(connection_url, allow_insecure_http);
    if (result != S64_WORDPRESS_OK) {
        return result;
    }

    http_result = s64_http_free_all(http);
    if (http_result != S64_HTTP_OK) {
        return map_http(http_result);
    }

    http_result = s64_http_header_create(
        http,
        S64_HTTP_GET,
        connection_url,
        &header_handle
    );
    if (http_result != S64_HTTP_OK) {
        return map_http(http_result);
    }
    header_created = 1u;

    result = s64_wordpress_add_authorization(http, header_handle, session);
    if (result != S64_WORDPRESS_OK) {
        goto cleanup;
    }

    sink.data = response_body;
    sink.capacity = (uint16_t)sizeof(response_body);
    sink.length = 0u;
    sink.overflow = 0u;
    response_body[0] = '\0';
    response_header[0] = '\0';

    http_result = s64_http_exchange_raw(
        http,
        header_handle,
        S64_HTTP_NO_BODY,
        collect_text,
        &sink,
        response_header,
        (uint16_t)sizeof(response_header),
        0
    );
    if (http_result != S64_HTTP_OK) {
        result = map_http(http_result);
        goto cleanup;
    }
    if (sink.overflow) {
        result = S64_WORDPRESS_RESPONSE_TOO_LARGE;
        goto cleanup;
    }

    status_code = http_status(response_header);
    if (status_code == 401u) {
        result = S64_WORDPRESS_AUTH_FAILED;
    } else if (status_code == 403u) {
        result = S64_WORDPRESS_PERMISSION_DENIED;
    } else if (status_code == 404u) {
        result = S64_WORDPRESS_PLUGIN_UNAVAILABLE;
    } else if (status_code < 200u || status_code >= 300u) {
        result = status_code == 0u
            ? S64_WORDPRESS_PROTOCOL_ERROR
            : S64_WORDPRESS_SERVER_ERROR;
    } else {
        result = response_ok(response_body)
            ? S64_WORDPRESS_OK
            : S64_WORDPRESS_PROTOCOL_ERROR;
    }

cleanup:
    if (header_created) {
        (void)s64_http_header_free(http, header_handle);
    }
    return result;
}
