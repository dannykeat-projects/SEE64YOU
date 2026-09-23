#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/http.h"

#define MOCK_MAX_COMMANDS 8u
#define MOCK_MAX_BYTES 896u

typedef struct {
    unsigned int count;
    unsigned int lengths[MOCK_MAX_COMMANDS];
    unsigned char bytes[MOCK_MAX_COMMANDS][MOCK_MAX_BYTES];
    const char* status;
    unsigned char response[16];
    unsigned int response_length;
    S64UciResult result;
} MockUci;

static MockUci mock;

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void mock_reset(void)
{
    memset(&mock, 0, sizeof(mock));
    mock.status = "000 OK";
    mock.result = S64_UCI_OK;
}

static S64UciResult mock_execute(
    const S64UciCommandPart* parts,
    uint8_t part_count,
    S64UciReply* reply
)
{
    unsigned int command_index = mock.count;
    unsigned int offset = 0u;
    unsigned int i;
    unsigned int j;
    size_t status_length;

    require(command_index < MOCK_MAX_COMMANDS, "mock command capacity");

    for (i = 0u; i < part_count; ++i) {
        require(offset + parts[i].length <= MOCK_MAX_BYTES, "mock byte capacity");
        for (j = 0u; j < parts[i].length; ++j) {
            mock.bytes[command_index][offset++] = parts[i].data[j];
        }
    }

    mock.lengths[command_index] = offset;
    ++mock.count;

    if (reply != NULL) {
        status_length = strlen(mock.status);
        reply->status_length = (uint16_t)status_length;

        if (reply->status != NULL && reply->status_capacity != 0u) {
            size_t copy = status_length;
            if (copy >= reply->status_capacity) {
                copy = reply->status_capacity - 1u;
            }
            memcpy(reply->status, mock.status, copy);
            reply->status[copy] = '\0';
        }

        if (mock.response_length != 0u && reply->data_sink != NULL) {
            if (!reply->data_sink(
                    mock.response,
                    (uint16_t)mock.response_length,
                    reply->data_context
                )) {
                return S64_UCI_CONSUMER_ABORTED;
            }
        }
    }

    return mock.result;
}

typedef struct {
    unsigned char bytes[32];
    unsigned int length;
} BodySink;

static uint8_t collect_body(const uint8_t* data, uint16_t length, void* context)
{
    BodySink* sink = (BodySink*)context;

    require(sink->length + length <= sizeof(sink->bytes), "body sink capacity");
    memcpy(sink->bytes + sink->length, data, length);
    sink->length += length;
    return 1u;
}

int main(void)
{
    S64HttpTransport http;
    uint8_t handle;
    unsigned char payload[1000];
    unsigned int i;
    BodySink sink;
    char response_header[S64_HTTP_RESPONSE_HEADER_CAPACITY];
    uint16_t response_header_length = 0u;

    s64_http_init(&http, mock_execute);

    mock_reset();
    mock.response[0] = 7u;
    mock.response_length = 1u;
    require(
        s64_http_header_create(
            &http,
            S64_HTTP_POST,
            "example.test/api.php?action=save",
            &handle
        ) == S64_HTTP_OK,
        "header create succeeds"
    );
    require(handle == 7u, "header handle captured");
    require(mock.lengths[0] == 35u, "header create command length");
    require(mock.bytes[0][0] == 0x06u, "header create target");
    require(mock.bytes[0][1] == 0x11u, "header create opcode");
    require(mock.bytes[0][2] == 0x03u, "header create verb");
    require(
        memcmp(mock.bytes[0] + 3u, "example.test/api.php?action=save", 32u) == 0,
        "header create URL"
    );

    mock_reset();
    require(
        s64_http_header_add(&http, 4u, "Authorization: Bearer token") == S64_HTTP_OK,
        "header add succeeds"
    );
    require(mock.bytes[0][0] == 0x06u && mock.bytes[0][1] == 0x13u, "header add opcode");
    require(mock.bytes[0][2] == 4u, "header add handle");

    mock_reset();
    mock.response[0] = 3u;
    mock.response_length = 1u;
    require(
        s64_http_body_create(&http, S64_HTTP_BODY_BINARY, &handle) == S64_HTTP_OK,
        "body create succeeds"
    );
    require(handle == 3u, "body handle captured");

    for (i = 0u; i < sizeof(payload); ++i) {
        payload[i] = (unsigned char)(i & 0xffu);
    }

    mock_reset();
    require(
        s64_http_body_add_binary(&http, 3u, payload, (uint16_t)sizeof(payload)) == S64_HTTP_OK,
        "binary body streams in chunks"
    );
    require(mock.count == 2u, "large body uses two UCI commands");
    require(mock.lengths[0] == 896u, "first binary chunk fills UCI queue");
    require(mock.lengths[1] == 110u, "second binary chunk contains remainder");
    require(mock.bytes[0][0] == 0x06u && mock.bytes[0][1] == 0x2cu, "binary add opcode");
    require(mock.bytes[0][2] == 3u, "binary add handle");
    require(memcmp(mock.bytes[0] + 3u, payload, 893u) == 0, "first binary payload");
    require(memcmp(mock.bytes[1] + 3u, payload + 893u, 107u) == 0, "second binary payload");

    mock_reset();
    mock.status = "HTTP/1.1 200 OK\r\nContent-Type: application/x-see64you\r\n";
    memcpy(mock.response, "BODY", 4u);
    mock.response_length = 4u;
    memset(&sink, 0, sizeof(sink));
    require(
        s64_http_exchange_raw(
            &http,
            2u,
            S64_HTTP_NO_BODY,
            collect_body,
            &sink,
            response_header,
            (uint16_t)sizeof(response_header),
            &response_header_length
        ) == S64_HTTP_OK,
        "raw exchange succeeds"
    );
    require(mock.lengths[0] == 4u, "exchange command length");
    require(mock.bytes[0][0] == 0x06u && mock.bytes[0][1] == 0x32u, "exchange opcode");
    require(mock.bytes[0][2] == 2u && mock.bytes[0][3] == 0xffu, "exchange handles");
    require(sink.length == 4u && memcmp(sink.bytes, "BODY", 4u) == 0, "response body streamed");
    require(response_header_length == strlen(mock.status), "response header length returned");
    require(strcmp(response_header, mock.status) == 0, "response header captured");

    mock_reset();
    mock.status = "507 NO HEADER SLOT";
    mock.response[0] = 1u;
    mock.response_length = 1u;
    require(
        s64_http_header_create(&http, S64_HTTP_GET, "example.test/", &handle)
            == S64_HTTP_TARGET_ERROR,
        "target status is not mistaken for transport success"
    );

    puts("http transport tests passed");
    return 0;
}
