#include "standalone_direct.h"
#include "standalone_documents.h"
#include "direct_auth.h"
#include "workspace.h"
#include "charset.h"
#include "platform/network.h"

#ifdef S64_PERSISTENT_RUNTIME_FIT
#include "publisher/persistent_services.h"
#define s64_document_slug_valid s64_persistent_document_slug_valid
#define s64_document_type_name s64_persistent_document_type_name
#define s64_document_write_wire s64_persistent_document_write_wire
#endif

#include <string.h>

#define HOST_CAPACITY 96u
#define URI_CAPACITY 256u
#define LINE_CAPACITY 192u
#define REPLY_TEXT_CAPACITY 256u

/* One synchronous request at a time. Do not retain document payloads in RAM. */
#ifdef S64_PERSISTENT_RUNTIME_FIT
#define uri ((char*)&s64_publisher_workspace.bytes[0])
#define line ((char*)&s64_publisher_workspace.bytes[256])
/* Challenge path is consumed before this area becomes response-header scratch. */
#define challenge_uri ((char*)&s64_publisher_workspace.bytes[256])
#define host ((char*)&s64_publisher_workspace.bytes[512])
#define reply_text ((char*)&s64_publisher_workspace.bytes[S64_PUBLISH_WORK_DIRECT_REPLY_OFFSET])
typedef char S64PersistentDirectWorkspaceFits[
    S64_PUBLISH_WORKSPACE_CAPACITY - 864u
];
#else
static char host[HOST_CAPACITY], uri[URI_CAPACITY];
#define line ((char*)&s64_publisher_workspace.bytes[0])
/* Challenge path is sent before the response header reuses this area. */
#define challenge_uri ((char*)&s64_publisher_workspace.bytes[0])
#define reply_text ((char*)&s64_publisher_workspace.bytes[S64_PUBLISH_WORK_AUX_OFFSET])
#endif
extern uint8_t s64_direct_key_bytes[S64_HMAC_SHA256_KEY_SIZE];
extern uint8_t s64_direct_key_ready;
#define direct_key s64_direct_key_bytes
#define key_ready s64_direct_key_ready
static S64NetTransport network;
static uint8_t socket_handle;
static uint16_t status_code;

typedef int (*BodyWriter)(S64DocumentSink sink, void* context, const void* source);
typedef struct {
    S64Sha256 sha;
    uint16_t length;
} BodyHash;
/* Offset 256 also preserves host SHA context alignment. */
#define body_hash (*(BodyHash*)&s64_publisher_workspace.bytes[256])
static int write_document(S64DocumentSink sink, void* context, const void* source);
typedef struct {
    S64UciDataSink sink;
    void* context;
    uint16_t line_length, header_bytes, remaining;
    uint8_t headers_done, have_status, have_length, failed;
} Reply;

static uint16_t text_length;
#ifdef S64_PERSISTENT_RUNTIME_FIT
uint8_t s64_persistent_direct_trace;
#define S64_DIRECT_TRACE(value) (s64_persistent_direct_trace = (value))
#else
#define S64_DIRECT_TRACE(value) ((void)0)
#endif

static int equal_ci(const char* left, const char* right, uint8_t count)
{
    uint8_t i;
    char a, b;
    for (i = 0; i < count; ++i) {
        a = left[i]; b = right[i];
        if (!a || !b) return 0;
        if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
        if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
        if (a != b) return 0;
    }
    return 1;
}

static uint8_t append_text_bounded(
    char* output, uint16_t capacity, uint16_t* length, const char* text)
{
    uint16_t i = 0u;
    if (!output || !length || !text || !capacity) return 0u;
    while (text[i] != 0) {
        if (*length + 1u >= capacity) return 0u;
        output[(*length)++] = text[i++];
    }
    output[*length] = 0;
    return 1u;
}

