#include "network.h"

#define S64_NET_TARGET            0x03u
#define S64_NET_CMD_OPEN_TCP      0x07u
#define S64_NET_CMD_CLOSE_SOCKET  0x09u
#define S64_NET_CMD_READ_SOCKET   0x10u
#define S64_NET_CMD_WRITE_SOCKET  0x11u

#define S64_NET_STATUS_CAPACITY 32u
#define S64_NET_WRITE_PREFIX     3u
#define S64_NET_WRITE_MAX        (S64_UCI_COMMAND_CAPACITY - S64_NET_WRITE_PREFIX)

typedef struct {
    uint8_t bytes[2];
    uint16_t count;
} S64NetPairSink;

typedef struct {
    uint8_t value;
    uint16_t count;
} S64NetHandleSink;

typedef struct {
    uint8_t header[2];
    uint8_t header_count;
    uint16_t delivered;
    S64UciDataSink sink;
    void* sink_context;
    uint8_t sink_failed;
} S64NetReadSink;

static uint8_t capture_pair(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64NetPairSink* sink = (S64NetPairSink*)context;
    uint16_t i;

    for (i = 0u; i < length; ++i) {
        if (sink->count < 2u) {
            sink->bytes[sink->count] = data[i];
        }
        ++sink->count;
    }
    return 1u;
}

static uint8_t capture_handle(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64NetHandleSink* sink = (S64NetHandleSink*)context;
    uint16_t i;

    for (i = 0u; i < length; ++i) {
        if (sink->count == 0u) {
            sink->value = data[i];
        }
        ++sink->count;
    }
    return 1u;
}

static uint8_t capture_read(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64NetReadSink* sink = (S64NetReadSink*)context;
    uint16_t offset = 0u;

    while (offset < length && sink->header_count < 2u) {
        sink->header[sink->header_count++] = data[offset++];
    }

    if (offset < length) {
        uint16_t payload = (uint16_t)(length - offset);

        if (sink->sink != 0
            && !sink->sink(data + offset, payload, sink->sink_context)) {
            sink->sink_failed = 1u;
            return 0u;
        }
        sink->delivered = (uint16_t)(sink->delivered + payload);
    }
    return 1u;
}

static uint8_t status_is(
    const char* status,
    uint16_t length,
    char first,
    char second
)
{
    return status != 0
        && length >= 2u
        && status[0] == first
        && status[1] == second;
}

static S64NetResult execute_status(
    S64NetTransport* transport,
    const S64UciCommandPart* parts,
    uint8_t part_count
)
{
    S64UciReply reply;
    S64UciResult result;
    char status[S64_NET_STATUS_CAPACITY];

    if (transport == 0 || transport->execute == 0) {
        return S64_NET_INVALID_ARGUMENT;
    }

    reply.data_sink = 0;
    reply.data_context = 0;
    reply.status = status;
    reply.status_capacity = (uint16_t)sizeof(status);
    reply.status_length = 0u;

    result = transport->execute(parts, part_count, &reply);
    if (result != S64_UCI_OK) {
        return S64_NET_TRANSPORT_ERROR;
    }

    return status_is(status, reply.status_length, '0', '0')
        ? S64_NET_OK
        : S64_NET_TARGET_ERROR;
}

void s64_net_init(S64NetTransport* transport, S64UciExecuteFn execute)
{
    if (transport != 0) {
        transport->execute = execute;
    }
}

S64NetResult s64_net_open_tcp(
    S64NetTransport* transport,
    const char* host,
    uint16_t port,
    uint8_t* handle
)
{
    uint8_t prefix[4];
    uint8_t terminator = 0u;
    uint16_t host_length = 0u;
    S64UciCommandPart parts[3];
    S64UciReply reply;
    S64UciResult result;
    S64NetHandleSink sink;
    char status[S64_NET_STATUS_CAPACITY];

    if (transport == 0 || transport->execute == 0
        || host == 0 || handle == 0 || port == 0u) {
        return S64_NET_INVALID_ARGUMENT;
    }

    while (host[host_length] != '\0') {
        if (host_length
            >= (uint16_t)(S64_UCI_COMMAND_CAPACITY - sizeof(prefix) - 1u)) {
            return S64_NET_INVALID_ARGUMENT;
        }
        ++host_length;
    }
    if (host_length == 0u) {
        return S64_NET_INVALID_ARGUMENT;
    }

    prefix[0] = S64_NET_TARGET;
    prefix[1] = S64_NET_CMD_OPEN_TCP;
    prefix[2] = (uint8_t)(port & 0xFFu);
    prefix[3] = (uint8_t)(port >> 8);

    parts[0].data = prefix;
    parts[0].length = (uint16_t)sizeof(prefix);
    parts[0].flags = 0u;
    parts[1].data = (const uint8_t*)host;
    parts[1].length = host_length;
    parts[1].flags = S64_UCI_PART_NATIVE_TEXT;
    parts[2].data = &terminator;
    parts[2].length = 1u;
    parts[2].flags = 0u;

    sink.value = 0u;
    sink.count = 0u;
    reply.data_sink = capture_handle;
    reply.data_context = &sink;
    reply.status = status;
    reply.status_capacity = (uint16_t)sizeof(status);
    reply.status_length = 0u;

    result = transport->execute(parts, 3u, &reply);
    if (result != S64_UCI_OK) {
        return S64_NET_TRANSPORT_ERROR;
    }
    if (!status_is(status, reply.status_length, '0', '0')) {
        return S64_NET_TARGET_ERROR;
    }
    if (sink.count != 1u) {
        return S64_NET_TRANSPORT_ERROR;
    }

    *handle = sink.value;
    return S64_NET_OK;
}

