#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "publisher/relay.h"
#include "publisher/wordpress.h"
#include "publisher/wordpress_documents.h"

#define MOCK_COMMAND_CAPACITY 897u
#define WIRE_CAPTURE_CAPACITY 10000u

typedef enum {
    SCENARIO_LIST = 1,
    SCENARIO_OPEN,
    SCENARIO_OPEN_BAD_COUNT,
    SCENARIO_OPEN_BAD_BASE64,
    SCENARIO_SAVE,
    SCENARIO_DELETE,
    SCENARIO_AUTH_FAIL
} Scenario;

static unsigned char command[MOCK_COMMAND_CAPACITY];
static unsigned int command_length;
static unsigned int command_count;
static unsigned char last_header_verb;
static Scenario scenario;
static const char* exchange_status;
static unsigned char wire_capture[WIRE_CAPTURE_CAPACITY];
static unsigned int wire_length;
static unsigned int basic_headers;
static unsigned int content_type_headers;

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

static S64UciResult reply_bytes(
    S64UciReply* reply,
    const unsigned char* bytes,
    uint16_t length
)
{
    if (reply->data_sink != NULL
        && !reply->data_sink(bytes, length, reply->data_context)) {
        return S64_UCI_CONSUMER_ABORTED;
    }
    return S64_UCI_OK;
}

static S64UciResult reply_string(S64UciReply* reply, const char* value)
{
    unsigned char encoded[258];
    size_t length = strlen(value);

    require(length <= 255u, "mock string query capacity");
    encoded[0] = 0x03u;
    encoded[1] = (unsigned char)length;
    memcpy(encoded + 2u, value, length);
    reply_status(reply, "000 OK");
    return reply_bytes(reply, encoded, (uint16_t)(length + 2u));
}

static S64UciResult reply_integer(S64UciReply* reply, int32_t value)
{
    uint32_t raw = (uint32_t)value;
    unsigned char encoded[5];

    encoded[0] = 0x01u;
    encoded[1] = (unsigned char)(raw & 0xffu);
    encoded[2] = (unsigned char)((raw >> 8) & 0xffu);
    encoded[3] = (unsigned char)((raw >> 16) & 0xffu);
    encoded[4] = (unsigned char)((raw >> 24) & 0xffu);
    reply_status(reply, "000 OK");
    return reply_bytes(reply, encoded, 5u);
}

