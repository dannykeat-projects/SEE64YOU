#ifndef SEE64YOU_STANDALONE_DIRECT_H
#define SEE64YOU_STANDALONE_DIRECT_H

#include "standalone.h"
#ifdef S64_PERSISTENT_RUNTIME_FIT
#include "publisher/persistent_services.h"
#endif

/* Firmware-1.1 backend for the existing synchronous publisher operations. */
int s64_direct_publisher_key(const char* hex);
S64StandaloneResult s64_direct_publisher_test(S64HttpTransport* transport,
    const char* url, uint8_t allow_http);
#ifdef S64_PERSISTENT_RUNTIME_FIT
S64StandaloneResult s64_persistent_standalone_open_stage(
    S64HttpTransport* transport,
    const char* url,
    uint8_t allow_http,
    S64PersistentOpenStage* stage
);
#endif

#endif
