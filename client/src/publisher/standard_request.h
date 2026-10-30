#ifndef SEE64YOU_STANDARD_REQUEST_H
#define SEE64YOU_STANDARD_REQUEST_H

#include <stdint.h>

typedef enum {
    S64_PUBLISH_REQUEST_STATUS,
    S64_PUBLISH_REQUEST_CHALLENGE,
    S64_PUBLISH_REQUEST_LIST,
    S64_PUBLISH_REQUEST_PUT
} S64PublishRequestKind;

/* Build the HTTP/1.0 origin-form URI path for the HMAC publishing route.
   path is the endpoint's URI path, not a whole URL; buffer is cleared on error. */
int s64_publish_request_path(char* output, uint16_t capacity,
    const char* endpoint_path, S64PublishRequestKind kind,
    const char* id, const char* name, uint32_t size,
    const char* challenge);

#endif
