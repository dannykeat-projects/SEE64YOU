#ifndef SEE64YOU_PUBLISHER_PERSISTENT_DISPATCH_H
#define SEE64YOU_PUBLISHER_PERSISTENT_DISPATCH_H

#include <stdint.h>

#include "platform/http.h"
#include "publisher/persistent_mode.h"
#include "publisher/persistent_ui.h"

S64PersistentPublisherAction s64_persistent_publisher_dispatch_key(
    S64PersistentPublisher* publisher,
    uint8_t key
);

typedef enum {
    S64_PERSISTENT_ACTION_OK = 0,
    S64_PERSISTENT_ACTION_INVALID,
    S64_PERSISTENT_ACTION_NOT_READY,
    S64_PERSISTENT_ACTION_URL_ERROR,
    S64_PERSISTENT_ACTION_NETWORK_ERROR,
    S64_PERSISTENT_ACTION_BIND_ERROR
} S64PersistentActionResult;

S64PersistentActionResult s64_persistent_publisher_save_direct(
    S64PersistentPublisher* publisher,
    S64HttpTransport* http,
    char* url,
    uint16_t url_capacity
);

S64PersistentActionResult s64_persistent_publisher_delete_direct(
    S64PersistentPublisher* publisher,
    S64HttpTransport* http,
    char* url,
    uint16_t url_capacity
);

#endif
