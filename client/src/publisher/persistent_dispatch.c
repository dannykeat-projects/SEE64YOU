#include "publisher/persistent_dispatch.h"

#include "publisher/state.h"

S64PersistentPublisherAction s64_persistent_publisher_dispatch_key(
    S64PersistentPublisher* publisher,
    uint8_t key)
{
    S64PersistentPublisherAction action;

    if (!publisher || !s64_persistent_publisher_valid(publisher)) {
        return S64_PUBLISH_UI_NONE;
    }

    action = s64_persistent_publisher_key(publisher, key);
    if (action == S64_PUBLISH_UI_TYPE_CHANGED) {
        s64_publish_state_clear_remote(&publisher->publish_state);
        return S64_PUBLISH_UI_NONE;
    }

    return action;
}
