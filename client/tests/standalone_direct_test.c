#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "publisher/standalone_direct.h"
#include "publisher/standalone_documents.h"
#include "publisher/direct_auth.h"
#include "publisher/workspace.h"

/* Exercise the private streaming request with bodies larger than a disk source. */
#include "../src/publisher/standalone_direct.c"

static const char test_key[] =
    "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
static const char challenge[] = "0123456789abcdef0123456789abcdef";
static char sent[125000], response[8192], wire[6000];
static unsigned int sent_length, response_offset, response_length, wire_length;
static unsigned int opens, closes, signed_requests;
static unsigned int failure_mode, empty_read;
static uint8_t large_body_mode;
static uint32_t large_body_length;

static void status(S64UciReply* reply, const char* value)
{
    strcpy(reply->status, value); reply->status_length = (uint16_t)strlen(value);
}

static void make_response(void)
{
    char method[16], path[256], signature[65];
    char* body = strstr(sent, "\r\n\r\n");
    char* header;
    uint8_t key[32], digest[32];
    S64Sha256 sha;
    const char* payload = "{\"ok\":true}";
    unsigned int code = 200, advertised;
    assert(body); body += 4;
    assert(sscanf(sent, "%15s %255s", method, path) == 2);
    if (strstr(path, "action=challenge")) {
        payload = "{\"ok\":true,\"challenge\":\"0123456789abcdef0123456789abcdef\"}";
        assert(!strstr(sent, "X-See64You-Signature"));
    } else {
        ++signed_requests;
        assert(!strstr(sent, test_key));
        assert(strstr(sent, "\r\nX-See64You-Challenge: 0123456789abcdef0123456789abcdef\r\n"));
        header = strstr(sent, "\r\nX-See64You-Signature: "); assert(header);
        assert(s64_direct_key_parse(test_key, key));
        s64_sha256_init(&sha);
        {
            size_t offset = 0, length = strlen(body);
            while (offset < length) {
                uint16_t chunk = length - offset > 1024u ? 1024u : (uint16_t)(length - offset);
                s64_sha256_update(&sha, (const uint8_t*)body + offset, chunk);
                offset += chunk;
            }
        }
        s64_sha256_final(&sha, digest);
        assert(s64_direct_sign(key, challenge, method, path, digest, signature));
        assert(memcmp(header + strlen("\r\nX-See64You-Signature: "), signature, 64) == 0);
        if (strcmp(method, "POST") == 0) {
            unsigned long length;
            header = strstr(sent, "Content-Length: "); assert(header);
            assert(sscanf(header, "Content-Length: %lu", &length) == 1);
            assert(length == strlen(body));
            if (large_body_mode) {
                size_t i;
                assert(length == large_body_length);
                assert(strstr(sent, "Content-Type: application/json\r\n"));
                for (i = 0; i < length; ++i) assert(body[i] == (char)('a' + i % 26u));
            } else if (strstr(path, "action=save")) {
                assert(strstr(sent, "Content-Type: application/x-see64you\r\n"));
                assert(strcmp(body, wire) == 0);
            } else {
                assert(strstr(sent, "Content-Type: application/json\r\n"));
                assert(strcmp(body, "{\"type\":\"page\",\"slug\":\"test-page\"}") == 0);
            }
        }
        if (strstr(path, "action=get")) payload = wire;
        if (failure_mode == 1) code = 401;
        if (failure_mode == 2) code = 302;
        if (failure_mode == 5) payload = "{\"ok\":false}";
    }
    advertised = (unsigned int)strlen(payload);
    if (failure_mode == 3 && !strstr(path, "action=challenge")) advertised += 3;
    sprintf(response, "HTTP/1.0 %u Test\r\nContent-Length: %u\r\n\r\n%s", code, advertised, payload);
    if (failure_mode == 4 && !strstr(path, "action=challenge"))
        strcpy(response, "HTTP/1.0 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n");
    response_length = (unsigned int)strlen(response);
}

