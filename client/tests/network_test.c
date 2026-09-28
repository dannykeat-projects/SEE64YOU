#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "platform/network.h"

static uint8_t mode;
static uint8_t calls;
static uint8_t captured_flags;
static uint16_t captured_length;
static uint8_t received_payload[16];
static uint16_t received_payload_length;

static void set_status(S64UciReply* reply, const char* text)
{
    uint16_t length = (uint16_t)strlen(text);

    assert(reply != 0);
    assert(reply->status != 0);
    assert(length + 1u <= reply->status_capacity);

    memcpy(reply->status, text, length + 1u);
    reply->status_length = length;
}

static S64UciResult mock_execute(
    const S64UciCommandPart* parts,
    uint8_t part_count,
    S64UciReply* reply
)
{
    ++calls;

    if (mode == 1u) {
        uint8_t handle = 7u;

        assert(part_count == 3u);
        assert(parts[0].length == 4u);
        assert(parts[0].data[0] == 0x03u);
        assert(parts[0].data[1] == 0x07u);
        assert(parts[0].data[2] == 80u);
        assert(parts[0].data[3] == 0u);
        assert(parts[1].flags == S64_UCI_PART_NATIVE_TEXT);
        assert(parts[1].length == strlen("example.com"));
        assert(parts[2].length == 1u);
        assert(parts[2].data[0] == 0u);

        assert(reply->data_sink(&handle, 1u, reply->data_context));
        set_status(reply, "00,OK");
        return S64_UCI_OK;
    }

    if (mode == 2u) {
        uint8_t count[2];

        assert(part_count == 2u);
        assert(parts[0].length == 3u);
        assert(parts[0].data[0] == 0x03u);
        assert(parts[0].data[1] == 0x11u);
        captured_flags = parts[1].flags;
        captured_length = parts[1].length;

        count[0] = (uint8_t)(parts[1].length & 0xFFu);
        count[1] = (uint8_t)(parts[1].length >> 8);
        assert(reply->data_sink(count, 2u, reply->data_context));
        set_status(reply, "00,OK");
        return S64_UCI_OK;
    }

    if (mode == 3u) {
        static const uint8_t first[] = {5u, 0u, 'H'};
        static const uint8_t second[] = {'e', 'l', 'l', 'o'};

        assert(part_count == 1u);
        assert(parts[0].data[0] == 0x03u);
        assert(parts[0].data[1] == 0x10u);
        assert(parts[0].data[3] == 64u);
        assert(parts[0].data[4] == 0u);

        assert(reply->data_sink(first, sizeof(first), reply->data_context));
        assert(reply->data_sink(second, sizeof(second), reply->data_context));
        set_status(reply, "00,OK");
        return S64_UCI_OK;
    }

    if (mode == 4u) {
        set_status(reply, "02,NO DATA");
        return S64_UCI_OK;
    }

    if (mode == 5u) {
        set_status(reply, "01,CONNECTION CLOSED BY HOST");
        return S64_UCI_OK;
    }

    if (mode == 6u) {
        assert(part_count == 1u);
        assert(parts[0].data[0] == 0x03u);
        assert(parts[0].data[1] == 0x09u);
        set_status(reply, "00,OK");
        return S64_UCI_OK;
    }

    return S64_UCI_PROTOCOL_ERROR;
}

static uint8_t collect(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    (void)context;
    assert(received_payload_length + length <= sizeof(received_payload));
    memcpy(received_payload + received_payload_length, data, length);
    received_payload_length = (uint16_t)(received_payload_length + length);
    return 1u;
}

int main(void)
{
    S64NetTransport transport;
    S64NetResult result;
    uint8_t handle = 0u;
    uint16_t received = 0u;

    s64_net_init(&transport, mock_execute);

    mode = 1u;
    calls = 0u;
    result = s64_net_open_tcp(&transport, "example.com", 80u, &handle);
    assert(result == S64_NET_OK);
    assert(handle == 7u);
    assert(calls == 1u);

    mode = 2u;
    captured_flags = 0u;
    captured_length = 0u;
    result = s64_net_write_text(&transport, handle, "GET /\r\n", 7u);
    assert(result == S64_NET_OK);
    assert(captured_flags == S64_UCI_PART_NATIVE_TEXT);
    assert(captured_length == 7u);

    mode = 3u;
    received_payload_length = 0u;
    result = s64_net_read(
        &transport,
        handle,
        64u,
        collect,
        0,
        &received
    );
    assert(result == S64_NET_OK);
    assert(received == 5u);
    assert(received_payload_length == 5u);
    assert(memcmp(received_payload, "Hello", 5u) == 0);

    mode = 4u;
    result = s64_net_read(&transport, handle, 64u, collect, 0, &received);
    assert(result == S64_NET_NO_DATA);
    assert(received == 0u);

    mode = 5u;
    result = s64_net_read(&transport, handle, 64u, collect, 0, &received);
    assert(result == S64_NET_CLOSED);
    assert(received == 0u);

    mode = 6u;
    result = s64_net_close(&transport, handle);
    assert(result == S64_NET_OK);

    puts("network tests passed");
    return 0;
}
