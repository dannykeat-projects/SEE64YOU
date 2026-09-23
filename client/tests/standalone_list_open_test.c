#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "publisher/standalone_documents.h"

#define MOCK_COMMAND_CAPACITY 897u

static unsigned char command[MOCK_COMMAND_CAPACITY];
static unsigned int command_length;
static const char* exchange_status;
static const char* raw_body;
static unsigned int authorization_headers;

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
    unsigned char encoded[130];
    size_t length = strlen(value);

    require(length <= 127u, "mock string capacity");
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
    if (strcmp(path, "count") == 0) return reply_integer(reply, 2);

    if (strcmp(path, "documents%0/type") == 0) return reply_string(reply, "post");
    if (strcmp(path, "documents%0/slug") == 0) return reply_string(reply, "first-post");
    if (strcmp(path, "documents%0/title") == 0) return reply_string(reply, "First Post");
    if (strcmp(path, "documents%0/status") == 0) return reply_string(reply, "published");
    if (strcmp(path, "documents%0/date") == 0) return reply_string(reply, "2026-09-23T20:00:00+10:00");
    if (strcmp(path, "documents%0/order") == 0) return reply_integer(reply, 0);

    if (strcmp(path, "documents%1/type") == 0) return reply_string(reply, "post");
    if (strcmp(path, "documents%1/slug") == 0) return reply_string(reply, "second-post");
    if (strcmp(path, "documents%1/title") == 0) return reply_string(reply, "Second Post");
    if (strcmp(path, "documents%1/status") == 0) return reply_string(reply, "draft");
    if (strcmp(path, "documents%1/date") == 0) return reply_string(reply, "2026-09-22T20:00:00+10:00");
    if (strcmp(path, "documents%1/order") == 0) return reply_integer(reply, -2);

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
    unsigned char handles[2];

    for (i = 0u; i < part_count; ++i) {
        require(offset + parts[i].length <= sizeof(command), "mock command capacity");
        for (j = 0u; j < parts[i].length; ++j) {
            command[offset++] = parts[i].data[j];
        }
    }
    require(offset <= S64_UCI_COMMAND_CAPACITY, "UCI command size");
    command[offset] = '\0';
    command_length = offset;
    require(command_length >= 2u && command[0] == 0x06u, "HTTP target command");
    opcode = command[1];

    if (opcode == 0x11u) {
        unsigned char handle = 1u;
        reply_status(reply, "000 OK");
        return reply_bytes(reply, &handle, 1u);
    }

    if (opcode == 0x13u) {
        reply_status(reply, "000 OK");
        if (command_length > 25u
            && memcmp(command + 3u, "Authorization: Bearer ", 22u) == 0) {
            ++authorization_headers;
        }
        return S64_UCI_OK;
    }

    if (opcode == 0x31u) {
        handles[0] = 2u;
        handles[1] = 3u;
        reply_status(reply, exchange_status);
        return reply_bytes(reply, handles, 2u);
    }

    if (opcode == 0x2au) {
        require(command_length > 3u, "query has path");
        return reply_query(reply, (const char*)(command + 3u));
    }

    if (opcode == 0x32u) {
        reply_status(reply, exchange_status);
        if (raw_body != NULL) {
            return reply_bytes(
                reply,
                (const unsigned char*)raw_body,
                (uint16_t)strlen(raw_body)
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

typedef struct {
    unsigned int count;
    char slugs[2][S64_DOCUMENT_SLUG_CAPACITY];
    int32_t orders[2];
} SummaryCapture;

static uint8_t capture_summary(
    const S64DocumentSummary* summary,
    uint16_t index,
    void* context
)
{
    SummaryCapture* capture = (SummaryCapture*)context;

    require(index < 2u, "summary index");
    strcpy(capture->slugs[index], summary->slug);
    capture->orders[index] = summary->order;
    ++capture->count;
    return 1u;
}

static uint8_t capture_one(
    const S64DocumentSummary* summary,
    uint16_t index,
    void* context
)
{
    SummaryCapture* capture = (SummaryCapture*)context;
    (void)index;

    strcpy(capture->slugs[0], summary->slug);
    ++capture->count;
    return 0u;
}

static void require_buffer_text(const S64Buffer* buffer, const char* expected)
{
    unsigned int i;
    size_t length = strlen(expected);

    require(s64_buffer_length(buffer) == length, "opened body length");
    for (i = 0u; i < length; ++i) {
        require(
            s64_buffer_char_at(buffer, i) == (unsigned char)expected[i],
            "opened body content"
        );
    }
}

int main(void)
{
    S64HttpTransport http;
    S64StandaloneSession session;
    S64StandaloneResult result;
    SummaryCapture capture;
    uint16_t total = 0u;
    uint16_t delivered = 0u;
    S64Buffer body;
    S64Document document;

    static const char wire[] =
        "SEE64YOU/1\n"
        "Type: post\n"
        "Slug: opened-post\n"
        "Title: Opened Post\n"
        "Status: draft\n"
        "Date: 2026-09-24T07:00:00+10:00\n"
        "Order: -3\n\n"
        "Body line one.\nBody line two.";

    s64_http_init(&http, mock_execute);
    set_session(&session);

    authorization_headers = 0u;
    exchange_status = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n";
    memset(&capture, 0, sizeof(capture));

    result = s64_standalone_list_documents(
        &http,
        "http://example.test/api.php?action=list&type=post",
        1u,
        &session,
        capture_summary,
        &capture,
        &total,
        &delivered
    );

    require(result == S64_STANDALONE_OK, "list succeeds");
    require(total == 2u && delivered == 2u && capture.count == 2u, "list counts");
    require(strcmp(capture.slugs[0], "first-post") == 0, "first list slug");
    require(strcmp(capture.slugs[1], "second-post") == 0, "second list slug");
    require(capture.orders[0] == 0 && capture.orders[1] == -2, "list order values");
    require(authorization_headers == 1u, "list sends bearer auth");

    memset(&capture, 0, sizeof(capture));
    total = 0u;
    delivered = 0u;
    result = s64_standalone_list_documents(
        &http,
        "http://example.test/api.php?action=list&type=post",
        1u,
        &session,
        capture_one,
        &capture,
        &total,
        &delivered
    );
    require(result == S64_STANDALONE_OK, "early-stop list succeeds");
    require(total == 2u && delivered == 1u && capture.count == 1u, "early-stop counts");

    raw_body = wire;
    exchange_status = "HTTP/1.1 200 OK\r\nContent-Type: application/x-see64you\r\n";
    s64_buffer_init(&body);
    s64_document_init(&document, &body);

    result = s64_standalone_open_document(
        &http,
        "http://example.test/api.php?action=get&type=post&slug=opened-post&format=s64",
        1u,
        &session,
        &document
    );

    require(result == S64_STANDALONE_OK, "open succeeds");
    require(document.type == S64_DOCUMENT_POST, "open type");
    require(document.status == S64_DOCUMENT_DRAFT, "open status");
    require(document.order == -3, "open order");
    require(strcmp(document.slug, "opened-post") == 0, "open slug");
    require(strcmp(document.title, "Opened Post") == 0, "open title");
    require_buffer_text(&body, "Body line one.\nBody line two.");
    require(s64_buffer_cursor(&body) == 0u, "open cursor at start");

    exchange_status = "HTTP/1.1 401 Unauthorized\r\nContent-Type: application/json\r\n";
    raw_body = "{\"ok\":false,\"error\":\"unauthorized\"}";
    result = s64_standalone_open_document(
        &http,
        "http://example.test/api.php?action=get&type=post&slug=opened-post&format=s64",
        1u,
        &session,
        &document
    );
    require(result == S64_STANDALONE_AUTH_FAILED, "open maps expired token");
    require(s64_buffer_length(&body) == 0u, "failed open clears partial destination");

    puts("standalone list/open tests passed");
    return 0;
}
