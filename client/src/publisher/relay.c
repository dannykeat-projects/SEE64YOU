#include "relay.h"

#include <string.h>

#define S64_RELAY_HEADER_CAPACITY (S64_RELAY_KEY_CAPACITY + 26u)

#define relay_header ((char*)&s64_publisher_workspace.bytes[S64_PUBLISH_WORK_RELAY_OFFSET])
static char relay_key[S64_RELAY_KEY_CAPACITY];

typedef char S64RelayWorkspaceFits[
    (S64_PUBLISH_WORK_RELAY_OFFSET + S64_RELAY_HEADER_CAPACITY
        <= S64_PUBLISH_WORKSPACE_CAPACITY) ? 1 : -1
];

static uint8_t is_hex_char(char value)
{
    return (value >= '0' && value <= '9')
        || (value >= 'a' && value <= 'f')
        || (value >= 'A' && value <= 'F');
}

int s64_relay_set_key(const char* value)
{
    uint16_t i;

    if (value == 0 || value[0] == '\0') {
        relay_key[0] = '\0';
        return 1;
    }

    for (i = 0u; i < 64u; ++i) {
        if (value[i] == '\0' || !is_hex_char(value[i])) {
            return 0;
        }
    }
    if (value[64] != '\0') {
        return 0;
    }

    memcpy(relay_key, value, 65u);
    return 1;
}

S64HttpResult s64_relay_add_header(
    S64HttpTransport* http,
    uint8_t header_handle
)
{
    static const char prefix[] = "X-See64You-Relay-Key: ";
    uint16_t prefix_length = (uint16_t)(sizeof(prefix) - 1u);

    if (http == 0) {
        return S64_HTTP_INVALID_ARGUMENT;
    }
    if (relay_key[0] == '\0') {
        return S64_HTTP_OK;
    }

    memcpy(relay_header, prefix, prefix_length);
    memcpy(relay_header + prefix_length, relay_key, 65u);

    return s64_http_header_add(http, header_handle, relay_header);
}
