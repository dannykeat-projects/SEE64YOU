#include "protocol.h"
#include <string.h>

static uint8_t lower(uint8_t c)
{
    return c >= 0x41u && c <= 0x5au ? (uint8_t)(c + 0x20u) : c;
}

static uint8_t scheme(const char* text, uint8_t tls)
{
    static const uint8_t http[] = {0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f};
    static const uint8_t https[] = {0x68,0x74,0x74,0x70,0x73,0x3a,0x2f,0x2f};
    const uint8_t* expected = tls ? https : http;
    uint8_t i, count = tls ? 8u : 7u;
    for (i = 0; i < count; ++i) {
        if (!text[i] || lower((uint8_t)text[i]) != expected[i]) return 0u;
    }
    return 1u;
}

static uint8_t safe_text(const char* text, uint16_t limit)
{
    uint16_t i;
    uint8_t c;
    for (i = 0; i < limit; ++i) {
        c = (uint8_t)text[i];
        if (!c) return 1u;
        if (c < 0x21u || c > 0x7eu || c == 0x23u || c == 0x5cu) return 0u;
    }
    return 0u;
}

/* RFC-style dot segments affect only the path, never query parameter bytes. */
static void normalize(char* path)
{
    uint16_t read = 1u, write = 1u, start, size, query;
    while (path[read] && (uint8_t)path[read] != 0x3fu) {
        start = read;
        while (path[read] && (uint8_t)path[read] != 0x2fu
            && (uint8_t)path[read] != 0x3fu) ++read;
        size = (uint16_t)(read - start);
        if (size == 1u && (uint8_t)path[start] == 0x2eu) {
            /* Leave the preceding separator in place. */
        } else if (size == 2u && (uint8_t)path[start] == 0x2eu
            && (uint8_t)path[start + 1u] == 0x2eu) {
            if (write > 1u) --write;
            while (write > 1u && (uint8_t)path[write - 1u] != 0x2fu) --write;
        } else {
            memmove(path + write, path + start, size);
            write += size;
            if ((uint8_t)path[read] == 0x2fu) path[write++] = (char)0x2f;
        }
        if ((uint8_t)path[read] == 0x2fu) ++read;
    }
    query = (uint16_t)strlen(path + read);
    memmove(path + write, path + read, query + 1u);
}

static S64BrowserProtocolResult parse_authority(const char* ascii, S64BrowserUrl* url)
{
    const char* p;
    const char* start;
    uint16_t length, port = 80u, value;
    uint8_t c;
    start = p = ascii;
    while (*p && (uint8_t)*p != 0x2fu && (uint8_t)*p != 0x3fu
        && (uint8_t)*p != 0x3au) {
        c = lower((uint8_t)*p);
        if (!((c >= 0x61u && c <= 0x7au) || (c >= 0x30u && c <= 0x39u)
            || c == 0x2eu || c == 0x2du)) return S64_BROWSER_PROTOCOL_INVALID;
        ++p;
    }
    length = (uint16_t)(p - start);
    if (!length || length >= S64_BROWSER_HOST_CAPACITY) return S64_BROWSER_PROTOCOL_INVALID;
    memcpy(url->host, start, length); url->host[length] = 0;
    if ((uint8_t)*p == 0x3au) {
        ++p; start = p; value = 0u;
        while ((uint8_t)*p >= 0x30u && (uint8_t)*p <= 0x39u) {
            c = (uint8_t)((uint8_t)*p - 0x30u);
            if (value > 6553u || (value == 6553u && c > 5u))
                return S64_BROWSER_PROTOCOL_INVALID;
            value = (uint16_t)(value * 10u + c); ++p;
        }
        if (p == start || !value) return S64_BROWSER_PROTOCOL_INVALID;
        port = value;
    }
    if (*p && (uint8_t)*p != 0x2fu && (uint8_t)*p != 0x3fu)
        return S64_BROWSER_PROTOCOL_INVALID;
    length = (uint16_t)strlen(p);
    if (length + ((uint8_t)*p == 0x2fu ? 0u : 1u) >= S64_BROWSER_PATH_CAPACITY)
        return S64_BROWSER_PROTOCOL_INVALID;
    url->path[0] = (char)0x2f;
    strcpy(url->path + ((uint8_t)*p == 0x2fu ? 0u : 1u), p);
    url->port = port;
    normalize(url->path);
    return S64_BROWSER_PROTOCOL_OK;
}

