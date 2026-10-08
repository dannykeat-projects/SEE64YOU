#include "publisher/persistent_dispatch.h"

#include "publisher/destination.h"
#include "publisher/standalone_documents.h"
#include "publisher/state.h"

S64PersistentActionResult s64_persistent_publisher_save_direct(
    S64PersistentPublisher* publisher,
    S64HttpTransport* http,
    char* url,
    uint16_t url_capacity)
{
    S64DestinationResult destination_result;
    S64StandaloneResult standalone_result;
    uint8_t updating;

    if (!publisher || !s64_persistent_publisher_valid(publisher)
        || !http || !url || !url_capacity) {
        return S64_PERSISTENT_ACTION_INVALID;
    }
    if (!publisher->network_available
        || publisher->destination.type != S64_DESTINATION_STANDALONE
        || !publisher->publish_state.connected
        || !publisher->document->title[0]) {
        return S64_PERSISTENT_ACTION_NOT_READY;
    }

    updating = (uint8_t)s64_publish_state_can_update(
        &publisher->publish_state,
        &publisher->destination,
        publisher->document
    );
    destination_result = s64_destination_save_url(
        &publisher->destination,
        updating ? publisher->publish_state.remote_identity : 0,
        updating,
        url,
        url_capacity
    );
    if (destination_result != S64_DESTINATION_OK) {
        return S64_PERSISTENT_ACTION_URL_ERROR;
    }

    standalone_result = s64_standalone_save_document(
        http,
        url,
        publisher->destination.allow_insecure_http,
        0,
        publisher->document
    );
    if (standalone_result != S64_STANDALONE_OK) {
        return S64_PERSISTENT_ACTION_NETWORK_ERROR;
    }

    if (!s64_publish_state_bind_remote(
            &publisher->publish_state,
            &publisher->destination,
            publisher->document->type,
            publisher->document->slug)) {
        return S64_PERSISTENT_ACTION_BIND_ERROR;
    }

    return S64_PERSISTENT_ACTION_OK;
}

S64PersistentActionResult s64_persistent_publisher_delete_direct(
    S64PersistentPublisher* publisher,
    S64HttpTransport* http,
    char* url,
    uint16_t url_capacity)
{
    S64DestinationResult destination_result;
    S64StandaloneResult standalone_result;

    if (!publisher || !s64_persistent_publisher_valid(publisher)
        || !http || !url || !url_capacity) {
        return S64_PERSISTENT_ACTION_INVALID;
    }
    if (!publisher->network_available
        || publisher->destination.type != S64_DESTINATION_STANDALONE
        || !publisher->publish_state.connected
        || !s64_publish_state_can_delete(
            &publisher->publish_state, &publisher->destination)) {
        return S64_PERSISTENT_ACTION_NOT_READY;
    }

    destination_result = s64_destination_delete_url(
        &publisher->destination,
        publisher->publish_state.remote_type,
        publisher->publish_state.remote_identity,
        url,
        url_capacity
    );
    if (destination_result != S64_DESTINATION_OK) {
        return S64_PERSISTENT_ACTION_URL_ERROR;
    }

    standalone_result = s64_standalone_delete_document(
        http,
        url,
        publisher->destination.allow_insecure_http,
        0,
        publisher->publish_state.remote_type,
        publisher->publish_state.remote_identity
    );
    if (standalone_result != S64_STANDALONE_OK) {
        return S64_PERSISTENT_ACTION_NETWORK_ERROR;
    }

    s64_publish_state_clear_remote(&publisher->publish_state);
    if (publisher->document->status == S64_DOCUMENT_PUBLISHED) {
        publisher->document->status = S64_DOCUMENT_DRAFT;
        s64_persistent_publisher_mark_changed(publisher, 0u);
    }
    return S64_PERSISTENT_ACTION_OK;
}
