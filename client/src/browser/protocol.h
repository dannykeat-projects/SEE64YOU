#ifndef SEE64YOU_BROWSER_PROTOCOL_H
#define SEE64YOU_BROWSER_PROTOCOL_H

#include <stdint.h>

#define S64_BROWSER_URL_CAPACITY 320u
#define S64_BROWSER_HOST_CAPACITY 96u
#define S64_BROWSER_PATH_CAPACITY 224u
#define S64_BROWSER_LINE_CAPACITY 252u

typedef enum {
    S64_BROWSER_PROTOCOL_OK = 0,
    S64_BROWSER_PROTOCOL_INVALID,
    S64_BROWSER_PROTOCOL_TLS_UNSUPPORTED,
    S64_BROWSER_PROTOCOL_CALLBACK_FAILED
} S64BrowserProtocolResult;

/* All text at this boundary is ASCII, including on the C64 target. */
typedef struct {
    char host[S64_BROWSER_HOST_CAPACITY];
    char path[S64_BROWSER_PATH_CAPACITY];
    uint16_t port;
} S64BrowserUrl;

S64BrowserProtocolResult s64_browser_url_parse(const char* ascii, S64BrowserUrl* url);
/* base must be a successfully parsed URL. Failed calls leave output unspecified. */
S64BrowserProtocolResult s64_browser_url_resolve(
    const S64BrowserUrl* base, const char* link, S64BrowserUrl* url);
uint8_t s64_browser_url_format(const S64BrowserUrl* url, char* ascii, uint16_t capacity);

/* kind is ASCII T (text), L (link), or C (v2 colour triple). Pointers expire after callback. */
typedef uint8_t (*S64BrowserRecordSink)(
    uint8_t kind, const char* text, const char* href, void* context);
typedef struct {
    char line[S64_BROWSER_LINE_CAPACITY];
    uint16_t length;
    uint8_t header_seen, pending_cr, version, style_seen;
    S64BrowserProtocolResult result;
    S64BrowserRecordSink sink;
    void* context;
} S64BrowserParser;

void s64_browser_parser_init(S64BrowserParser* parser, S64BrowserRecordSink sink, void* context);
S64BrowserProtocolResult s64_browser_parser_feed(
    S64BrowserParser* parser, const uint8_t* bytes, uint16_t length);
S64BrowserProtocolResult s64_browser_parser_finish(S64BrowserParser* parser);

#endif
