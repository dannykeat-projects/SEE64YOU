#ifndef S64_BROWSER_HTTP_H
#define S64_BROWSER_HTTP_H

#include "platform/network.h"

typedef enum {
    S64_BROWSER_HTTP_OK = 0,
    S64_BROWSER_HTTP_NETWORK,
    S64_BROWSER_HTTP_PROTOCOL,
    S64_BROWSER_HTTP_SERVER,
    S64_BROWSER_HTTP_CANCELLED,
    S64_BROWSER_HTTP_CLOSE
} S64BrowserHttpResult;

/* Called between bounded reads; returning zero cancels and closes the socket. */
typedef uint8_t (*S64BrowserWait)(void* context);

/* Host/path use native text; the body sink receives wire ASCII. Synchronous. */
S64BrowserHttpResult s64_browser_http_get(S64NetTransport* network,
    const char* host, uint16_t port, const char* path, S64UciDataSink sink, void* context,
    S64BrowserWait wait, void* wait_context);

#endif
