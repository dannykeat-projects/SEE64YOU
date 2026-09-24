#include "state.h"

#include <string.h>

static uint8_t wordpress_id_valid(const char* identity)
{
    uint16_t length = 0u;

    if (identity == 0 || identity[0] == '\0') return 0u;

    while (identity[length] != '\0') {
        if (length + 1u >= S64_PUBLISH_IDENTITY_CAPACITY
            || identity[length] < '0' || identity[length] > '9') {
            return 0u;
        }
        ++length;
    }

    return 1u;
}

static uint8_t document_type_valid(S64DocumentType type)
{
    return type == S64_DOCUMENT_PAGE || type == S64_DOCUMENT_POST;
}

void s64_publish_state_init(S64PublishState* state)
{
    if (state == 0) return;

    state->connected = 0u;
    state->remote_bound = 0u;
    state->remote_type = S64_DOCUMENT_PAGE;
    state->remote_identity[0] = '\0';
}

void s64_publish_state_set_connected(S64PublishState* state, uint8_t connected)
{
    if (state == 0) return;
    state->connected = connected ? 1u : 0u;
}

void s64_publish_state_clear_remote(S64PublishState* state)
{
    if (state == 0) return;

    state->remote_bound = 0u;
    state->remote_type = S64_DOCUMENT_PAGE;
    state->remote_identity[0] = '\0';
}

int s64_publish_state_bind_remote(
    S64PublishState* state,
    const S64PublishDestination* destination,
    S64DocumentType type,
    const char* identity
)
{
    uint16_t length = 0u;
    uint8_t valid;

    if (state == 0 || destination == 0 || !document_type_valid(type)) {
        return 0;
    }

    if (destination->type == S64_DESTINATION_STANDALONE) {
        valid = (uint8_t)s64_document_slug_valid(identity);
    } else if (destination->type == S64_DESTINATION_WORDPRESS) {
        valid = wordpress_id_valid(identity);
    } else {
        return 0;
    }

    if (!valid) return 0;

    while (identity[length] != '\0') {
        if (length + 1u >= S64_PUBLISH_IDENTITY_CAPACITY) {
            return 0;
        }
        ++length;
    }

    memcpy(state->remote_identity, identity, length + 1u);
    state->remote_type = type;
    state->remote_bound = 1u;
    return 1;
}

int s64_publish_state_can_update(
    const S64PublishState* state,
    const S64PublishDestination* destination,
    const S64Document* document
)
{
    if (state == 0 || destination == 0 || document == 0
        || !state->connected || !state->remote_bound
        || state->remote_type != document->type) {
        return 0;
    }

    if (destination->type == S64_DESTINATION_STANDALONE) {
        return strcmp(state->remote_identity, document->slug) == 0;
    }

    if (destination->type == S64_DESTINATION_WORDPRESS) {
        return wordpress_id_valid(state->remote_identity);
    }

    return 0;
}

int s64_publish_state_can_delete(
    const S64PublishState* state,
    const S64PublishDestination* destination
)
{
    if (state == 0 || destination == 0
        || !state->connected || !state->remote_bound) {
        return 0;
    }

    if (destination->type == S64_DESTINATION_STANDALONE) {
        return s64_document_slug_valid(state->remote_identity);
    }
    if (destination->type == S64_DESTINATION_WORDPRESS) {
        return wordpress_id_valid(state->remote_identity);
    }

    return 0;
}
