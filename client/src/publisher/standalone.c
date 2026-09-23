#include "standalone.h"
#include "relay.h"

#include <string.h>

#define S64_STANDALONE_LOGIN_RESPONSE_CAPACITY 192u
#define S64_STANDALONE_AUTH_HEADER_CAPACITY (S64_STANDALONE_TOKEN_CAPACITY + 24u)

typedef struct {
    char* data;
    uint16_t capacity;
    uint16_t length;
    uint8_t overflow;
} S64StandaloneTextSink;

static char response_header[S64_HTTP_RESPONSE_HEADER_CAPACITY];
static char login_response[S64_STANDALONE_LOGIN_RESPONSE_CAPACITY];
static char authorization_header[S64_STANDALONE_AUTH_HEADER_CAPACITY];

static uint8_t s64_standalone_collect_text(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64StandaloneTextSink* sink = (S64StandaloneTextSink*)context;
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

S64StandaloneResult s64_standalone_map_http(S64HttpResult result)
{
    switch (result) {
        case S64_HTTP_OK:
            return S64_STANDALONE_OK;
        case S64_HTTP_INVALID_ARGUMENT:
            return S64_STANDALONE_INVALID_ARGUMENT;
        case S64_HTTP_TARGET_ERROR:
            return S64_STANDALONE_PROTOCOL_ERROR;
        case S64_HTTP_TRANSPORT_ERROR:
        default:
            return S64_STANDALONE_TRANSPORT_ERROR;
    }
}

static uint16_t s64_standalone_http_status(const char* header)
{
    const char* cursor = header;
    uint16_t value;

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

    while (*cursor != '\0' && *cursor != ' ') {
        ++cursor;
    }
    while (*cursor == ' ') {
        ++cursor;
    }

    if (cursor[0] < '0' || cursor[0] > '9'
        || cursor[1] < '0' || cursor[1] > '9'
        || cursor[2] < '0' || cursor[2] > '9') {
        return 0u;
    }

    value = (uint16_t)(
        (uint16_t)(cursor[0] - '0') * 100u
        + (uint16_t)(cursor[1] - '0') * 10u
        + (uint16_t)(cursor[2] - '0')
    );
    return value;
}

static const char* s64_standalone_find(const char* text, const char* needle)
{
    size_t needle_length = strlen(needle);

    if (needle_length == 0u) {
        return text;
    }

    while (*text != '\0') {
        if (strncmp(text, needle, needle_length) == 0) {
            return text + needle_length;
        }
        ++text;
    }

    return 0;
}

static uint8_t s64_standalone_parse_token(
    const char* json,
    char* token,
    uint16_t capacity
)
{
    const char* value = s64_standalone_find(json, "\"token\":\"");
    uint16_t length = 0u;

    if (value == 0 || capacity == 0u) {
        return 0u;
    }

    while (*value != '\0' && *value != '"') {
        if (*value == '\\' || length + 1u >= capacity) {
            return 0u;
        }
        token[length++] = *value++;
    }

    if (*value != '"' || length == 0u) {
        return 0u;
    }

    token[length] = '\0';
    return 1u;
}

static uint8_t s64_standalone_parse_expiry(const char* json, uint32_t* expires)
{
    const char* value = s64_standalone_find(json, "\"expires\":");
    uint32_t result = 0u;
    uint8_t digits = 0u;
    uint8_t digit;

    if (value == 0 || expires == 0) {
        return 0u;
    }

    while (*value >= '0' && *value <= '9') {
        digit = (uint8_t)(*value - '0');
        if (result > 429496729u
            || (result == 429496729u && digit > 5u)) {
            return 0u;
        }
        result = result * 10u + digit;
        ++digits;
        ++value;
    }

    if (digits == 0u) {
        return 0u;
    }

    *expires = result;
    return 1u;
}

S64StandaloneResult s64_standalone_validate_transport(
    const char* url,
    uint8_t allow_insecure_http
)
{
    if (url == 0 || url[0] == '\0') {
        return S64_STANDALONE_INVALID_ARGUMENT;
    }
    if (strncmp(url, "https://", 8u) == 0) {
        return S64_STANDALONE_TLS_UNSUPPORTED;
    }
    return allow_insecure_http
        ? S64_STANDALONE_OK
        : S64_STANDALONE_INSECURE_TRANSPORT;
}

static S64StandaloneResult s64_standalone_add_relay_header(
    S64HttpTransport* http,
    uint8_t header_handle
)
{
    return s64_standalone_map_http(s64_relay_add_header(http, header_handle));
}

S64StandaloneResult s64_standalone_add_authorization(
    S64HttpTransport* http,
    uint8_t header_handle,
    const S64StandaloneSession* session
)
{
    static const char prefix[] = "Authorization: Bearer ";
    uint16_t length = 0u;
    uint16_t prefix_length = (uint16_t)(sizeof(prefix) - 1u);
    S64HttpResult result;

    S64StandaloneResult relay_result;

    if (http == 0 || session == 0) {
        return S64_STANDALONE_INVALID_ARGUMENT;
    }

    relay_result = s64_standalone_add_relay_header(http, header_handle);
    if (relay_result != S64_STANDALONE_OK) {
        return relay_result;
    }

    while (length < S64_STANDALONE_TOKEN_CAPACITY
        && session->token[length] != '\0') {
        ++length;
    }

    if (length == 0u || length >= S64_STANDALONE_TOKEN_CAPACITY
        || prefix_length + length + 1u > sizeof(authorization_header)) {
        return S64_STANDALONE_INVALID_ARGUMENT;
    }

    memcpy(authorization_header, prefix, prefix_length);
    memcpy(authorization_header + prefix_length, session->token, length + 1u);

    result = s64_http_header_add(http, header_handle, authorization_header);
    return s64_standalone_map_http(result);
}

S64StandaloneResult s64_standalone_exchange_ack(
    S64HttpTransport* http,
    uint8_t header_handle,
    uint8_t body_handle
)
{
    S64StandaloneTextSink sink;
    S64HttpResult http_result;
    uint16_t status_code;

    if (http == 0) {
        return S64_STANDALONE_INVALID_ARGUMENT;
    }

    sink.data = login_response;
    sink.capacity = (uint16_t)sizeof(login_response);
    sink.length = 0u;
    sink.overflow = 0u;
    login_response[0] = '\0';
    response_header[0] = '\0';

    http_result = s64_http_exchange_raw(
        http,
        header_handle,
        body_handle,
        s64_standalone_collect_text,
        &sink,
        response_header,
        (uint16_t)sizeof(response_header),
        0
    );
    if (http_result != S64_HTTP_OK) {
        return s64_standalone_map_http(http_result);
    }
    if (sink.overflow) {
        return S64_STANDALONE_RESPONSE_TOO_LARGE;
    }

    status_code = s64_standalone_http_status(response_header);
    if (status_code == 401u) {
        return S64_STANDALONE_AUTH_FAILED;
    }
    if (status_code < 200u || status_code >= 300u) {
        return status_code == 0u
            ? S64_STANDALONE_PROTOCOL_ERROR
            : S64_STANDALONE_SERVER_ERROR;
    }

    return s64_standalone_find(login_response, "\"ok\":true") != 0
        ? S64_STANDALONE_OK
        : S64_STANDALONE_PROTOCOL_ERROR;
}

const char* s64_standalone_result_message(S64StandaloneResult result)
{
    switch (result) {
        case S64_STANDALONE_OK:
            return "OK";
        case S64_STANDALONE_INVALID_ARGUMENT:
            return "Invalid request";
        case S64_STANDALONE_TRANSPORT_ERROR:
            return "Network/UCI error";
        case S64_STANDALONE_AUTH_FAILED:
            return "Login expired or denied";
        case S64_STANDALONE_INSECURE_TRANSPORT:
            return "Plain HTTP not enabled";
        case S64_STANDALONE_TLS_UNSUPPORTED:
            return "C64U TLS unavailable";
        case S64_STANDALONE_SERVER_ERROR:
            return "Server rejected request";
        case S64_STANDALONE_RESPONSE_TOO_LARGE:
            return "Server reply too large";
        case S64_STANDALONE_PROTOCOL_ERROR:
        default:
            return "Unexpected server reply";
    }
}

void s64_standalone_session_clear(S64StandaloneSession* session)
{
    if (session == 0) {
        return;
    }

    session->token[0] = '\0';
    session->expires = 0u;
}

S64StandaloneResult s64_standalone_login(
    S64HttpTransport* http,
    const char* login_url,
    const char* password,
    uint8_t allow_insecure_http,
    S64StandaloneSession* session
)
{
    S64HttpResult http_result;
    S64StandaloneResult result = S64_STANDALONE_PROTOCOL_ERROR;
    S64StandaloneTextSink sink;
    uint8_t header_handle = 0u;
    uint8_t body_handle = 0u;
    uint8_t header_created = 0u;
    uint8_t body_created = 0u;
    uint16_t status_code;
    uint16_t response_header_length = 0u;

    if (http == 0 || login_url == 0 || login_url[0] == '\0'
        || password == 0 || session == 0) {
        return S64_STANDALONE_INVALID_ARGUMENT;
    }

    s64_standalone_session_clear(session);

    result = s64_standalone_validate_transport(login_url, allow_insecure_http);
    if (result != S64_STANDALONE_OK) {
        return result;
    }

    http_result = s64_http_free_all(http);
    if (http_result != S64_HTTP_OK) {
        return s64_standalone_map_http(http_result);
    }

    http_result = s64_http_header_create(http, S64_HTTP_POST, login_url, &header_handle);
    if (http_result != S64_HTTP_OK) {
        return s64_standalone_map_http(http_result);
    }
    header_created = 1u;

    result = s64_standalone_add_relay_header(http, header_handle);
    if (result != S64_STANDALONE_OK) {
        goto cleanup;
    }

    http_result = s64_http_body_create(http, S64_HTTP_BODY_URL_ENCODED, &body_handle);
    if (http_result != S64_HTTP_OK) {
        result = s64_standalone_map_http(http_result);
        goto cleanup;
    }
    body_created = 1u;

    http_result = s64_http_body_add_string(http, body_handle, "password", password);
    if (http_result != S64_HTTP_OK) {
        result = s64_standalone_map_http(http_result);
        goto cleanup;
    }

    sink.data = login_response;
    sink.capacity = (uint16_t)sizeof(login_response);
    sink.length = 0u;
    sink.overflow = 0u;
    login_response[0] = '\0';
    response_header[0] = '\0';

    http_result = s64_http_exchange_raw(
        http,
        header_handle,
        body_handle,
        s64_standalone_collect_text,
        &sink,
        response_header,
        (uint16_t)sizeof(response_header),
        &response_header_length
    );
    if (http_result != S64_HTTP_OK) {
        result = s64_standalone_map_http(http_result);
        goto cleanup;
    }
    if (sink.overflow) {
        result = S64_STANDALONE_RESPONSE_TOO_LARGE;
        goto cleanup;
    }

    status_code = s64_standalone_http_status(response_header);
    if (status_code == 401u) {
        result = S64_STANDALONE_AUTH_FAILED;
        goto cleanup;
    }
    if (status_code < 200u || status_code >= 300u) {
        result = status_code == 0u
            ? S64_STANDALONE_PROTOCOL_ERROR
            : S64_STANDALONE_SERVER_ERROR;
        goto cleanup;
    }

    if (!s64_standalone_parse_token(
            login_response,
            session->token,
            (uint16_t)sizeof(session->token)
        )
        || !s64_standalone_parse_expiry(login_response, &session->expires)) {
        s64_standalone_session_clear(session);
        result = S64_STANDALONE_PROTOCOL_ERROR;
        goto cleanup;
    }

    result = S64_STANDALONE_OK;

cleanup:
    if (body_created) {
        (void)s64_http_body_free(http, body_handle);
    }
    if (header_created) {
        (void)s64_http_header_free(http, header_handle);
    }

    if (result != S64_STANDALONE_OK) {
        s64_standalone_session_clear(session);
    }
    return result;
}
