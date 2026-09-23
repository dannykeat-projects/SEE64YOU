#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "publisher/relay.h"

#define MOCK_COMMAND_CAPACITY 896u

static unsigned char command[MOCK_COMMAND_CAPACITY];
static unsigned int command_length;
static unsigned int command_count;

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
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
    const char* status = "000 OK";

    for (i = 0u; i < part_count; ++i) {
        require(offset + parts[i].length <= sizeof(command), "mock command capacity");
        for (j = 0u; j < parts[i].length; ++j) {
            command[offset++] = parts[i].data[j];
        }
    }

    command_length = offset;
    ++command_count;

    if (reply != NULL) {
        reply->status_length = (uint16_t)strlen(status);
        if (reply->status != NULL && reply->status_capacity != 0u) {
            size_t copy = strlen(status);
            if (copy >= reply->status_capacity) copy = reply->status_capacity - 1u;
            memcpy(reply->status, status, copy);
            reply->status[copy] = '\0';
        }
    }

    return S64_UCI_OK;
}

int main(void)
{
    static const char valid_key[] =
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    static const char header_prefix[] = "X-See64You-Relay-Key: ";
    S64HttpTransport http;

    s64_http_init(&http, mock_execute);

    command_count = 0u;
    require(s64_relay_set_key(NULL), "clear relay key");
    require(s64_relay_add_header(&http, 4u) == S64_HTTP_OK, "disabled relay is a no-op");
    require(command_count == 0u, "disabled relay sends no command");

    require(!s64_relay_set_key("short"), "short key rejected");
    require(!s64_relay_set_key(
        "gggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg"
    ), "non-hex key rejected");

    require(s64_relay_set_key(valid_key), "valid relay key accepted");
    command_count = 0u;
    require(s64_relay_add_header(&http, 4u) == S64_HTTP_OK, "relay header added");
    require(command_count == 1u, "relay header uses one HTTP command");
    require(command_length == 3u + sizeof(header_prefix) - 1u + 64u, "relay header command length");
    require(command[0] == 0x06u && command[1] == 0x13u, "relay header HTTP opcode");
    require(command[2] == 4u, "relay header handle");
    require(
        memcmp(command + 3u, header_prefix, sizeof(header_prefix) - 1u) == 0,
        "relay header prefix"
    );
    require(
        memcmp(command + 3u + sizeof(header_prefix) - 1u, valid_key, 64u) == 0,
        "relay header key"
    );

    require(!s64_relay_set_key("bad"), "invalid replacement key rejected");
    command_count = 0u;
    require(s64_relay_add_header(&http, 7u) == S64_HTTP_OK, "previous valid key remains active");
    require(command_count == 1u && command[2] == 7u, "invalid update is atomic");

    require(s64_relay_set_key(""), "empty key disables relay");
    command_count = 0u;
    require(s64_relay_add_header(&http, 7u) == S64_HTTP_OK, "empty key no-op");
    require(command_count == 0u, "cleared relay sends nothing");

    puts("relay client tests passed");
    return 0;
}
