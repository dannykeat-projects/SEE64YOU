#include "browser/http.h"
#include "charset.h"
#include <string.h>

typedef struct {
    char line[256];
    uint16_t length, header_bytes, remaining, body_bytes, status;
    uint8_t have_status, headers, have_length, have_type, failed, cr;
    S64UciDataSink sink;
    void* context;
} Reply;

static uint8_t prefix(const char* text, const char* expected)
{
    uint8_t a, b;
    while (*expected) {
        a = (uint8_t)*text++; b = (uint8_t)*expected++;
        if (a >= 'A' && a <= 'Z') a = a - 'A' + 'a';
        if (b >= 'A' && b <= 'Z') b = b - 'A' + 'a';
        if (a != b) return 0;
    }
    return 1;
}

static uint8_t header(Reply* reply)
{
    const char* value;
    uint16_t number;
    uint8_t i;
    reply->line[reply->length] = 0;
    if (!reply->have_status) {
        if (reply->length < 12u || (!prefix(reply->line, "HTTP/1.0 ")
            && !prefix(reply->line, "HTTP/1.1 "))) return 0;
        number = 0;
        for (i = 9; i < 12; ++i) {
            if (reply->line[i] < '0' || reply->line[i] > '9') return 0;
            number = number * 10u + (uint8_t)(reply->line[i] - '0');
        }
        if (reply->length > 12u && reply->line[12] != ' ') return 0;
        reply->status = number; reply->have_status = 1;
    } else if (!reply->length) {
        if (reply->status == 200u && !reply->have_type) return 0;
        reply->headers = 1;
    } else if (prefix(reply->line, "Content-Length:")) {
        if (reply->have_length) return 0;
        value = reply->line + 15;
        while (*value == ' ') ++value;
        if (*value < '0' || *value > '9') return 0;
        number = 0;
        while (*value >= '0' && *value <= '9') {
            if (number > 6553u || (number == 6553u && *value > '5')) return 0;
            number = number * 10u + (uint8_t)(*value++ - '0');
        }
        if (*value) return 0;
        reply->have_length = 1; reply->remaining = number;
    } else if (prefix(reply->line, "Content-Type:")) {
        if (reply->status != 200u) { reply->length = 0; return 1; }
        if (reply->have_type) return 0;
        value = reply->line + 13;
        while (*value == ' ') ++value;
        if (!prefix(value, "application/x-see64you-browser")) return 0;
        value += 30;
        if (*value && *value != ';' && *value != ' ') return 0;
        reply->have_type = 1;
    } else if (prefix(reply->line, "Transfer-Encoding:")
        || prefix(reply->line, "Content-Encoding:")) return 0;
    reply->length = 0;
    return 1;
}

static uint8_t receive(const uint8_t* data, uint16_t length, void* context)
{
    Reply* reply = (Reply*)context;
    uint16_t i = 0, amount;
    uint8_t native;
    while (i < length && !reply->headers) {
        if (++reply->header_bytes > 4096u) goto fail;
        if (reply->cr && data[i] != 10u) goto fail;
        if (data[i] == 13u) { reply->cr = 1; ++i; continue; }
        if (data[i] == 10u) {
            reply->cr = 0;
            ++i;
            if (!header(reply)) goto fail;
        } else {
            if (data[i] < 32u || data[i] > 126u
                || reply->length >= sizeof(reply->line) - 1u
                || !s64_charset_ascii_to_native(data[i++], &native)) goto fail;
            reply->line[reply->length++] = (char)native;
        }
    }
    amount = length - i;
    if (amount) {
        if (amount > 65535u - reply->body_bytes
            || (reply->have_length && amount > reply->remaining)) goto fail;
        reply->body_bytes += amount;
        if (reply->have_length) reply->remaining -= amount;
        if (reply->status == 200u && !reply->sink(data + i, amount, reply->context)) goto fail;
    }
    return 1;
fail:
    reply->failed = 1;
    return 0;
}

static uint8_t send(S64NetTransport* network, uint8_t handle, const char* text)
{
    return s64_net_write_text(network, handle, text, (uint16_t)strlen(text)) == S64_NET_OK;
}

S64BrowserHttpResult s64_browser_http_get(S64NetTransport* network,
    const char* host, uint16_t port, const char* path, S64UciDataSink sink, void* context,
    S64BrowserWait wait, void* wait_context)
{
    /* cc65 stack offsets are limited; this service has one synchronous request. */
    static Reply reply;
    uint8_t handle;
    uint16_t idle = 0, received;
    char port_text[7];
    uint8_t digit = 6;
    uint16_t port_value = port;
    S64NetResult result;
    S64BrowserHttpResult outcome = S64_BROWSER_HTTP_NETWORK;
    if (!network || !host || !*host || !port || !path || !sink || strpbrk(host, "\r\n @:#?\t")
        || path[0] != '/' || strpbrk(path, "\r\n \t#")) return S64_BROWSER_HTTP_PROTOCOL;
    if (s64_net_open_tcp(network, host, port, &handle) != S64_NET_OK) return outcome;
    memset(&reply, 0, sizeof(reply)); reply.sink = sink; reply.context = context;
    if (!send(network, handle, "GET ") || !send(network, handle, path)
        || !send(network, handle, " HTTP/1.0\r\nHost: ") || !send(network, handle, host)) goto cleanup;
    if (port != 80u) {
        port_text[digit] = 0;
        do { port_text[--digit] = (char)('0' + port_value % 10u); port_value /= 10u; } while (port_value);
        port_text[--digit] = ':';
        if (!send(network, handle, port_text + digit)) goto cleanup;
    }
    if (!send(network, handle, "\r\nAccept: application/x-see64you-browser; version=2\r\nConnection: close\r\n\r\n")) goto cleanup;
    while (idle < 512u) {
        if (wait && !wait(wait_context)) { outcome = S64_BROWSER_HTTP_CANCELLED; goto cleanup; }
        received = 0;
        result = s64_net_read(network, handle, S64_NET_READ_MAX, receive, &reply, &received);
        if (reply.failed) { outcome = S64_BROWSER_HTTP_PROTOCOL; goto cleanup; }
        if (result != S64_NET_OK && result != S64_NET_NO_DATA && result != S64_NET_CLOSED) goto cleanup;
        if (received) idle = 0; else ++idle;
        if ((reply.headers && reply.have_length && !reply.remaining) || result == S64_NET_CLOSED) break;
    }
    if (!reply.headers || idle == 512u || (reply.have_length && reply.remaining))
        outcome = S64_BROWSER_HTTP_PROTOCOL;
    else outcome = reply.status == 200u ? S64_BROWSER_HTTP_OK : S64_BROWSER_HTTP_SERVER;
cleanup:
    if (s64_net_close(network, handle) != S64_NET_OK) return S64_BROWSER_HTTP_CLOSE;
    return outcome;
}
