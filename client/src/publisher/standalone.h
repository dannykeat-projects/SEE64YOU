#ifndef SEE64YOU_PUBLISHER_STANDALONE_H
#define SEE64YOU_PUBLISHER_STANDALONE_H

#include <stdint.h>

#include "platform/http.h"

#define S64_STANDALONE_TOKEN_CAPACITY 112u

typedef enum {
    S64_STANDALONE_OK = 0,
    S64_STANDALONE_INVALID_ARGUMENT,
    S64_STANDALONE_TRANSPORT_ERROR,
    S64_STANDALONE_AUTH_FAILED,
    S64_STANDALONE_INSECURE_TRANSPORT,
    S64_STANDALONE_TLS_UNSUPPORTED,
    S64_STANDALONE_SERVER_ERROR,
    S64_STANDALONE_RESPONSE_TOO_LARGE,
    S64_STANDALONE_PROTOCOL_ERROR
} S64StandaloneResult;

typedef struct {
    char token[S64_STANDALONE_TOKEN_CAPACITY];
    uint32_t expires;
} S64StandaloneSession;

void s64_standalone_session_clear(S64StandaloneSession* session);

/* Shared by standalone publisher operations; not UI commands. */
S64StandaloneResult s64_standalone_validate_transport(
    const char* url,
    uint8_t allow_insecure_http
);
S64StandaloneResult s64_standalone_add_authorization(
    S64HttpTransport* http,
    uint8_t header_handle,
    const S64StandaloneSession* session
);
S64StandaloneResult s64_standalone_exchange_ack(
    S64HttpTransport* http,
    uint8_t header_handle,
    uint8_t body_handle
);

/*
 * login_url is the complete UCI HTTP resource specifier including query.
 *
 * Current upstream firmware source parses https:// but does not negotiate TLS;
 * the HTTP request is sent over a plain TCP socket. For that reason:
 * - https:// is rejected until target hardware proves a secure implementation;
 * - http:// or bare host/path requires allow_insecure_http != 0 and is intended
 *   only for explicit trusted-LAN testing or a local compatibility relay.
 */
S64StandaloneResult s64_standalone_login(
    S64HttpTransport* http,
    const char* login_url,
    const char* password,
    uint8_t allow_insecure_http,
    S64StandaloneSession* session
);

#endif
