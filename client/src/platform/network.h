#ifndef SEE64YOU_PLATFORM_NETWORK_H
#define SEE64YOU_PLATFORM_NETWORK_H

#include <stdint.h>

#include "uci.h"

/*
 * Keep reads at 512 bytes for the first stock-firmware path. Older Ultimate
 * network-target revisions have had larger-read edge cases, and streaming
 * repeated 512-byte reads is sufficient for See64You.
 */
#define S64_NET_READ_MAX 512u

typedef enum {
    S64_NET_OK = 0,
    S64_NET_INVALID_ARGUMENT,
    S64_NET_TRANSPORT_ERROR,
    S64_NET_TARGET_ERROR,
    S64_NET_NO_DATA,
    S64_NET_CLOSED
} S64NetResult;

typedef struct {
    S64UciExecuteFn execute;
} S64NetTransport;

void s64_net_init(S64NetTransport* transport, S64UciExecuteFn execute);

S64NetResult s64_net_open_tcp(
    S64NetTransport* transport,
    const char* host,
    uint16_t port,
    uint8_t* handle
);

S64NetResult s64_net_close(S64NetTransport* transport, uint8_t handle);

S64NetResult s64_net_write(
    S64NetTransport* transport,
    uint8_t handle,
    const uint8_t* data,
    uint16_t length
);

S64NetResult s64_net_write_text(
    S64NetTransport* transport,
    uint8_t handle,
    const char* text,
    uint16_t length
);

S64NetResult s64_net_read(
    S64NetTransport* transport,
    uint8_t handle,
    uint16_t maximum,
    S64UciDataSink sink,
    void* sink_context,
    uint16_t* received
);

#endif