static S64UciResult reply_query(S64UciReply* reply, const char* path)
{
    if (scenario == SCENARIO_LIST) {
        if (strcmp(path, "count") == 0) return reply_integer(reply, 1);
        if (strcmp(path, "documents%0/id_text") == 0) return reply_string(reply, "17");
        if (strcmp(path, "documents%0/type") == 0) return reply_string(reply, "post");
        if (strcmp(path, "documents%0/slug") == 0) return reply_string(reply, "hello-world");
        if (strcmp(path, "documents%0/title") == 0) return reply_string(reply, "Hello World");
        if (strcmp(path, "documents%0/status") == 0) return reply_string(reply, "published");
        if (strcmp(path, "documents%0/date") == 0) return reply_string(reply, "2026-09-24T08:00:00+10:00");
        if (strcmp(path, "documents%0/order") == 0) return reply_integer(reply, 0);
    }

    if (scenario == SCENARIO_OPEN
        || scenario == SCENARIO_OPEN_BAD_COUNT
        || scenario == SCENARIO_OPEN_BAD_BASE64) {
        if (strcmp(path, "document/type") == 0) return reply_string(reply, "post");
        if (strcmp(path, "document/slug") == 0) return reply_string(reply, "opened-post");
        if (strcmp(path, "document/title") == 0) return reply_string(reply, "Opened Post");
        if (strcmp(path, "document/status") == 0) return reply_string(reply, "published");
        if (strcmp(path, "document/date") == 0) return reply_string(reply, "2026-09-24T08:00:00+10:00");
        if (strcmp(path, "document/order") == 0) return reply_integer(reply, -2);
        if (strcmp(path, "document/source_encoding") == 0) return reply_string(reply, "base64-chunks");
        if (strcmp(path, "document/source_length") == 0) {
            return reply_integer(reply, scenario == SCENARIO_OPEN_BAD_BASE64 ? 1 : 12);
        }
        if (strcmp(path, "document/source_count") == 0) {
            return reply_integer(reply, scenario == SCENARIO_OPEN_BAD_COUNT ? 2 : 1);
        }
        if (strcmp(path, "document/source_chunks%0") == 0) {
            return reply_string(
                reply,
                scenario == SCENARIO_OPEN_BAD_BASE64
                    ? "AB=="
                    : "SGVsbG8gd29ybGQK"
            );
        }
    }

    if (scenario == SCENARIO_SAVE && strcmp(path, "id_text") == 0) {
        return reply_string(reply, "42");
    }

    if (scenario == SCENARIO_DELETE && strcmp(path, "deleted_int") == 0) {
        return reply_integer(reply, 1);
    }

    reply_status(reply, "400 BAD FORMAT");
    return S64_UCI_OK;
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
    unsigned char handles[2];

    for (i = 0u; i < part_count; ++i) {
        require(offset + parts[i].length <= S64_UCI_COMMAND_CAPACITY, "UCI command capacity");
        for (j = 0u; j < parts[i].length; ++j) {
            command[offset++] = parts[i].data[j];
        }
    }
    command[offset] = '\0';
    command_length = offset;
    ++command_count;

    require(command_length >= 2u && command[0] == 0x06u, "HTTP target command");
    opcode = command[1];

    if (opcode == 0x11u) {
        last_header_verb = command[2];
        handle = 1u;
        reply_status(reply, "000 OK");
        return reply_bytes(reply, &handle, 1u);
    }

    if (opcode == 0x21u) {
        handle = 4u;
        reply_status(reply, "000 OK");
        return reply_bytes(reply, &handle, 1u);
    }

    if (opcode == 0x13u) {
        static const char basic[] = "Authorization: Basic ";
        static const char content_type[] = "Content-Type: application/x-see64you";

        reply_status(reply, "000 OK");
        if (command_length >= 3u + sizeof(basic) - 1u
            && memcmp(command + 3u, basic, sizeof(basic) - 1u) == 0) {
            ++basic_headers;
        }
        if (command_length == 3u + sizeof(content_type) - 1u
            && memcmp(command + 3u, content_type, sizeof(content_type) - 1u) == 0) {
            ++content_type_headers;
        }
        return S64_UCI_OK;
    }

    if (opcode == 0x2cu) {
        unsigned int payload = command_length - 3u;
        require(wire_length + payload <= sizeof(wire_capture), "wire capture capacity");
        memcpy(wire_capture + wire_length, command + 3u, payload);
        wire_length += payload;
        reply_status(reply, "000 OK");
        return S64_UCI_OK;
    }

    if (opcode == 0x31u) {
        handles[0] = 2u;
        handles[1] = 3u;
        reply_status(reply, exchange_status);
        return reply_bytes(reply, handles, 2u);
    }

    if (opcode == 0x2au) {
        return reply_query(reply, (const char*)(command + 3u));
    }

    reply_status(reply, "000 OK");
    return S64_UCI_OK;
}

static void reset_mock(Scenario next, const char* status)
{
    scenario = next;
    exchange_status = status;
    command_count = 0u;
    command_length = 0u;
    wire_length = 0u;
    basic_headers = 0u;
    content_type_headers = 0u;
    last_header_verb = 0u;
}

static void insert_text(S64Buffer* buffer, const char* text)
{
    while (*text != '\0') {
        require(s64_buffer_insert(buffer, (unsigned char)*text), "insert fixture text");
        ++text;
    }
}

static void require_buffer_text(const S64Buffer* buffer, const char* expected, const char* message)
{
    unsigned int i;
    size_t length = strlen(expected);

    require(s64_buffer_length(buffer) == length, message);
    for (i = 0u; i < length; ++i) {
        require(s64_buffer_char_at(buffer, i) == (unsigned char)expected[i], message);
    }
}

typedef struct {
    unsigned int count;
    S64WordPressDocumentSummary item;
} SummaryCapture;

static uint8_t capture_summary(
    const S64WordPressDocumentSummary* summary,
    uint16_t index,
    void* context
)
{
    SummaryCapture* capture = (SummaryCapture*)context;
    require(index == 0u, "list summary index");
    capture->item = *summary;
    ++capture->count;
    return 1u;
}