static S64UciResult execute(const S64UciCommandPart* parts, uint8_t count, S64UciReply* reply)
{
    uint8_t handle = 1, pair[2], packet[9];
    unsigned int length;
    assert(parts[0].data[0] == 3);
    switch (parts[0].data[1]) {
        case 7:
            assert(count == 3 && parts[0].data[2] == 80 && parts[0].data[3] == 0);
            assert(parts[1].length == strlen("example.com"));
            assert(memcmp(parts[1].data, "example.com", parts[1].length) == 0);
            ++opens; sent_length = response_offset = response_length = 0;
            sent[0] = 0; empty_read = 1;
            assert(reply->data_sink(&handle, 1, reply->data_context));
            break;
        case 17:
            assert(count == 2);
            length = parts[1].length;
            assert(sent_length + length < sizeof(sent));
            memcpy(sent + sent_length, parts[1].data, length);
            sent_length += length; sent[sent_length] = 0;
            pair[0] = (uint8_t)length; pair[1] = (uint8_t)(length >> 8);
            assert(reply->data_sink(pair, 2, reply->data_context));
            break;
        case 16:
            if (empty_read) { empty_read = 0; status(reply, "02,NO DATA"); return S64_UCI_OK; }
            if (!response_length) make_response();
            length = response_length - response_offset;
            if (length > 7) length = 7;
            packet[0] = (uint8_t)length; packet[1] = 0;
            memcpy(packet + 2, response + response_offset, length); response_offset += length;
            if (!reply->data_sink(packet, (uint16_t)(length + 2), reply->data_context))
                return S64_UCI_CONSUMER_ABORTED;
            status(reply, length ? "00,OK" : "01,CLOSED"); return S64_UCI_OK;
        case 9: ++closes; break;
        default: assert(0);
    }
    status(reply, "00,OK"); return S64_UCI_OK;
}

static uint8_t collect_wire(const uint8_t* data, uint16_t length, void* context)
{
    (void)context;
    assert(wire_length + length < sizeof(wire));
    memcpy(wire + wire_length, data, length); wire_length += length; wire[wire_length] = 0;
    return 1;
}

static int write_large_body(S64DocumentSink sink, void* context, const void* source)
{
    uint8_t chunk[257];
    uint32_t offset = 0, length = *(const uint32_t*)source;
    while (offset < length) {
        uint16_t i, amount = length - offset > sizeof(chunk)
            ? sizeof(chunk) : (uint16_t)(length - offset);
        for (i = 0; i < amount; ++i) chunk[i] = (uint8_t)('a' + (offset + i) % 26u);
        if (!sink(chunk, amount, context)) return 0;
        offset += amount;
    }
    return 1;
}

