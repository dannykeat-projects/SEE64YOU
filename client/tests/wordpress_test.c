#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "publisher/relay.h"
#include "publisher/wordpress.h"

#define MOCK_COMMAND_CAPACITY 896u

static unsigned char command[MOCK_COMMAND_CAPACITY];
static unsigned int command_length;
static unsigned int command_count;
static unsigned int relay_headers;
static unsigned int basic_headers;
static const char* exchange_status;
static const char* exchange_body;

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void reply_status(S64UciReply* reply, const char* status)
{
    size_t length = strlen(status);
    size_t copy = length;

    reply->status_length = (uint16_t)length;
    if (reply->status == NULL || reply->status_capacity == 0u) return;
    if (copy >= reply->status_capacity) copy = reply->status_capacity - 1u;
    memcpy(reply->status, status, copy);
    reply->status[copy] = '\0';
}

static S64UciResult mock_execute(
    const S64UciCommandPart* parts,
    uint8_t part_count,
    S64UciReply* reply
)
{
    unsigned int i;
    unsigned int j;
    unsigned int offset = 0u;
    unsigned char opcode;
    unsigned char handle = 1u;

    for (i = 0u; i < part_count; ++i) {
        require(offset + parts[i].length <= sizeof(command), "mock command capacity");
        for (j = 0u; j < parts[i].length; ++j) {
            command[offset++] = parts[i].data[j];
        }
    }

    command_length = offset;
    ++command_count;
    require(command_length >= 2u && command[0] == 0x06u, "HTTP target command");
    opcode = command[1];

    if (opcode == 0x11u) {
        reply_status(reply, "000 OK");
        require(reply->data_sink != NULL, "header handle sink");
        require(reply->data_sink(&handle, 1u, reply->data_context), "header handle accepted");
        return S64_UCI_OK;
    }

    if (opcode == 0x13u) {
        static const char relay_prefix[] = "X-See64You-Relay-Key: ";
        static const char basic_prefix[] = "Authorization: Basic ";

        reply_status(reply, "000 OK");
        if (command_length >= 3u + sizeof(relay_prefix) - 1u
            && memcmp(command + 3u, relay_prefix, sizeof(relay_prefix) - 1u) == 0) {
            ++relay_headers;
        }
        if (command_length >= 3u + sizeof(basic_prefix) - 1u
            && memcmp(command + 3u, basic_prefix, sizeof(basic_prefix) - 1u) == 0) {
            ++basic_headers;
        }
        return S64_UCI_OK;
    }

    if (opcode == 0x32u) {
        reply_status(reply, exchange_status);
        if (exchange_body != NULL && reply->data_sink != NULL) {
            require(
                reply->data_sink(
                    (const uint8_t*)exchange_body,
                    (uint16_t)strlen(exchange_body),
                    reply->data_context
                ),
                "exchange body accepted"
            );
        }
        return S64_UCI_OK;
    }

    reply_status(reply, "000 OK");
    return S64_UCI_OK;
}

static void reset_mock(void)
{
    command_length = 0u;
    command_count = 0u;
    relay_headers = 0u;
    basic_headers = 0u;
}

int main(void)
{
    static const char relay_key[] =
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    S64HttpTransport http;
    S64WordPressSession session;
    S64WordPressResult result;

    s64_http_init(&http, mock_execute);
    s64_wordpress_session_clear(&session);

    require(
        s64_wordpress_session_configure(&session, "editor", "abcd efgh ijkl"),
        "configure Application Password"
    );
    require(
        strcmp(
            session.authorization,
            "Authorization: Basic ZWRpdG9yOmFiY2RlZmdoaWprbA=="
        ) == 0,
        "Basic Authorization encoding"
    );
    require(strstr(session.authorization, "abcd") == NULL, "plain password not retained");

    require(
        !s64_wordpress_session_configure(&session, "bad:name", "abcdefghijkl"),
        "colon in username rejected"
    );
    require(session.authorization[0] == '\0', "invalid username clears old credentials");
    require(
        !s64_wordpress_session_configure(&session, "editor", "    "),
        "empty normalized password rejected"
    );
    require(session.authorization[0] == '\0', "invalid password leaves session clear");

    require(
        s64_wordpress_session_configure(&session, "editor", "abcd efgh ijkl"),
        "restore valid credentials"
    );

    reset_mock();
    result = s64_wordpress_test_connection(
        &http,
        "http://example.test/wp-json/see64you/v1/connection",
        0u,
        &session
    );
    require(result == S64_WORDPRESS_INSECURE_TRANSPORT, "plaintext requires opt in");
    require(command_count == 0u, "blocked plaintext sends nothing");

    result = s64_wordpress_test_connection(
        &http,
        "https://example.test/wp-json/see64you/v1/connection",
        1u,
        &session
    );
    require(result == S64_WORDPRESS_TLS_UNSUPPORTED, "direct https blocked");
    require(command_count == 0u, "blocked https sends nothing");

    result = s64_wordpress_test_connection(
        &http,
        "HTTPS://example.test/wp-json/see64you/v1/connection",
        1u,
        &session
    );
    require(result == S64_WORDPRESS_TLS_UNSUPPORTED, "https scheme check is case insensitive");
    require(command_count == 0u, "uppercase blocked https sends nothing");

    require(s64_relay_set_key(relay_key), "configure relay key");
    reset_mock();
    exchange_status = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n";
    exchange_body = "{\"ok\":true,\"backend\":\"wordpress\"}";
    result = s64_wordpress_test_connection(
        &http,
        "http://relay.local:6464/wp-json/see64you/v1/connection",
        1u,
        &session
    );
    require(result == S64_WORDPRESS_OK, "authenticated connection succeeds");
    require(relay_headers == 1u, "relay key header sent");
    require(basic_headers == 1u, "Basic Authorization header sent");
    require(command_count == 6u, "connection creates, authenticates, exchanges and frees");

    require(s64_relay_set_key(NULL), "disable relay");
    reset_mock();
    exchange_status = "HTTP/1.1 401 Unauthorized\r\nContent-Type: application/json\r\n";
    exchange_body = "{\"code\":\"rest_cannot_access\"}";
    result = s64_wordpress_test_connection(
        &http,
        "http://example.test/wp-json/see64you/v1/connection",
        1u,
        &session
    );
    require(result == S64_WORDPRESS_AUTH_FAILED, "401 maps to auth failure");
    require(command_count == 5u, "direct connection uses no relay command");

    reset_mock();
    exchange_status = "HTTP/1.1 403 Forbidden\r\nContent-Type: application/json\r\n";
    exchange_body = "{\"code\":\"rest_forbidden\"}";
    result = s64_wordpress_test_connection(
        &http,
        "http://example.test/wp-json/see64you/v1/connection",
        1u,
        &session
    );
    require(result == S64_WORDPRESS_PERMISSION_DENIED, "403 maps to permission failure");

    reset_mock();
    exchange_status = "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\n";
    exchange_body = "{\"code\":\"rest_no_route\"}";
    result = s64_wordpress_test_connection(
        &http,
        "http://example.test/wp-json/see64you/v1/connection",
        1u,
        &session
    );
    require(result == S64_WORDPRESS_PLUGIN_UNAVAILABLE, "404 maps to missing plugin");

    s64_wordpress_session_clear(&session);
    require(session.authorization[0] == '\0', "session clear removes authorization");

    puts("wordpress publisher auth tests passed");
    return 0;
}
