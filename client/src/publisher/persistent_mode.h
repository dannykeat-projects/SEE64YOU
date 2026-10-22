#ifndef SEE64YOU_PUBLISHER_PERSISTENT_MODE_H
#define SEE64YOU_PUBLISHER_PERSISTENT_MODE_H

#include <stdint.h>

#include "document.h"
#include "publisher/destination.h"
#include "publisher/state.h"

typedef struct {
    S64Document* document;
    S64PublishDestination destination;
    S64PublishState publish_state;
    uint8_t document_changed;
    uint8_t document_replaced;
    uint8_t network_available;
} S64PersistentPublisher;

void s64_persistent_publisher_init(
    S64PersistentPublisher* publisher,
    S64Document* document
);
uint8_t s64_persistent_publisher_valid(
    const S64PersistentPublisher* publisher
);
void s64_persistent_publisher_mark_changed(
    S64PersistentPublisher* publisher,
    uint8_t replaced
);
void s64_persistent_publisher_reset_change_flags(
    S64PersistentPublisher* publisher
);

#endif
