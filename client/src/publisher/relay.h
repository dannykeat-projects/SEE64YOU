#ifndef SEE64YOU_PUBLISHER_RELAY_H
#define SEE64YOU_PUBLISHER_RELAY_H

#include <stdint.h>

#include "platform/http.h"

#define S64_RELAY_KEY_CAPACITY 65u

/*
 * Configure the optional trusted-LAN relay key. Pass NULL or an empty string
 * to disable relay authentication. Non-empty keys must be exactly 64 hex
 * characters (32 random bytes).
 */
int s64_relay_set_key(const char* relay_key);

/*
 * Add relay authentication to an HTTP request header object. With no relay key
 * configured this is a no-op, so direct trusted-LAN requests remain possible.
 */
S64HttpResult s64_relay_add_header(
    S64HttpTransport* http,
    uint8_t header_handle
);

#endif