int main(void)
{
    S64HttpTransport transport;
    S64StandaloneSession session;
    S64Buffer buffer, reopened;
    S64Document document, loaded;
    unsigned int i, before;
    char long_url[300];
    transport.execute = execute;
    assert(s64_direct_publisher_key(test_key));
    before = opens;
    assert(s64_direct_publisher_test(&transport, "https://example.com/api.php?action=login", 1)
        == S64_STANDALONE_TLS_UNSUPPORTED);
    assert(s64_direct_publisher_test(&transport, "http://example.com/api.php?action=login", 0)
        == S64_STANDALONE_INSECURE_TRANSPORT);
    assert(s64_direct_publisher_test(&transport, "http://user@example.com/api.php?action=login", 1)
        == S64_STANDALONE_INVALID_ARGUMENT);
    assert(opens == before);
    strcpy(long_url, "http://example.com/");
    memset(long_url + strlen(long_url), 'a', 239);
    strcpy(long_url + 258, "?x");
    assert(s64_direct_publisher_test(&transport, long_url, 1) == S64_STANDALONE_INVALID_ARGUMENT);
    assert(opens == before);
    /* Exercise the UI's shared URL workspace, which response parsing overwrites. */
    strcpy((char*)s64_publisher_workspace.bytes, "http://example.com/api.php?action=login");
    assert(s64_direct_publisher_test(&transport, (char*)s64_publisher_workspace.bytes, 1) == S64_STANDALONE_OK);
    s64_buffer_init(&buffer); s64_document_init(&document, &buffer);
    assert(s64_document_set_slug(&document, "test-page"));
    assert(s64_document_set_title(&document, "C64U test"));
    for (i = 0; i < 2000; ++i) assert(s64_buffer_insert(&buffer, (unsigned char)('a' + i % 26)));
    assert(s64_document_write_wire(&document, collect_wire, 0) == S64_DOCUMENT_OK);
    assert(s64_standalone_save_document(&transport, "http://example.com/api.php?action=save&ack=1", 1,
        &session, &document) == S64_STANDALONE_OK);
    large_body_mode = 1;
    {
        static const uint32_t lengths[] = {0u, 65534u, 65535u, 65536u, 122880u, 122881u};
        for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); ++i) {
            large_body_length = lengths[i];
            assert(request(&transport, "http://example.com/api.php?action=save&ack=1", 1,
                "POST", write_large_body, &large_body_length, collect_text, 0, 0) == S64_STANDALONE_OK);
        }
    }
    large_body_mode = 0;
    assert(build_content_header(UINT32_MAX, 0));
    assert(strstr(line, "Content-Length: 4294967295\r\n"));
    body_hash.length = UINT32_MAX;
    assert(!hash_body((const uint8_t*)"x", 1u, &body_hash));
    assert(body_hash.length == UINT32_MAX);
    {
        char decimal[11];
        uint16_t length = 0;
        assert(append_decimal(decimal, sizeof(decimal), &length, UINT32_MAX));
        assert(strcmp(decimal, "4294967295") == 0);
        length = 0;
        assert(!append_decimal(decimal, sizeof(decimal) - 1u, &length, UINT32_MAX));
        assert(length == 0);
    }
    s64_buffer_init(&reopened); s64_document_init(&loaded, &reopened);
    assert(s64_standalone_open_document(&transport, "http://example.com/api.php?action=get&type=page&slug=test-page&format=s64",
        1, &session, &loaded) == S64_STANDALONE_OK);
    assert(strcmp(loaded.slug, document.slug) == 0 && s64_buffer_length(&reopened) == 2000);
    for (i = 0; i < 2000; ++i) assert(s64_buffer_char_at(&reopened, i) == s64_buffer_char_at(&buffer, i));
    assert(s64_standalone_delete_document(&transport, "http://example.com/api.php?action=delete", 1,
        &session, S64_DOCUMENT_PAGE, "test-page") == S64_STANDALONE_OK);
    document.type = S64_DOCUMENT_POST;
    wire_length = 0;
    assert(s64_document_write_wire(&document, collect_wire, 0) == S64_DOCUMENT_OK);
    assert(s64_standalone_save_document(&transport, "http://example.com/api.php?action=save&ack=1", 1,
        &session, &document) == S64_STANDALONE_OK);
    failure_mode = 1;
    assert(s64_standalone_open_document(&transport, "http://example.com/api.php?action=get&type=page&slug=test-page&format=s64",
        1, &session, &loaded) == S64_STANDALONE_AUTH_FAILED);
    assert(s64_buffer_length(&reopened) == 2000); /* Error body was never applied. */
    failure_mode = 2;
    assert(s64_direct_publisher_test(&transport, "http://example.com/api.php?action=login", 1) == S64_STANDALONE_SERVER_ERROR);
    failure_mode = 3;
    assert(s64_direct_publisher_test(&transport, "http://example.com/api.php?action=login", 1) == S64_STANDALONE_PROTOCOL_ERROR);
    failure_mode = 4;
    assert(s64_direct_publisher_test(&transport, "http://example.com/api.php?action=login", 1) == S64_STANDALONE_PROTOCOL_ERROR);
    failure_mode = 5;
    assert(s64_direct_publisher_test(&transport, "http://example.com/api.php?action=login", 1) == S64_STANDALONE_PROTOCOL_ERROR);
    before = opens;
    assert(!s64_direct_publisher_key(""));
    assert(s64_direct_publisher_test(&transport, "http://example.com/api.php?action=login", 1) == S64_STANDALONE_AUTH_FAILED);
    assert(opens == before);
    assert(opens == closes && signed_requests == 16);
    puts("standalone direct tests passed");
    return 0;
}
