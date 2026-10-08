#ifndef SEE64YOU_PUBLISHER_PERSISTENT_DISPATCH_H
#define SEE64YOU_PUBLISHER_PERSISTENT_DISPATCH_H

#include <stdint.h>

#include "publisher/persistent_mode.h"
#include "publisher/persistent_ui.h"

S64PersistentPublisherAction s64_persistent_publisher_dispatch_key(
    S64PersistentPublisher* publisher,
    uint8_t key
);

#endif