int main(void)
{
    S64HttpTransport http;
    S64WordPressSession session;
    S64WordPressResult result;
    SummaryCapture capture;
    uint16_t total = 0u;
    uint16_t delivered = 0u;
    S64Buffer body;
    S64Document document;
    char id[S64_WORDPRESS_ID_CAPACITY];

    s64_http_init(&http, mock_execute);
    require(s64_relay_set_key(NULL), "relay disabled");
    require(
        s64_wordpress_session_configure(&session, "editor", "abcd efgh ijkl"),
        "WordPress session configured"
    );

    reset_mock(SCENARIO_LIST, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n");
    memset(&capture, 0, sizeof(capture));
    result = s64_wordpress_list_documents(
        &http,
        "http://example.test/wp-json/see64you/v1/documents?type=post&managed=1",
        1u,
        &session,
        capture_summary,
        &capture,
        &total,
        &delivered
    );
    require(result == S64_WORDPRESS_OK, "WordPress list succeeds");
    require(total == 1u && delivered == 1u && capture.count == 1u, "WordPress list counts");
    require(strcmp(capture.item.id, "17") == 0, "WordPress list id");
    require(strcmp(capture.item.slug, "hello-world") == 0, "WordPress list slug");
    require(capture.item.status == S64_DOCUMENT_PUBLISHED, "WordPress list status");
    require(basic_headers == 1u, "WordPress list sends Basic auth");

    s64_buffer_init(&body);
    s64_document_init(&document, &body);
    insert_text(&body, "KEEP");
    strcpy(document.title, "Existing");

    reset_mock(SCENARIO_AUTH_FAIL, "HTTP/1.1 401 Unauthorized\r\nContent-Type: application/json\r\n");
    result = s64_wordpress_open_document(
        &http,
        "http://example.test/wp-json/see64you/v1/documents/17?chunks=1",
        1u,
        &session,
        &document
    );
    require(result == S64_WORDPRESS_AUTH_FAILED, "WordPress open maps auth failure");
    require_buffer_text(&body, "KEEP", "pre-load failure preserves editor body");
    require(strcmp(document.title, "Existing") == 0, "pre-load failure preserves metadata");

    reset_mock(SCENARIO_OPEN, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n");
    result = s64_wordpress_open_document(
        &http,
        "http://example.test/wp-json/see64you/v1/documents/17?chunks=1",
        1u,
        &session,
        &document
    );
    require(result == S64_WORDPRESS_OK, "WordPress open succeeds");
    require(document.type == S64_DOCUMENT_POST, "WordPress open type");
    require(document.status == S64_DOCUMENT_PUBLISHED, "WordPress open status");
    require(document.order == -2, "WordPress open order");
    require(strcmp(document.slug, "opened-post") == 0, "WordPress open slug");
    require(strcmp(document.title, "Opened Post") == 0, "WordPress open title");
    require_buffer_text(&body, "Hello world\n", "WordPress source chunks decode");
    require(s64_buffer_cursor(&body) == 0u, "WordPress open cursor at start");

    reset_mock(SCENARIO_OPEN_BAD_COUNT, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n");
    result = s64_wordpress_open_document(
        &http,
        "http://example.test/wp-json/see64you/v1/documents/17?chunks=1",
        1u,
        &session,
        &document
    );
    require(result == S64_WORDPRESS_PROTOCOL_ERROR, "wrong source chunk count rejected");
    require(s64_buffer_length(&body) == 0u, "bad chunk count clears partial destination");

    reset_mock(SCENARIO_OPEN_BAD_BASE64, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n");
    result = s64_wordpress_open_document(
        &http,
        "http://example.test/wp-json/see64you/v1/documents/17?chunks=1",
        1u,
        &session,
        &document
    );
    require(result == S64_WORDPRESS_PROTOCOL_ERROR, "non-canonical base64 rejected");
    require(s64_buffer_length(&body) == 0u, "bad base64 clears partial destination");

    s64_buffer_init(&body);
    s64_document_init(&document, &body);
    document.type = S64_DOCUMENT_POST;
    document.status = S64_DOCUMENT_PUBLISHED;
    require(s64_document_set_slug(&document, "saved-post"), "save slug");
    require(s64_document_set_title(&document, "Saved Post"), "save title");
    require(s64_document_set_date(&document, "2026-09-24T08:30:00+10:00"), "save date");
    document.order = 0;
    insert_text(&body, "Saved **body**.");

    reset_mock(SCENARIO_SAVE, "HTTP/1.1 201 Created\r\nContent-Type: application/json\r\n");
    result = s64_wordpress_save_document(
        &http,
        "http://example.test/wp-json/see64you/v1/documents?ack=1",
        1u,
        &session,
        &document,
        0u,
        id,
        (uint16_t)sizeof(id)
    );
    require(result == S64_WORDPRESS_OK, "WordPress create succeeds");
    require(strcmp(id, "42") == 0, "WordPress create id");
    require(last_header_verb == (unsigned char)S64_HTTP_POST, "create uses POST");
    require(content_type_headers == 1u, "create sends wire content type");
    require(
        wire_length >= sizeof("SEE64YOU/1\n") - 1u
        && memcmp(
            wire_capture,
            "SEE64YOU/1\n",
            sizeof("SEE64YOU/1\n") - 1u
        ) == 0,
        "create streams wire header"
    );
    require(
        wire_length >= strlen("Saved **body**.")
        && memcmp(
            wire_capture + wire_length - strlen("Saved **body**."),
            "Saved **body**.",
            strlen("Saved **body**.")
        ) == 0,
        "create streams source body"
    );

    reset_mock(SCENARIO_SAVE, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n");
    result = s64_wordpress_save_document(
        &http,
        "http://example.test/wp-json/see64you/v1/documents/42?ack=1",
        1u,
        &session,
        &document,
        1u,
        id,
        (uint16_t)sizeof(id)
    );
    require(result == S64_WORDPRESS_OK, "WordPress update succeeds");
    require(last_header_verb == (unsigned char)S64_HTTP_PATCH, "update uses PATCH");

    reset_mock(SCENARIO_SAVE, "HTTP/1.1 409 Conflict\r\nContent-Type: application/json\r\n");
    result = s64_wordpress_save_document(
        &http,
        "http://example.test/wp-json/see64you/v1/documents?ack=1",
        1u,
        &session,
        &document,
        0u,
        id,
        (uint16_t)sizeof(id)
    );
    require(result == S64_WORDPRESS_SLUG_CONFLICT, "slug conflict mapped distinctly");
    require(id[0] == '\0', "slug conflict clears returned id");

    reset_mock(SCENARIO_SAVE, "HTTP/1.1 413 Payload Too Large\r\nContent-Type: application/json\r\n");
    result = s64_wordpress_save_document(
        &http,
        "http://example.test/wp-json/see64you/v1/documents?ack=1",
        1u,
        &session,
        &document,
        0u,
        id,
        (uint16_t)sizeof(id)
    );
    require(result == S64_WORDPRESS_INCOMPATIBLE_DOCUMENT, "oversized document mapped distinctly");
    require(id[0] == '\0', "oversized response clears returned id");

    reset_mock(SCENARIO_SAVE, "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\n");
    result = s64_wordpress_save_document(
        &http,
        "http://example.test/wp-json/see64you/v1/documents?ack=1",
        1u,
        &session,
        &document,
        0u,
        id,
        (uint16_t)sizeof(id)
    );
    require(result == S64_WORDPRESS_DOCUMENT_REJECTED, "bad document mapped distinctly");
    require(id[0] == '\0', "rejected document clears returned id");

    reset_mock(SCENARIO_DELETE, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n");
    result = s64_wordpress_delete_document(
        &http,
        "http://example.test/wp-json/see64you/v1/documents/42",
        1u,
        &session
    );
    require(result == S64_WORDPRESS_OK, "WordPress delete succeeds");
    require(last_header_verb == (unsigned char)S64_HTTP_DELETE, "delete uses DELETE");

    s64_document_init(&document, &body);
    id[0] = 'x';
    id[1] = '\0';
    reset_mock(SCENARIO_SAVE, "HTTP/1.1 201 Created\r\n");
    result = s64_wordpress_save_document(
        &http,
        "http://example.test/wp-json/see64you/v1/documents?ack=1",
        1u,
        &session,
        &document,
        0u,
        id,
        (uint16_t)sizeof(id)
    );
    require(result == S64_WORDPRESS_INVALID_ARGUMENT, "invalid document save rejected locally");
    require(id[0] == '\0', "failed save clears returned id");

    puts("wordpress document publisher tests passed");
    return 0;
}
