#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "publisher/standalone_documents.h"

#define MOCK_MAX_COMMAND 896u
#define MOCK_WIRE_CAPACITY 1024u

static const char* exchange_status;
static const char* exchange_body;
static unsigned char command[MOCK_MAX_COMMAND];
static unsigned int command_length;
static unsigned char wire_body[MOCK_WIRE_CAPACITY];
static unsigned int wire_body_length;
static unsigned int authorization_headers;
static unsigned int wire_content_headers;
static unsigned int type_fields;
static unsigned int slug_fields;

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void reset_mock(void)
{
    command_length = 0u;
    wire_body_length = 0u;
    authorization_headers = 0u;
    wire_content_headers = 0u;
    type_fields = 0u;
    slug_fields = 0u;
}

static void reply_status(S64UciReply* reply, const char* status)
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

static void inspect_string_field(void)
{
    unsigned int key_length;

    if (command_length < 5u) {
        return;
    }

    key_length = command[3];
    if (4u + key_length + 1u > command_length) {
        return;
    }

    if (key_length == 4u && memcmp(command + 4u, "type", 4u) == 0) {
        ++type_fields;
    }
    if (key_length == 4u && memcmp(command + 4u, "slug", 4u) == 0) {
        ++slug_fields;
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
    unsigned char opcode;
    unsigned char handle;

    for (i = 0u; i < part_count; ++i) {
        require(offset + parts[i].length <= sizeof(command), "mock command capacity");
        for (j = 0u; j < parts[i].length; ++j) {
            command[offset++] = parts[i].data[j];
        }
    }

    command_length = offset;
    require(offset >= 2u && command[0] == 0x06u, "HTTP target command");
    opcode = command[1];

    if (opcode == 0x11u) {
        handle = 1u;
        reply_status(reply, "000 OK");
        require(reply->data_sink != NULL, "header handle sink");
        require(reply->data_sink(&handle, 1u, reply->data_context), "header handle accepted");
        return S64_UCI_OK;
    }

    if (opcode == 0x21u) {
        handle = 2u;
        reply_status(reply, "000 OK");
        require(reply->data_sink != NULL, "body handle sink");
        require(reply->data_sink(&handle, 1u, reply->data_context), "body handle accepted");
        return S64_UCI_OK;
    }

    if (opcode == 0x13u) {
        reply_status(reply, "000 OK");
        if (command_length > 3u
            && memcmp(command + 3u, "Authorization: Bearer ", 22u) == 0) {
            ++authorization_headers;
        }
        if (command_length > 3u
            && memcmp(
                command + 3u,
                "Content-Type: application/x-see64you",
                sizeof("Content-Type: application/x-see64you") - 1u
            ) == 0) {
            ++wire_content_headers;
        }
        return S64_UCI_OK;
    }

    if (opcode == 0x25u) {
        reply_status(reply, "000 OK");
        inspect_string_field();
        return S64_UCI_OK;
    }

    if (opcode == 0x2cu) {
        unsigned int body_length = command_length - 3u;

        reply_status(reply, "000 OK");
        require(wire_body_length + body_length < sizeof(wire_body), "wire capture capacity");
        memcpy(wire_body + wire_body_length, command + 3u, body_length);
        wire_body_length += body_length;
        wire_body[wire_body_length] = '\0';
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

static void set_session(S64StandaloneSession* session)
{
    strcpy(
        session->token,
        "v1.1790199999.0123456789abcdef."
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
    );
    session->expires = 1790199999u;
}

int main(void)
{
    S64HttpTransport http;
    S64StandaloneSession session;
    S64Buffer body;
    S64Document document;
    S64StandaloneResult result;
    const char* body_text = "Hello C64U";

    s64_http_init(&http, mock_execute);
    set_session(&session);

    s64_buffer_init(&body);
    while (*body_text != '\0') {
        require(s64_buffer_insert(&body, (unsigned char)*body_text++), "insert body");
    }

    s64_document_init(&document, &body);
    document.type = S64_DOCUMENT_POST;
    require(s64_document_set_slug(&document, "save-test"), "set save slug");
    require(s64_document_set_title(&document, "Save Test"), "set save title");

    reset_mock();
    exchange_status = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n";
    exchange_body = "{\"ok\":true,\"slug\":\"save-test\",\"status\":\"draft\"}";

    result = s64_standalone_save_document(
        &http,
        "http://example.test/api.php?action=save&ack=1",
        1u,
        &session,
        &document
    );

    require(result == S64_STANDALONE_OK, "save succeeds");
    require(authorization_headers == 1u, "save sends bearer auth");
    require(wire_content_headers == 1u, "save sends wire content type");
    require(
        strstr((const char*)wire_body, "SEE64YOU/1\nType: post\nSlug: save-test") != NULL,
        "save streams wire metadata"
    );
    require(
        strstr((const char*)wire_body, "\n\nHello C64U") != NULL,
        "save streams existing body"
    );

    reset_mock();
    exchange_status = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n";
    exchange_body = "{\"ok\":true,\"deleted\":true}";

    result = s64_standalone_delete_document(
        &http,
        "http://example.test/api.php?action=delete",
        1u,
        &session,
        S64_DOCUMENT_POST,
        "save-test"
    );

    require(result == S64_STANDALONE_OK, "delete succeeds");
    require(authorization_headers == 1u, "delete sends bearer auth");
    require(type_fields == 1u && slug_fields == 1u, "delete sends type and slug");

    exchange_status = "HTTP/1.1 401 Unauthorized\r\n";
    exchange_body = "{\"ok\":false,\"error\":\"unauthorized\"}";
    result = s64_standalone_delete_document(
        &http,
        "http://example.test/api.php?action=delete",
        1u,
        &session,
        S64_DOCUMENT_POST,
        "save-test"
    );
    require(result == S64_STANDALONE_AUTH_FAILED, "delete maps expired token");

    result = s64_standalone_save_document(
        &http,
        "https://example.test/api.php?action=save&ack=1",
        1u,
        &session,
        &document
    );
    require(result == S64_STANDALONE_TLS_UNSUPPORTED, "save respects TLS safety gate");

    puts("standalone document publisher tests passed");
    return 0;
}
