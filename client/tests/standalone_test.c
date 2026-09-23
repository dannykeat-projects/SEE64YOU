#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "publisher/standalone.h"

#define MOCK_MAX_COMMAND 896u

static const char* exchange_status;
static const char* exchange_body;
static unsigned int command_count;
static unsigned char last_command[MOCK_MAX_COMMAND];
static unsigned int last_command_length;

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void mock_reply_status(S64UciReply* reply, const char* status)
{
    size_t length = strlen(status);
    size_t copy = length;

    reply->status_length = (uint16_t)length;
    if (reply->status == NULL || reply->status_capacity == 0u) {
        return;
    }

    if (copy >= reply->status_capacity) {
        copy = reply->status_capacity - 1u;
    }
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
    unsigned char handle;

    require(part_count != 0u, "mock command parts");

    for (i = 0u; i < part_count; ++i) {
        require(offset + parts[i].length <= sizeof(last_command), "mock command capacity");
        for (j = 0u; j < parts[i].length; ++j) {
            last_command[offset++] = parts[i].data[j];
        }
    }

    last_command_length = offset;
    ++command_count;
    require(offset >= 2u && last_command[0] == 0x06u, "HTTP target command");
    opcode = last_command[1];

    if (opcode == 0x11u) {
        handle = 1u;
        mock_reply_status(reply, "000 OK");
        require(reply->data_sink != NULL, "header handle sink");
        require(reply->data_sink(&handle, 1u, reply->data_context), "header handle accepted");
        return S64_UCI_OK;
    }

    if (opcode == 0x21u) {
        handle = 2u;
        mock_reply_status(reply, "000 OK");
        require(reply->data_sink != NULL, "body handle sink");
        require(reply->data_sink(&handle, 1u, reply->data_context), "body handle accepted");
        return S64_UCI_OK;
    }

    if (opcode == 0x25u) {
        mock_reply_status(reply, "000 OK");
        require(last_command_length >= 13u, "password field command length");
        require(last_command[3] == 8u, "password key length");
        require(memcmp(last_command + 4u, "password", 8u) == 0, "password field key");
        return S64_UCI_OK;
    }

    if (opcode == 0x32u) {
        mock_reply_status(reply, exchange_status);
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

    mock_reply_status(reply, "000 OK");
    return S64_UCI_OK;
}

int main(void)
{
    S64HttpTransport http;
    S64StandaloneSession session;
    S64StandaloneResult result;
    const char* token =
        "v1.1790199999.0123456789abcdef."
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    char response[180];

    s64_http_init(&http, mock_execute);

    command_count = 0u;
    exchange_status = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n";
    snprintf(response, sizeof(response), "{\"ok\":true,\"token\":\"%s\",\"expires\":1790199999}", token);
    exchange_body = response;

    result = s64_standalone_login(
        &http,
        "example.test/see64you/api.php?action=login",
        "correct horse battery staple",
        &session
    );
    require(result == S64_STANDALONE_OK, "login succeeds");
    require(strcmp(session.token, token) == 0, "token captured exactly");
    require(session.expires == 1790199999u, "expiry captured");
    require(command_count == 7u, "login creates, exchanges and frees resources");

    command_count = 0u;
    exchange_status = "HTTP/1.1 401 Unauthorized\r\nContent-Type: application/json\r\n";
    exchange_body = "{\"ok\":false,\"error\":\"invalid_credentials\"}";
    result = s64_standalone_login(
        &http,
        "example.test/see64you/api.php?action=login",
        "wrong",
        &session
    );
    require(result == S64_STANDALONE_AUTH_FAILED, "401 maps to auth failure");
    require(session.token[0] == '\0' && session.expires == 0u, "failed login clears session");
    require(command_count == 7u, "failed login still frees resources");

    puts("standalone publisher tests passed");
    return 0;
}
