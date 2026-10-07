#include "publisher/persistent_mode.h"

#include <string.h>

void s64_persistent_publisher_init(
    S64PersistentPublisher* publisher,
    S64Document* document
)
{
    if (publisher == 0) return;
    memset(publisher, 0, sizeof(*publisher));
    publisher->document = document;
    publisher->publish_state.remote_type = S64_DOCUMENT_PAGE;
}

uint8_t s64_persistent_publisher_valid(
    const S64PersistentPublisher* publisher
)
{
    return publisher != 0
        && publisher->document != 0
        && publisher->document->body != 0
        && publisher->document_changed <= 1u
        && publisher->document_replaced <= 1u
        && publisher->network_available <= 1u;
}

void s64_persistent_publisher_mark_changed(
    S64PersistentPublisher* publisher,
    uint8_t replaced
)
{
    if (publisher == 0) return;
    publisher->document_changed = 1u;
    if (replaced) publisher->document_replaced = 1u;
}

void s64_persistent_publisher_reset_change_flags(
    S64PersistentPublisher* publisher
)
{
    if (publisher == 0) return;
    publisher->document_changed = 0u;
    publisher->document_replaced = 0u;
}
