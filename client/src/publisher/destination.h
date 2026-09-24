#ifndef SEE64YOU_PUBLISHER_DESTINATION_H
#define SEE64YOU_PUBLISHER_DESTINATION_H

#include <stdint.h>

#include "document.h"

#define S64_DESTINATION_BASE_URL_CAPACITY 193u
#define S64_DESTINATION_URL_CAPACITY 320u
#define S64_WORDPRESS_REMOTE_ID_MAX 20u

typedef enum {
    S64_DESTINATION_NONE = 0,
    S64_DESTINATION_STANDALONE,
    S64_DESTINATION_WORDPRESS
} S64DestinationType;

typedef enum {
    S64_DESTINATION_OK = 0,
    S64_DESTINATION_INVALID_ARGUMENT,
    S64_DESTINATION_INVALID_BASE_URL,
    S64_DESTINATION_URL_TOO_LONG,
    S64_DESTINATION_INVALID_ID
} S64DestinationResult;

typedef struct {
    S64DestinationType type;
    char base_url[S64_DESTINATION_BASE_URL_CAPACITY];
    uint8_t allow_insecure_http;
} S64PublishDestination;

void s64_destination_clear(S64PublishDestination* destination);

S64DestinationResult s64_destination_configure(
    S64PublishDestination* destination,
    S64DestinationType type,
    const char* base_url,
    uint8_t allow_insecure_http
);

const char* s64_destination_result_message(S64DestinationResult result);

S64DestinationResult s64_destination_connection_url(
    const S64PublishDestination* destination,
    char* output,
    uint16_t capacity
);

S64DestinationResult s64_destination_list_url(
    const S64PublishDestination* destination,
    S64DocumentType type,
    char* output,
    uint16_t capacity
);

S64DestinationResult s64_destination_open_url(
    const S64PublishDestination* destination,
    S64DocumentType type,
    const char* identity,
    char* output,
    uint16_t capacity
);

S64DestinationResult s64_destination_save_url(
    const S64PublishDestination* destination,
    const char* identity,
    uint8_t updating,
    char* output,
    uint16_t capacity
);

S64DestinationResult s64_destination_delete_url(
    const S64PublishDestination* destination,
    S64DocumentType type,
    const char* identity,
    char* output,
    uint16_t capacity
);

#endif
