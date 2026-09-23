#ifndef SEE64YOU_PLATFORM_HTTP_H
#define SEE64YOU_PLATFORM_HTTP_H

#include <stdint.h>

#include "uci.h"

#define S64_HTTP_RESPONSE_HEADER_CAPACITY 257u
#define S64_HTTP_NO_BODY 0xFFu

typedef enum {
    S64_HTTP_OK = 0,
    S64_HTTP_INVALID_ARGUMENT,
    S64_HTTP_TRANSPORT_ERROR,
    S64_HTTP_TARGET_ERROR
} S64HttpResult;

typedef enum {
    S64_HTTP_GET = 0x01u,
    S64_HTTP_PUT = 0x02u,
    S64_HTTP_POST = 0x03u,
    S64_HTTP_PATCH = 0x04u,
    S64_HTTP_DELETE = 0x05u,
    S64_HTTP_HEAD = 0x06u,
    S64_HTTP_OPTIONS = 0x07u,
    S64_HTTP_CONNECT = 0x08u,
    S64_HTTP_TRACE = 0x09u
} S64HttpVerb;

typedef enum {
    S64_HTTP_BODY_BINARY = 0x01u,
    S64_HTTP_BODY_JSON_OBJECT = 0x02u,
    S64_HTTP_BODY_JSON_ARRAY = 0x03u,
    S64_HTTP_BODY_URL_ENCODED = 0x04u
} S64HttpBodyFormat;

typedef struct {
    S64UciExecuteFn execute;
} S64HttpTransport;

void s64_http_init(S64HttpTransport* transport, S64UciExecuteFn execute);

S64HttpResult s64_http_free_all(S64HttpTransport* transport);
S64HttpResult s64_http_header_create(
    S64HttpTransport* transport,
    S64HttpVerb verb,
    const char* url,
    uint8_t* handle
);
S64HttpResult s64_http_header_add(
    S64HttpTransport* transport,
    uint8_t handle,
    const char* header_line
);
S64HttpResult s64_http_header_free(S64HttpTransport* transport, uint8_t handle);

S64HttpResult s64_http_body_create(
    S64HttpTransport* transport,
    S64HttpBodyFormat format,
    uint8_t* handle
);
S64HttpResult s64_http_body_add_string(
    S64HttpTransport* transport,
    uint8_t handle,
    const char* key,
    const char* value
);
S64HttpResult s64_http_body_add_binary(
    S64HttpTransport* transport,
    uint8_t handle,
    const uint8_t* data,
    uint16_t length
);
S64HttpResult s64_http_body_free(S64HttpTransport* transport, uint8_t handle);

S64HttpResult s64_http_exchange_raw(
    S64HttpTransport* transport,
    uint8_t header_handle,
    uint8_t body_handle,
    S64UciDataSink body_sink,
    void* body_context,
    char* response_header,
    uint16_t response_header_capacity,
    uint16_t* response_header_length
);

#endif