S64BrowserProtocolResult s64_browser_url_parse(const char* ascii, S64BrowserUrl* url)
{
    if (!ascii || !url || !safe_text(ascii, S64_BROWSER_URL_CAPACITY))
        return S64_BROWSER_PROTOCOL_INVALID;
    if (scheme(ascii, 1u)) return S64_BROWSER_PROTOCOL_TLS_UNSUPPORTED;
    if (!scheme(ascii, 0u)) return S64_BROWSER_PROTOCOL_INVALID;
    return parse_authority(ascii + 7u, url);
}

S64BrowserProtocolResult s64_browser_url_resolve(
    const S64BrowserUrl* base, const char* link, S64BrowserUrl* url)
{
    uint16_t prefix, length;
    if (!base || !link || !url || !safe_text(link, S64_BROWSER_URL_CAPACITY))
        return S64_BROWSER_PROTOCOL_INVALID;
    if (scheme(link, 0u) || scheme(link, 1u)) return s64_browser_url_parse(link, url);
    if ((uint8_t)link[0] == 0x2fu && (uint8_t)link[1] == 0x2fu) {
        if (strlen(link) + 5u >= S64_BROWSER_URL_CAPACITY) return S64_BROWSER_PROTOCOL_INVALID;
        return parse_authority(link + 2u, url);
    }
    /* A colon before the first path separator is another URI scheme. */
    for (prefix = 0u; link[prefix] && (uint8_t)link[prefix] != 0x2fu
        && (uint8_t)link[prefix] != 0x3fu; ++prefix) {
        if ((uint8_t)link[prefix] == 0x3au) return S64_BROWSER_PROTOCOL_INVALID;
    }
    *url = *base;
    if (!link[0]) return S64_BROWSER_PROTOCOL_OK;
    prefix = 0u;
    if ((uint8_t)link[0] != 0x2fu) {
        while (base->path[prefix] && (uint8_t)base->path[prefix] != 0x3fu) ++prefix;
        if ((uint8_t)link[0] != 0x3fu) {
            while (prefix && (uint8_t)base->path[prefix - 1u] != 0x2fu) --prefix;
        }
    }
    length = (uint16_t)strlen(link);
    if (prefix + length >= S64_BROWSER_PATH_CAPACITY) return S64_BROWSER_PROTOCOL_INVALID;
    memcpy(url->path, base->path, prefix);
    strcpy(url->path + prefix, link);
    normalize(url->path);
    return S64_BROWSER_PROTOCOL_OK;
}

uint8_t s64_browser_url_format(const S64BrowserUrl* url, char* ascii, uint16_t capacity)
{
    static const uint8_t http[] = {0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f};
    char digits[5];
    uint8_t count = 0u;
    uint16_t length, value;
    if (!url || !ascii || !capacity) return 0u;
    length = (uint16_t)(7u + strlen(url->host) + strlen(url->path));
    value = url->port;
    if (value != 80u) {
        do { digits[count++] = (char)(0x30u + value % 10u); value /= 10u; } while (value);
        length += (uint16_t)(count + 1u);
    }
    if (length >= capacity || length >= S64_BROWSER_URL_CAPACITY) return 0u;
    memcpy(ascii, http, 7u); strcpy(ascii + 7u, url->host);
    length = (uint16_t)strlen(ascii);
    if (count) {
        ascii[length++] = (char)0x3a;
        while (count) ascii[length++] = digits[--count];
    }
    strcpy(ascii + length, url->path);
    return 1u;
}

void s64_browser_parser_init(S64BrowserParser* parser, S64BrowserRecordSink sink, void* context)
{
    memset(parser, 0, sizeof(*parser)); parser->sink = sink; parser->context = context;
}

