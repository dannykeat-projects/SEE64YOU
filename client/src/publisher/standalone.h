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
    S64_STANDALONE_SERVER_ERROR,
    S64_STANDALONE_RESPONSE_TOO_LARGE,
    S64_STANDALONE_PROTOCOL_ERROR
} S64StandaloneResult;

typedef struct {
    char token[S64_STANDALONE_TOKEN_CAPACITY];
    uint32_t expires;
} S64StandaloneSession;

void s64_standalone_session_clear(S64StandaloneSession* session);

/*
 * login_url is the complete UCI HTTP resource specifier, including host and
 * query string, for example:
 *   example.test/see64you/api.php?action=login
 */
S64StandaloneResult s64_standalone_login(
    S64HttpTransport* http,
    const char* login_url,
    const char* password,
    S64StandaloneSession* session
);

#endif