static uint8_t append_u16_decimal(
    char* output, uint16_t capacity, uint16_t* length, uint16_t value)
{
    char digits[5];
    uint8_t count = 0u;
    do {
        digits[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    } while (value != 0u);
    while (count != 0u) {
        char digit[2];
        digit[0] = digits[--count];
        digit[1] = 0;
        if (!append_text_bounded(output, capacity, length, digit)) return 0u;
    }
    return 1u;
}

static uint8_t build_content_header(uint16_t body_length, uint8_t document_body)
{
    uint16_t length = 0u;
    line[0] = 0;
    return append_text_bounded(line, LINE_CAPACITY, &length, "Content-Type: ")
        && append_text_bounded(
            line, LINE_CAPACITY, &length,
            document_body ? "application/x-see64you" : "application/json")
        && append_text_bounded(line, LINE_CAPACITY, &length, "\r\nContent-Length: ")
        && append_u16_decimal(line, LINE_CAPACITY, &length, body_length)
        && append_text_bounded(line, LINE_CAPACITY, &length, "\r\n");
}

static S64StandaloneResult parse_url(const char* url, uint8_t allow_http)
{
    const char* start;
    const char* path;
    uint16_t length;
    if (!url) return S64_STANDALONE_INVALID_ARGUMENT;
    if (equal_ci(url, "https://", 8)) return S64_STANDALONE_TLS_UNSUPPORTED;
    if (!allow_http) return S64_STANDALONE_INSECURE_TRANSPORT;
    if (!equal_ci(url, "http://", 7)) return S64_STANDALONE_INVALID_ARGUMENT;
    start = url + 7;
    path = strchr(start, '/');
    if (!path || path == start) return S64_STANDALONE_INVALID_ARGUMENT;
    length = (uint16_t)(path - start);
    if (length >= HOST_CAPACITY || strlen(path) >= URI_CAPACITY)
        return S64_STANDALONE_INVALID_ARGUMENT;
    memcpy(host, start, length); host[length] = 0;
    /* Reject credentials, ports and request-line/header injection. Port 80 only. */
    if (strpbrk(host, ":@?# \r\n\t") || strpbrk(path, "# \r\n\t"))
        return S64_STANDALONE_INVALID_ARGUMENT;
    strcpy(uri, path);
    return S64_STANDALONE_OK;
}

static uint8_t collect_text(const uint8_t* data, uint16_t length, void* context)
{
    uint16_t i;
    uint8_t native;
    (void)context;
    for (i = 0; i < length; ++i) {
        if (text_length + 1u >= REPLY_TEXT_CAPACITY
            || !s64_charset_ascii_to_native(data[i], &native)) return 0;
        reply_text[text_length++] = (char)native;
    }
    reply_text[text_length] = 0;
    return 1;
}

static uint8_t read_reply(const uint8_t* data, uint16_t length, void* context)
{
    Reply* reply = (Reply*)context;
    uint16_t i = 0, amount, value;
    uint8_t native;
    const char* digit;
    while (i < length && !reply->headers_done) {
        if (++reply->header_bytes > 4096u) goto fail;
        if (data[i] == 0x0du) { ++i; continue; }
        if (data[i] != 0x0au) {
            if (reply->line_length + 1u >= LINE_CAPACITY
                || !s64_charset_ascii_to_native(data[i++], &native)) goto fail;
            line[reply->line_length++] = (char)native;
            continue;
        }
        ++i;
        line[reply->line_length] = 0;
        if (!reply->have_status) {
            if ((!equal_ci(line, "HTTP/1.1 ", 9) && !equal_ci(line, "HTTP/1.0 ", 9))
                || reply->line_length < 12u) goto fail;
            status_code = 0;
            for (amount = 9; amount < 12; ++amount) {
                if (line[amount] < '0' || line[amount] > '9') goto fail;
                status_code = status_code * 10u + (uint8_t)(line[amount] - '0');
            }
            reply->have_status = 1;
        } else if (!reply->line_length) {
            reply->headers_done = 1;
        } else if (equal_ci(line, "Content-Length:", 15)) {
            if (reply->have_length) goto fail;
            digit = line + 15;
            while (*digit == ' ') ++digit;
            if (*digit < '0' || *digit > '9') goto fail;
            value = 0;
            while (*digit >= '0' && *digit <= '9') {
                if (value > 6553u || (value == 6553u && *digit > '5')) goto fail;
                value = value * 10u + (uint8_t)(*digit++ - '0');
            }
            if (*digit) goto fail;
            reply->remaining = value; reply->have_length = 1;
        } else if (equal_ci(line, "Transfer-Encoding:", 18)) {
            /* HTTP/1.0 requests ask for a close-delimited/non-chunked response. */
            goto fail;
        }
        reply->line_length = 0;
    }
    amount = (uint16_t)(length - i);
    if (reply->headers_done && amount) {
        if (reply->have_length && amount > reply->remaining) goto fail;
        if (reply->have_length) reply->remaining -= amount;
        /* Error pages never overwrite the caller's document buffer. */
        if (status_code >= 200u && status_code < 300u && reply->sink
            && !reply->sink(data + i, amount, reply->context)) goto fail;
    }
    return 1;
fail:
    reply->failed = 1;
    return 0;
}

static int send_text(const char* text)
{
    return s64_net_write_text(&network, socket_handle, text,
        (uint16_t)strlen(text)) == S64_NET_OK;
}

static uint8_t send_body(const uint8_t* data, uint16_t length, void* context)
{
    (void)context;
    return s64_net_write(&network, socket_handle, data, length) == S64_NET_OK;
}

static S64StandaloneResult exchange(const char* method, const char* path,
    const char* challenge, const char* signature, uint16_t body_length,
    BodyWriter writer, const void* source, S64UciDataSink sink, void* context)
{
    Reply reply;
    S64NetResult result;
    S64StandaloneResult outcome = S64_STANDALONE_TRANSPORT_ERROR;
    uint16_t idle = 0, received;
    if (s64_net_open_tcp(&network, host, 80u, &socket_handle) != S64_NET_OK)
        return outcome;
    memset(&reply, 0, sizeof(reply));
    reply.sink = sink; reply.context = context;
    status_code = 0; text_length = 0; reply_text[0] = 0;
    if (!send_text(method) || !send_text(" ") || !send_text(path)
        || !send_text(" HTTP/1.0\r\nHost: ") || !send_text(host)
        || !send_text("\r\nConnection: close\r\n")) goto cleanup;
    if (challenge && (!send_text("X-See64You-Challenge: ") || !send_text(challenge)
        || !send_text("\r\nX-See64You-Signature: ") || !send_text(signature)
        || !send_text("\r\n"))) goto cleanup;
    if (writer) {
        if (!build_content_header(body_length, writer == write_document)
            || !send_text(line)) goto cleanup;
    }
    if (!send_text("\r\n") || (writer && !writer(send_body, 0, source))) goto cleanup;
    while (idle < 512u) {
        received = 0;
        result = s64_net_read(&network, socket_handle, S64_NET_READ_MAX,
            read_reply, &reply, &received);
        if (reply.failed) { outcome = S64_STANDALONE_PROTOCOL_ERROR; goto cleanup; }
        if (result != S64_NET_OK && result != S64_NET_NO_DATA && result != S64_NET_CLOSED)
            goto cleanup;
        if (received) idle = 0; else ++idle;
        if (reply.headers_done && reply.have_length && !reply.remaining) break;
        if (result == S64_NET_CLOSED) break;
    }
    if (!reply.headers_done || (reply.have_length && reply.remaining)
        || idle == 512u) { outcome = S64_STANDALONE_PROTOCOL_ERROR; goto cleanup; }
    if (status_code == 401u) outcome = S64_STANDALONE_AUTH_FAILED;
    else if (status_code >= 200u && status_code < 300u) outcome = S64_STANDALONE_OK;
    else outcome = S64_STANDALONE_SERVER_ERROR;
cleanup:
    (void)s64_net_close(&network, socket_handle);
    return outcome;
}

static uint8_t hash_body(const uint8_t* data, uint16_t length, void* context)
{
    BodyHash* hash = (BodyHash*)context;
    if (length > (uint16_t)(65535u - hash->length)) return 0;
    hash->length += length;
    s64_sha256_update(&hash->sha, data, length);
    return 1;
}

static S64StandaloneResult request(S64HttpTransport* transport, const char* url,
    uint8_t allow_http, const char* method, BodyWriter writer, const void* source,
    S64UciDataSink sink, void* context, uint8_t test)
{
    S64StandaloneResult result;
    uint8_t digest[32];
    char challenge[33], signature[65];
    char* query;
    const char* value;
    uint8_t i;

    S64_DIRECT_TRACE(1u);
    if (!transport || !transport->execute) return S64_STANDALONE_INVALID_ARGUMENT;
    result = parse_url(url, allow_http);
    if (result != S64_STANDALONE_OK) return result;
    S64_DIRECT_TRACE(2u);
    if (!key_ready) return S64_STANDALONE_AUTH_FAILED;
    S64_DIRECT_TRACE(3u);
    query = strchr(uri, '?');
    if (!query || (uint16_t)(query - uri) + sizeof("?action=challenge") > 256u)
        return S64_STANDALONE_INVALID_ARGUMENT;

    if (test) strcpy(query, "?action=settings");
    body_hash.length = 0;
    s64_sha256_init(&body_hash.sha);
    if (writer && !writer(hash_body, &body_hash, source)) {
        result = S64_STANDALONE_INVALID_ARGUMENT;
        goto cleanup;
    }

    s64_sha256_final(&body_hash.sha, digest);
    S64_DIRECT_TRACE(4u);

    /* Persistent mode reuses the body-hash area for challenge/header scratch. */
    memcpy(challenge_uri, uri, (uint16_t)(query - uri));
    strcpy(challenge_uri + (query - uri), "?action=challenge");

    s64_net_init(&network, transport->execute);
    result = exchange("GET", challenge_uri, 0, 0, 0, 0, 0, collect_text, 0);
    if (result != S64_STANDALONE_OK) goto cleanup;
    S64_DIRECT_TRACE(5u);

    value = strstr(reply_text, "\"challenge\":\"");
    if (!value) {
        result = S64_STANDALONE_PROTOCOL_ERROR;
        goto cleanup;
    }
    S64_DIRECT_TRACE(6u);
    value += 13;
    for (i = 0; i < 32u; ++i) {
        if (!((value[i] >= '0' && value[i] <= '9')
            || (value[i] >= 'a' && value[i] <= 'f'))) {
            result = S64_STANDALONE_PROTOCOL_ERROR;
            goto cleanup;
        }
        challenge[i] = value[i];
    }
    if (value[32] != '"') {
        result = S64_STANDALONE_PROTOCOL_ERROR;
        goto cleanup;
    }
    challenge[32] = 0;
    S64_DIRECT_TRACE(7u);

    i = (uint8_t)s64_direct_sign(direct_key, challenge, method, uri, digest, signature);
    if (!i) {
        result = S64_STANDALONE_PROTOCOL_ERROR;
        goto cleanup;
    }
    S64_DIRECT_TRACE(8u);

    result = exchange(method, uri, challenge, signature, body_hash.length, writer, source, sink, context);
    S64_DIRECT_TRACE(9u);

cleanup:
    memset(signature, 0, sizeof(signature));
    return result;
}

static int write_document(S64DocumentSink sink, void* context, const void* source)
{
    return s64_document_write_wire((const S64Document*)source, sink, context) == S64_DOCUMENT_OK;
}

static S64StandaloneResult ack_result(S64StandaloneResult result)
{
    if (result == S64_STANDALONE_OK && !strstr(reply_text, "\"ok\":true"))
        return S64_STANDALONE_PROTOCOL_ERROR;
    return result;
}

S64StandaloneResult s64_direct_publisher_test(S64HttpTransport* transport,
    const char* url, uint8_t allow_http)
{
    return ack_result(request(transport, url, allow_http, "GET", 0, 0, collect_text, 0, 1));
}

#ifdef S64_PERSISTENT_RUNTIME_FIT
S64StandaloneResult s64_persistent_standalone_list_stage(
    S64HttpTransport* transport,
    const char* url,
    uint8_t allow_http,
    uint16_t index)
{
    uint16_t url_length = 0u;

    if (!transport || !url) return S64_STANDALONE_INVALID_ARGUMENT;
    reply_text[0] = 0;
    if (!append_text_bounded(reply_text, REPLY_TEXT_CAPACITY, &url_length, url)
        || !append_text_bounded(
            reply_text, REPLY_TEXT_CAPACITY, &url_length,
            "&format=summary&offset=")
        || !append_u16_decimal(
            reply_text, REPLY_TEXT_CAPACITY, &url_length, index)) {
        return S64_STANDALONE_INVALID_ARGUMENT;
    }
    return request(
        transport, reply_text, allow_http, "GET", 0, 0,
        collect_text, 0, 0);
}
#else
S64StandaloneResult s64_direct_list_summary(
    S64HttpTransport* transport,
    const char* url,
    uint8_t allow_http,
    S64DocumentType type,
    uint16_t index,
    S64DocumentSummary* summary,
    uint16_t* total_count)
{
    static const char prefix[] = "SEE64YOU-LIST/1\nCount: ";
    static const char slug_prefix[] = "Slug: ";
    static const char title_prefix[] = "Title: ";
    S64StandaloneResult result;
    uint16_t url_length = 0u;
    uint16_t count = 0u;
    uint16_t value_length;
    char* cursor;
    char* end;

    if (!transport || !url || !summary || !total_count
        || !s64_document_type_name(type)) {
        return S64_STANDALONE_INVALID_ARGUMENT;
    }

    reply_text[0] = 0;
    if (!append_text_bounded(reply_text, REPLY_TEXT_CAPACITY, &url_length, url)
        || !append_text_bounded(
            reply_text, REPLY_TEXT_CAPACITY, &url_length,
            "&format=summary&offset=")
        || !append_u16_decimal(
            reply_text, REPLY_TEXT_CAPACITY, &url_length, index)) {
        return S64_STANDALONE_INVALID_ARGUMENT;
    }

    result = request(
        transport, reply_text, allow_http, "GET", 0, 0,
        collect_text, 0, 0);
    if (result != S64_STANDALONE_OK) return result;

    cursor = reply_text;
    if (strncmp(cursor, prefix, sizeof(prefix) - 1u) != 0)
        return S64_STANDALONE_PROTOCOL_ERROR;
    cursor += sizeof(prefix) - 1u;
    if (*cursor < '0' || *cursor > '9')
        return S64_STANDALONE_PROTOCOL_ERROR;
    while (*cursor >= '0' && *cursor <= '9') {
        uint8_t digit = (uint8_t)(*cursor++ - '0');
        if (count > 6553u || (count == 6553u && digit > 5u))
            return S64_STANDALONE_PROTOCOL_ERROR;
        count = (uint16_t)(count * 10u + digit);
    }
    if (*cursor++ != '\n') return S64_STANDALONE_PROTOCOL_ERROR;
    *total_count = count;

    memset(summary, 0, sizeof(*summary));
    summary->type = type;
    summary->status = S64_DOCUMENT_DRAFT;

    if (index >= count) {
        return (*cursor == '\n' && cursor[1] == 0)
            ? S64_STANDALONE_OK
            : S64_STANDALONE_PROTOCOL_ERROR;
    }

    if (strncmp(cursor, slug_prefix, sizeof(slug_prefix) - 1u) != 0)
        return S64_STANDALONE_PROTOCOL_ERROR;
    cursor += sizeof(slug_prefix) - 1u;
    end = strchr(cursor, '\n');
    if (!end) return S64_STANDALONE_PROTOCOL_ERROR;
    value_length = (uint16_t)(end - cursor);
    if (!value_length || value_length >= sizeof(summary->slug))
        return S64_STANDALONE_PROTOCOL_ERROR;
    memcpy(summary->slug, cursor, value_length);
    summary->slug[value_length] = 0;
    if (!s64_document_slug_valid(summary->slug))
        return S64_STANDALONE_PROTOCOL_ERROR;

    cursor = end + 1;
    if (strncmp(cursor, title_prefix, sizeof(title_prefix) - 1u) != 0)
        return S64_STANDALONE_PROTOCOL_ERROR;
    cursor += sizeof(title_prefix) - 1u;
    end = strchr(cursor, '\n');
    if (!end) return S64_STANDALONE_PROTOCOL_ERROR;
    value_length = (uint16_t)(end - cursor);
    if (!value_length || value_length >= sizeof(summary->title)
        || end[1] != '\n' || end[2] != 0) {
        return S64_STANDALONE_PROTOCOL_ERROR;
    }
    memcpy(summary->title, cursor, value_length);
    summary->title[value_length] = 0;
    return S64_STANDALONE_OK;
}
#endif

#ifdef S64_PERSISTENT_RUNTIME_FIT
S64StandaloneResult s64_persistent_standalone_open_stage(
    S64HttpTransport* transport,
    const char* url,
    uint8_t allow_http,
    S64PersistentOpenStage* stage)
{
    S64StandaloneResult result;

    if (!stage || !stage->address || stage->complete
        || stage->result != S64_DOCUMENT_OK || stage->recovery_required)
        return S64_STANDALONE_INVALID_ARGUMENT;
    result = request(
        transport, url, allow_http, "GET", 0, 0,
        s64_persistent_open_stage_write, stage, 0);
    if (stage->result == S64_DOCUMENT_SINK_ERROR)
        return S64_STANDALONE_RESPONSE_TOO_LARGE;
    if (result != S64_STANDALONE_OK) {
        stage->result = S64_DOCUMENT_SINK_ERROR;
        return result;
    }
    return s64_persistent_open_stage_finish(stage)
        ? S64_STANDALONE_OK
        : S64_STANDALONE_PROTOCOL_ERROR;
}
#endif

S64StandaloneResult s64_standalone_save_document(S64HttpTransport* transport,
    const char* url, uint8_t allow_http, const S64StandaloneSession* session,
    const S64Document* document)
{
    S64StandaloneResult result;
    (void)session;
    result = request(transport, url, allow_http, "POST", write_document, document, collect_text, 0, 0);
    return ack_result(result);
}

#ifndef S64_PERSISTENT_RUNTIME_FIT
typedef struct {
    S64DocumentWireReader reader;
    S64Document* document;
    uint8_t started;
} OpenReply;

static uint8_t read_document(const uint8_t* data, uint16_t length, void* context)
{
    OpenReply* reply = (OpenReply*)context;
    if (!reply->started) {
        s64_document_wire_reader_init(&reply->reader, reply->document);
        reply->started = 1;
    }
    return s64_document_wire_reader_write(data, length, &reply->reader);
}

S64StandaloneResult s64_standalone_open_document(S64HttpTransport* transport,
    const char* url, uint8_t allow_http, const S64StandaloneSession* session,
    S64Document* document)
{
    OpenReply reply;
    S64StandaloneResult result;
    (void)session;
    if (!document || !document->body) return S64_STANDALONE_INVALID_ARGUMENT;
    reply.document = document; reply.started = 0;
    result = request(transport, url, allow_http, "GET", 0, 0,
        read_document, &reply, 0);
    if (result == S64_STANDALONE_OK && (!reply.started
        || s64_document_wire_reader_finish(&reply.reader) != S64_DOCUMENT_OK))
        return S64_STANDALONE_PROTOCOL_ERROR;
    return result;
}

#endif

typedef struct { const char* type; const char* slug; } DeleteBody;
static int write_delete(S64DocumentSink sink, void* context, const void* source)
{
    const DeleteBody* body = (const DeleteBody*)source;
    char json[112];
    uint16_t length = 0u;
    uint16_t i;
    uint8_t ascii;

    json[0] = 0;
    if (!append_text_bounded(json, sizeof(json), &length, "{\"type\":\"")
        || !append_text_bounded(json, sizeof(json), &length, body->type)
        || !append_text_bounded(json, sizeof(json), &length, "\",\"slug\":\"")
        || !append_text_bounded(json, sizeof(json), &length, body->slug)
        || !append_text_bounded(json, sizeof(json), &length, "\"}")) {
        return 0;
    }
    for (i = 0u; i < length; ++i) {
        if (!s64_charset_native_to_ascii((uint8_t)json[i], &ascii)) return 0;
        json[i] = (char)ascii;
    }
    return sink((const uint8_t*)json, length, context);
}

S64StandaloneResult s64_standalone_delete_document(S64HttpTransport* transport,
    const char* url, uint8_t allow_http, const S64StandaloneSession* session,
    S64DocumentType type, const char* slug)
{
    DeleteBody body;
    (void)session;
    body.type = s64_document_type_name(type); body.slug = slug;
    if (!body.type || !s64_document_slug_valid(slug)) return S64_STANDALONE_INVALID_ARGUMENT;
    return ack_result(request(transport, url, allow_http, "POST", write_delete, &body, collect_text, 0, 0));
}

#ifndef S64_PERSISTENT_RUNTIME_FIT
const char* s64_standalone_result_message(S64StandaloneResult result)
{
    switch (result) {
        case S64_STANDALONE_OK: return "OK";
        case S64_STANDALONE_AUTH_FAILED: return "Direct key missing or denied";
        case S64_STANDALONE_INSECURE_TRANSPORT: return "Enable plain HTTP";
        case S64_STANDALONE_TLS_UNSUPPORTED: return "Direct mode needs http:// URL";
        case S64_STANDALONE_TRANSPORT_ERROR: return "Network/UCI error";
        case S64_STANDALONE_SERVER_ERROR: return "Server rejected request";
        case S64_STANDALONE_INVALID_ARGUMENT: return "Invalid request/site URL";
        default: return "Unexpected server reply";
    }
}
#endif