static S64BrowserProtocolResult record(S64BrowserParser* parser)
{
    static const uint8_t header[] = {0x53,0x45,0x45,0x36,0x34,0x59,0x4f,0x55,
        0x2d,0x42,0x52,0x4f,0x57,0x53,0x45,0x2f,0x31};
    char* separator;
    uint16_t href_length, i;
    parser->line[parser->length] = 0;
    if (!parser->header_seen) {
        if (parser->length != sizeof(header) || memcmp(parser->line, header, sizeof(header)))
            return S64_BROWSER_PROTOCOL_INVALID;
        parser->header_seen = 1u; return S64_BROWSER_PROTOCOL_OK;
    }
    if (parser->length < 3u || (uint8_t)parser->line[1] != 0x09u)
        return S64_BROWSER_PROTOCOL_INVALID;
    separator = strchr(parser->line + 2u, 0x09);
    if ((uint8_t)parser->line[0] == 0x54u) {
        if (separator || parser->length > 102u) return S64_BROWSER_PROTOCOL_INVALID;
        return parser->sink && parser->sink(0x54u, parser->line + 2u, 0, parser->context)
            ? S64_BROWSER_PROTOCOL_OK : S64_BROWSER_PROTOCOL_CALLBACK_FAILED;
    }
    if ((uint8_t)parser->line[0] != 0x4cu || !separator)
        return S64_BROWSER_PROTOCOL_INVALID;
    href_length = (uint16_t)(separator - (parser->line + 2u));
    if (!href_length || href_length > 190u || !separator[1] || strlen(separator + 1u) > 58u
        || strchr(separator + 1u, 0x09)) return S64_BROWSER_PROTOCOL_INVALID;
    *separator = 0;
    /* Keep unsupported URLs visible; selecting them is a separate operation. */
    for (i = 0u; i < href_length; ++i) {
        if ((uint8_t)parser->line[i + 2u] <= 0x20u
            || (uint8_t)parser->line[i + 2u] == 0x7cu)
            return S64_BROWSER_PROTOCOL_INVALID;
    }
    return parser->sink && parser->sink(0x4cu, separator + 1u, parser->line + 2u, parser->context)
        ? S64_BROWSER_PROTOCOL_OK : S64_BROWSER_PROTOCOL_CALLBACK_FAILED;
}

S64BrowserProtocolResult s64_browser_parser_feed(
    S64BrowserParser* parser, const uint8_t* bytes, uint16_t length)
{
    uint16_t i;
    uint8_t c;
    if (!parser || (!bytes && length)) return S64_BROWSER_PROTOCOL_INVALID;
    if (parser->result != S64_BROWSER_PROTOCOL_OK) return parser->result;
    for (i = 0u; i < length; ++i) {
        c = bytes[i];
        if (parser->pending_cr && c != 0x0au) goto invalid;
        if (c == 0x0du) { parser->pending_cr = 1u; continue; }
        if (c == 0x0au) {
            parser->pending_cr = 0u;
            parser->result = record(parser); parser->length = 0u;
            if (parser->result != S64_BROWSER_PROTOCOL_OK) return parser->result;
            continue;
        }
        if ((c < 0x20u && c != 0x09u) || c > 0x7eu
            || parser->length + 1u >= S64_BROWSER_LINE_CAPACITY) goto invalid;
        parser->line[parser->length++] = (char)c;
    }
    return S64_BROWSER_PROTOCOL_OK;
invalid:
    parser->result = S64_BROWSER_PROTOCOL_INVALID;
    return parser->result;
}

S64BrowserProtocolResult s64_browser_parser_finish(S64BrowserParser* parser)
{
    if (!parser) return S64_BROWSER_PROTOCOL_INVALID;
    if (parser->result == S64_BROWSER_PROTOCOL_OK
        && (!parser->header_seen || parser->length || parser->pending_cr))
        parser->result = S64_BROWSER_PROTOCOL_INVALID;
    return parser->result;
}
