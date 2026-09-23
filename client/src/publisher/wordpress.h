#ifndef SEE64YOU_PUBLISHER_WORDPRESS_H
#define SEE64YOU_PUBLISHER_WORDPRESS_H

#include <stdint.h>

#include "platform/http.h"

#define S64_WORDPRESS_AUTH_CAPACITY 192u

typedef enum {
    S64_WORDPRESS_OK = 0,
    S64_WORDPRESS_INVALID_ARGUMENT,
    S64_WORDPRESS_TRANSPORT_ERROR,
    S64_WORDPRESS_AUTH_FAILED,
    S64_WORDPRESS_PERMISSION_DENIED,
    S64_WORDPRESS_PLUGIN_UNAVAILABLE,
    S64_WORDPRESS_NOT_FOUND,
    S64_WORDPRESS_INCOMPATIBLE_DOCUMENT,
    S64_WORDPRESS_INSECURE_TRANSPORT,
    S64_WORDPRESS_TLS_UNSUPPORTED,
    S64_WORDPRESS_SERVER_ERROR,
    S64_WORDPRESS_RESPONSE_TOO_LARGE,
    S64_WORDPRESS_PROTOCOL_ERROR
} S64WordPressResult;

typedef struct {
    char authorization[S64_WORDPRESS_AUTH_CAPACITY];
} S64WordPressSession;

void s64_wordpress_session_clear(S64WordPressSession* session);

/*
 * Builds an RFC 7617 Basic Authorization header from a WordPress username and
 * Application Password. Spaces in the Application Password are ignored, which
 * matches WordPress's grouped display format. The plain credentials are not
 * retained in the session.
 */
int s64_wordpress_session_configure(
    S64WordPressSession* session,
    const char* username,
    const char* application_password
);

const char* s64_wordpress_result_message(S64WordPressResult result);

/* Shared by WordPress publisher operations; not UI commands. */
S64WordPressResult s64_wordpress_map_http(S64HttpResult result);
S64WordPressResult s64_wordpress_validate_transport(
    const char* url,
    uint8_t allow_insecure_http
);
uint16_t s64_wordpress_http_status(const char* header);

S64WordPressResult s64_wordpress_add_authorization(
    S64HttpTransport* http,
    uint8_t header_handle,
    const S64WordPressSession* session
);

/*
 * connection_url should point to /wp-json/see64you/v1/connection, directly on
 * a trusted LAN or through the See64You relay. Current C64U firmware does not
 * provide verified TLS, so direct https:// is rejected.
 */
S64WordPressResult s64_wordpress_test_connection(
    S64HttpTransport* http,
    const char* connection_url,
    uint8_t allow_insecure_http,
    const S64WordPressSession* session
);

#endif
