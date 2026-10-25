#ifndef SEE64YOU_PUBLISHER_STATE_H
#define SEE64YOU_PUBLISHER_STATE_H

#include <stdint.h>

#include "destination.h"

#define S64_PUBLISH_IDENTITY_CAPACITY 65u

typedef struct {
    uint8_t connected;
    uint8_t remote_bound;
    S64DocumentType remote_type;
    char remote_identity[S64_PUBLISH_IDENTITY_CAPACITY];
#ifdef S64_INDEPENDENT_PUBLISHER
    char remote_revision[33];
#endif
} S64PublishState;

void s64_publish_state_init(S64PublishState* state);
void s64_publish_state_set_connected(S64PublishState* state, uint8_t connected);
void s64_publish_state_clear_remote(S64PublishState* state);

int s64_publish_state_bind_remote(
    S64PublishState* state,
    const S64PublishDestination* destination,
    S64DocumentType type,
    const char* identity
);

int s64_publish_state_can_update(
    const S64PublishState* state,
    const S64PublishDestination* destination,
    const S64Document* document
);

int s64_publish_state_can_delete(
    const S64PublishState* state,
    const S64PublishDestination* destination
);

#endif