S64NetResult s64_net_close(S64NetTransport* transport, uint8_t handle)
{
    uint8_t command[3];
    S64UciCommandPart part;

    command[0] = S64_NET_TARGET;
    command[1] = S64_NET_CMD_CLOSE_SOCKET;
    command[2] = handle;

    part.data = command;
    part.length = (uint16_t)sizeof(command);
    part.flags = 0u;

    return execute_status(transport, &part, 1u);
}

static S64NetResult write_internal(
    S64NetTransport* transport,
    uint8_t handle,
    const uint8_t* data,
    uint16_t length,
    uint8_t flags
)
{
    uint8_t prefix[3];
    uint16_t offset = 0u;

    if (transport == 0 || transport->execute == 0
        || (length != 0u && data == 0)) {
        return S64_NET_INVALID_ARGUMENT;
    }

    prefix[0] = S64_NET_TARGET;
    prefix[1] = S64_NET_CMD_WRITE_SOCKET;
    prefix[2] = handle;

    while (offset < length) {
        uint16_t chunk = (uint16_t)(length - offset);
        uint16_t sent;
        S64UciCommandPart parts[2];
        S64UciReply reply;
        S64UciResult result;
        S64NetPairSink sink;
        char status[S64_NET_STATUS_CAPACITY];

        if (chunk > S64_NET_WRITE_MAX) {
            chunk = S64_NET_WRITE_MAX;
        }

        parts[0].data = prefix;
        parts[0].length = (uint16_t)sizeof(prefix);
        parts[0].flags = 0u;
        parts[1].data = data + offset;
        parts[1].length = chunk;
        parts[1].flags = flags;

        sink.count = 0u;
        sink.bytes[0] = 0u;
        sink.bytes[1] = 0u;
        reply.data_sink = capture_pair;
        reply.data_context = &sink;
        reply.status = status;
        reply.status_capacity = (uint16_t)sizeof(status);
        reply.status_length = 0u;

        result = transport->execute(parts, 2u, &reply);
        if (result != S64_UCI_OK) {
            return S64_NET_TRANSPORT_ERROR;
        }
        if (!status_is(status, reply.status_length, '0', '0')) {
            return S64_NET_TARGET_ERROR;
        }
        if (sink.count != 2u) {
            return S64_NET_TRANSPORT_ERROR;
        }

        sent = (uint16_t)(
            sink.bytes[0] | ((uint16_t)sink.bytes[1] << 8)
        );
        if (sent == 0u || sent > chunk) {
            return S64_NET_TRANSPORT_ERROR;
        }
        offset = (uint16_t)(offset + sent);
    }

    return S64_NET_OK;
}

S64NetResult s64_net_write(
    S64NetTransport* transport,
    uint8_t handle,
    const uint8_t* data,
    uint16_t length
)
{
    return write_internal(transport, handle, data, length, 0u);
}

S64NetResult s64_net_write_text(
    S64NetTransport* transport,
    uint8_t handle,
    const char* text,
    uint16_t length
)
{
    return write_internal(
        transport,
        handle,
        (const uint8_t*)text,
        length,
        S64_UCI_PART_NATIVE_TEXT
    );
}

S64NetResult s64_net_read(
    S64NetTransport* transport,
    uint8_t handle,
    uint16_t maximum,
    S64UciDataSink sink,
    void* sink_context,
    uint16_t* received
)
{
    uint8_t command[5];
    uint16_t reported;
    S64UciCommandPart part;
    S64UciReply reply;
    S64UciResult execute_result;
    S64NetReadSink read_sink;
    char status[S64_NET_STATUS_CAPACITY];

    if (received != 0) {
        *received = 0u;
    }
    if (transport == 0 || transport->execute == 0
        || maximum == 0u || maximum > S64_NET_READ_MAX) {
        return S64_NET_INVALID_ARGUMENT;
    }

    command[0] = S64_NET_TARGET;
    command[1] = S64_NET_CMD_READ_SOCKET;
    command[2] = handle;
    command[3] = (uint8_t)(maximum & 0xFFu);
    command[4] = (uint8_t)(maximum >> 8);

    part.data = command;
    part.length = (uint16_t)sizeof(command);
    part.flags = 0u;

    read_sink.header_count = 0u;
    read_sink.delivered = 0u;
    read_sink.sink = sink;
    read_sink.sink_context = sink_context;
    read_sink.sink_failed = 0u;

    reply.data_sink = capture_read;
    reply.data_context = &read_sink;
    reply.status = status;
    reply.status_capacity = (uint16_t)sizeof(status);
    reply.status_length = 0u;

    execute_result = transport->execute(&part, 1u, &reply);
    if (execute_result == S64_UCI_CONSUMER_ABORTED
        || read_sink.sink_failed) {
        return S64_NET_TRANSPORT_ERROR;
    }
    if (execute_result != S64_UCI_OK) {
        return S64_NET_TRANSPORT_ERROR;
    }

    if (status_is(status, reply.status_length, '0', '2')) {
        return S64_NET_NO_DATA;
    }

    if (read_sink.header_count != 2u) {
        return status_is(status, reply.status_length, '0', '1')
            ? S64_NET_CLOSED
            : S64_NET_TRANSPORT_ERROR;
    }

    reported = (uint16_t)(
        read_sink.header[0] | ((uint16_t)read_sink.header[1] << 8)
    );
    if (reported > maximum || read_sink.delivered != reported) {
        return S64_NET_TRANSPORT_ERROR;
    }
    if (received != 0) {
        *received = reported;
    }

    if (status_is(status, reply.status_length, '0', '1')) {
        return S64_NET_CLOSED;
    }
    return status_is(status, reply.status_length, '0', '0')
        ? S64_NET_OK
        : S64_NET_TARGET_ERROR;
}
