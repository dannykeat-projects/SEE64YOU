#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/http.h"

#define MOCK_MAX_COMMANDS 8u
#define MOCK_MAX_BYTES 896u

typedef struct {
    unsigned int count;
    unsigned int lengths[MOCK_MAX_COMMANDS];
    unsigned char part_counts[MOCK_MAX_COMMANDS];
    unsigned char part_flags[MOCK_MAX_COMMANDS][4];
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
    require(part_count <= 4u, "mock part capacity");
    mock.part_counts[command_index] = part_count;

    for (i = 0u; i < part_count; ++i) {
        mock.part_flags[command_index][i] = parts[i].flags;
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
    uint8_t response_header_handle;
    uint8_t response_body_handle;
    int32_t integer_value;
    char queried_string[16];
    unsigned char payload[1000];
    unsigned int i;
    BodySink sink;
    char response_header[S64_HTTP_RESPONSE_HEADER_CAPACITY];
    uint16_t response_header_length = 0u;

    s64_http_init(&http, mock_execute);

    mock_reset();
    mock.status = "00,OK";
    require(
        s64_http_free_all(&http) == S64_HTTP_OK,
        "stock C64U 1.1 HTTP success status accepted"
    );

    mock_reset();
    mock.status = "001 ERROR";
    require(
        s64_http_free_all(&http) == S64_HTTP_TARGET_ERROR,
        "non-success HTTP status remains rejected"
    );

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
    require(mock.part_counts[0] == 2u, "header create part count");
    require(mock.part_flags[0][0] == 0u, "header create prefix is binary");
    require(mock.part_flags[0][1] == S64_UCI_PART_NATIVE_TEXT, "header create URL is text");
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
    require(mock.part_flags[0][0] == 0u, "header add prefix is binary");
    require(mock.part_flags[0][1] == S64_UCI_PART_NATIVE_TEXT, "header line is text");

    mock_reset();
    mock.response[0] = 3u;
    mock.response_length = 1u;
    require(
        s64_http_body_create(&http, S64_HTTP_BODY_BINARY, &handle) == S64_HTTP_OK,
        "body create succeeds"
    );
    require(handle == 3u, "body handle captured");

    mock_reset();
    require(
        s64_http_body_add_string(&http, 3u, "password", "secret") == S64_HTTP_OK,
        "string body field succeeds"
    );
    require(mock.bytes[0][0] == 0x06u && mock.bytes[0][1] == 0x25u, "string add opcode");
    require(mock.bytes[0][2] == 3u && mock.bytes[0][3] == 8u, "string add handle and key length");
    require(memcmp(mock.bytes[0] + 4u, "password", 8u) == 0, "string add key");
    require(mock.bytes[0][12] == 6u, "string add value length");
    require(memcmp(mock.bytes[0] + 13u, "secret", 6u) == 0, "string add value");
    require(mock.part_counts[0] == 4u, "string add part count");
    require(mock.part_flags[0][0] == 0u, "string add prefix is binary");
    require(mock.part_flags[0][1] == S64_UCI_PART_NATIVE_TEXT, "string key is text");
    require(mock.part_flags[0][2] == 0u, "string length is binary");
    require(mock.part_flags[0][3] == S64_UCI_PART_NATIVE_TEXT, "string value is text");

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
    require(mock.part_flags[0][0] == 0u && mock.part_flags[0][1] == 0u,
        "binary body remains binary");

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
    mock.status = "HTTP/1.1 200 OK";
    mock.response[0] = 9u;
    mock.response[1] = 10u;
    mock.response_length = 2u;
    require(
        s64_http_exchange_object(
            &http,
            2u,
            S64_HTTP_NO_BODY,
            &response_header_handle,
            &response_body_handle,
            response_header,
            (uint16_t)sizeof(response_header),
            &response_header_length
        ) == S64_HTTP_OK,
        "object exchange succeeds"
    );
    require(mock.lengths[0] == 4u, "object exchange command length");
    require(mock.bytes[0][0] == 0x06u && mock.bytes[0][1] == 0x31u, "object exchange opcode");
    require(response_header_handle == 9u && response_body_handle == 10u, "object exchange handles");
    require(strcmp(response_header, "HTTP/1.1 200 OK") == 0, "object exchange response status");

    mock_reset();
    mock.response[0] = 0x03u;
    mock.response[1] = 5u;
    memcpy(mock.response + 2u, "hello", 5u);
    mock.response_length = 7u;
    require(
        s64_http_body_query_string(
            &http,
            10u,
            "documents%2/title",
            queried_string,
            (uint16_t)sizeof(queried_string)
        ) == S64_HTTP_OK,
        "string body query succeeds"
    );
    require(mock.bytes[0][0] == 0x06u && mock.bytes[0][1] == 0x2au, "body query opcode");
    require(mock.bytes[0][2] == 10u, "body query handle");
    require(
        memcmp(mock.bytes[0] + 3u, "documents%2/title", 17u) == 0,
        "body query path"
    );
    require(mock.part_flags[0][0] == 0u, "body query prefix is binary");
    require(mock.part_flags[0][1] == S64_UCI_PART_NATIVE_TEXT, "body query path is text");
    require(strcmp(queried_string, "hello") == 0, "string body query value");

    mock_reset();
    mock.response[0] = 0x01u;
    mock.response[1] = 0xd6u;
    mock.response[2] = 0xffu;
    mock.response[3] = 0xffu;
    mock.response[4] = 0xffu;
    mock.response_length = 5u;
    require(
        s64_http_body_query_integer(&http, 10u, "documents%2/order", &integer_value)
            == S64_HTTP_OK,
        "integer body query succeeds"
    );
    require(integer_value == -42, "integer body query signed value");

    mock_reset();
    mock.response[0] = 0x03u;
    mock.response[1] = 10u;
    memcpy(mock.response + 2u, "0123456789", 10u);
    mock.response_length = 12u;
    require(
        s64_http_body_query_string(&http, 10u, "documents%0/title", queried_string, 5u)
            == S64_HTTP_TRANSPORT_ERROR,
        "oversized string query is rejected safely"
    );

    mock_reset();
    mock.status = "400 BAD FORMAT: ENTRY documents NOT FOUND";
    require(
        s64_http_body_query_string(
            &http,
            10u,
            "documents%99/slug",
            queried_string,
            (uint16_t)sizeof(queried_string)
        ) == S64_HTTP_TARGET_ERROR,
        "missing query path maps to target error"
    );

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
