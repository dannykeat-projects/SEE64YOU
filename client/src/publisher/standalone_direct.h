#ifndef SEE64YOU_STANDALONE_DIRECT_H
#define SEE64YOU_STANDALONE_DIRECT_H

#include "standalone.h"

/* Firmware-1.1 backend for the existing synchronous publisher operations. */
int s64_direct_publisher_key(const char* hex);
S64StandaloneResult s64_direct_publisher_test(S64HttpTransport* transport,
    const char* url, uint8_t allow_http);

#endif
