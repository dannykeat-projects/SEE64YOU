#include "http.h"

#define S64_HTTP_TARGET              0x06u
#define S64_HTTP_CMD_FREE_ALL        0x10u
#define S64_HTTP_CMD_HEADER_CREATE   0x11u
#define S64_HTTP_CMD_HEADER_FREE     0x12u
#define S64_HTTP_CMD_HEADER_ADD      0x13u
#define S64_HTTP_CMD_BODY_CREATE     0x21u
#define S64_HTTP_CMD_BODY_FREE       0x22u
#define S64_HTTP_CMD_BODY_ADD_BINARY 0x2Cu
#define S64_HTTP_CMD_EXCHANGE_RAW    0x32u

#define S64_HTTP_SMALL_STATUS_CAPACITY 32u
#define S64_HTTP_BINARY_PREFIX_SIZE     3u
#define S64_HTTP_BINARY_CHUNK_MAX     (S64_UCI_COMMAND_CAPACITY - S64_HTTP_BINARY_PREFIX_SIZE)

typedef struct {
    uint8_t value;
    uint16_t count;
} S64HttpHandleSink;

static uint8_t s64_http_capture_handle(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64HttpHandleSink* sink = (S64HttpHandleSink*)context;
    uint16_t i;

    for (i = 0u; i < length; ++i) {
        if (sink->count == 0u) {
            sink->value = data[i];
        }
        ++sink->count;
    }

    return 1u;
}

static uint8_t s64_http_status_ok(const char* status, uint16_t length)
{
    return length >= 3u
        && status != 0
        && status[0] == '0'
        && status[1] == '0'
        && status[2] == '0';
}

static S64HttpResult s64_http_map_transport(S64UciResult result)
{
    return result == S64_UCI_OK ? S64_HTTP_OK : S64_HTTP_TRANSPORT_ERROR;
}

static S64HttpResult s64_http_run_status_command(
    S64HttpTransport* transport,
    const S64UciCommandPart* parts,
    uint8_t part_count
)
{
    S64UciReply reply;
    S64UciResult result;
    char status[S64_HTTP_SMALL_STATUS_CAPACITY];

    if (transport == 0 || transport->execute == 0) {
        return S64_HTTP_INVALID_ARGUMENT;
    }

    reply.data_sink = 0;
    reply.data_context = 0;
    reply.status = status;
    reply.status_capacity = (uint16_t)sizeof(status);
    reply.status_length = 0u;

    result = transport->execute(parts, part_count, &reply);
    if (result != S64_UCI_OK) {
        return s64_http_map_transport(result);
    }

    return s64_http_status_ok(status, reply.status_length)
        ? S64_HTTP_OK
        : S64_HTTP_TARGET_ERROR;
}

static S64HttpResult s64_http_create_handle(
    S64HttpTransport* transport,
    const S64UciCommandPart* parts,
    uint8_t part_count,
    uint8_t* handle
)
{
    S64HttpHandleSink sink;
    S64UciReply reply;
    S64UciResult result;
    char status[S64_HTTP_SMALL_STATUS_CAPACITY];

    if (transport == 0 || transport->execute == 0 || handle == 0) {
        return S64_HTTP_INVALID_ARGUMENT;
    }

    sink.value = 0u;
    sink.count = 0u;

    reply.data_sink = s64_http_capture_handle;
    reply.data_context = &sink;
    reply.status = status;
    reply.status_capacity = (uint16_t)sizeof(status);
    reply.status_length = 0u;

    result = transport->execute(parts, part_count, &reply);
    if (result != S64_UCI_OK) {
        return s64_http_map_transport(result);
    }
    if (!s64_http_status_ok(status, reply.status_length)) {
        return S64_HTTP_TARGET_ERROR;
    }
    if (sink.count != 1u) {
        return S64_HTTP_TRANSPORT_ERROR;
    }

    *handle = sink.value;
    return S64_HTTP_OK;
}

static uint8_t s64_http_string_length(const char* text, uint16_t maximum, uint16_t* length)
{
    uint16_t i = 0u;

    if (text == 0 || length == 0) {
        return 0u;
    }

    while (i <= maximum && text[i] != '\0') {
        ++i;
    }

    if (i > maximum) {
        return 0u;
    }

    *length = i;
    return 1u;
}

void s64_http_init(S64HttpTransport* transport, S64UciExecuteFn execute)
{
    if (transport == 0) {
        return;
    }

    transport->execute = execute;
}

S64HttpResult s64_http_free_all(S64HttpTransport* transport)
{
    static const uint8_t command[] = {
        S64_HTTP_TARGET,
        S64_HTTP_CMD_FREE_ALL
    };
    S64UciCommandPart part;

    part.data = command;
    part.length = (uint16_t)sizeof(command);
    return s64_http_run_status_command(transport, &part, 1u);
}

S64HttpResult s64_http_header_create(
    S64HttpTransport* transport,
    S64HttpVerb verb,
    const char* url,
    uint8_t* handle
)
{
    uint8_t prefix[3];
    uint16_t url_length;
    S64UciCommandPart parts[2];

    if ((uint8_t)verb < (uint8_t)S64_HTTP_GET
        || (uint8_t)verb > (uint8_t)S64_HTTP_TRACE
        || !s64_http_string_length(
            url,
            (uint16_t)(S64_UCI_COMMAND_CAPACITY - sizeof(prefix)),
            &url_length
        )
        || url_length == 0u) {
        return S64_HTTP_INVALID_ARGUMENT;
    }

    prefix[0] = S64_HTTP_TARGET;
    prefix[1] = S64_HTTP_CMD_HEADER_CREATE;
    prefix[2] = (uint8_t)verb;

    parts[0].data = prefix;
    parts[0].length = (uint16_t)sizeof(prefix);
    parts[1].data = (const uint8_t*)url;
    parts[1].length = url_length;

    return s64_http_create_handle(transport, parts, 2u, handle);
}

S64HttpResult s64_http_header_add(
    S64HttpTransport* transport,
    uint8_t handle,
    const char* header_line
)
{
    uint8_t prefix[3];
    uint16_t line_length;
    S64UciCommandPart parts[2];

    if (!s64_http_string_length(
            header_line,
            (uint16_t)(S64_UCI_COMMAND_CAPACITY - sizeof(prefix)),
            &line_length
        )
        || line_length == 0u) {
        return S64_HTTP_INVALID_ARGUMENT;
    }

    prefix[0] = S64_HTTP_TARGET;
    prefix[1] = S64_HTTP_CMD_HEADER_ADD;
    prefix[2] = handle;

    parts[0].data = prefix;
    parts[0].length = (uint16_t)sizeof(prefix);
    parts[1].data = (const uint8_t*)header_line;
    parts[1].length = line_length;

    return s64_http_run_status_command(transport, parts, 2u);
}

S64HttpResult s64_http_header_free(S64HttpTransport* transport, uint8_t handle)
{
    uint8_t command[3];
    S64UciCommandPart part;

    command[0] = S64_HTTP_TARGET;
    command[1] = S64_HTTP_CMD_HEADER_FREE;
    command[2] = handle;

    part.data = command;
    part.length = (uint16_t)sizeof(command);
    return s64_http_run_status_command(transport, &part, 1u);
}

S64HttpResult s64_http_body_create(
    S64HttpTransport* transport,
    S64HttpBodyFormat format,
    uint8_t* handle
)
{
    uint8_t command[3];
    S64UciCommandPart part;

    if ((uint8_t)format < (uint8_t)S64_HTTP_BODY_BINARY
        || (uint8_t)format > (uint8_t)S64_HTTP_BODY_URL_ENCODED) {
        return S64_HTTP_INVALID_ARGUMENT;
    }

    command[0] = S64_HTTP_TARGET;
    command[1] = S64_HTTP_CMD_BODY_CREATE;
    command[2] = (uint8_t)format;

    part.data = command;
    part.length = (uint16_t)sizeof(command);
    return s64_http_create_handle(transport, &part, 1u, handle);
}

S64HttpResult s64_http_body_add_binary(
    S64HttpTransport* transport,
    uint8_t handle,
    const uint8_t* data,
    uint16_t length
)
{
    uint8_t prefix[S64_HTTP_BINARY_PREFIX_SIZE];
    uint16_t offset = 0u;
    uint16_t chunk;
    S64UciCommandPart parts[2];
    S64HttpResult result;

    if (transport == 0 || transport->execute == 0
        || (length != 0u && data == 0)) {
        return S64_HTTP_INVALID_ARGUMENT;
    }

    if (length == 0u) {
        return S64_HTTP_OK;
    }

    prefix[0] = S64_HTTP_TARGET;
    prefix[1] = S64_HTTP_CMD_BODY_ADD_BINARY;
    prefix[2] = handle;

    parts[0].data = prefix;
    parts[0].length = (uint16_t)sizeof(prefix);

    while (offset < length) {
        chunk = (uint16_t)(length - offset);
        if (chunk > S64_HTTP_BINARY_CHUNK_MAX) {
            chunk = S64_HTTP_BINARY_CHUNK_MAX;
        }

        parts[1].data = data + offset;
        parts[1].length = chunk;

        result = s64_http_run_status_command(transport, parts, 2u);
        if (result != S64_HTTP_OK) {
            return result;
        }

        offset = (uint16_t)(offset + chunk);
    }

    return S64_HTTP_OK;
}

S64HttpResult s64_http_body_free(S64HttpTransport* transport, uint8_t handle)
{
    uint8_t command[3];
    S64UciCommandPart part;

    command[0] = S64_HTTP_TARGET;
    command[1] = S64_HTTP_CMD_BODY_FREE;
    command[2] = handle;

    part.data = command;
    part.length = (uint16_t)sizeof(command);
    return s64_http_run_status_command(transport, &part, 1u);
}

S64HttpResult s64_http_exchange_raw(
    S64HttpTransport* transport,
    uint8_t header_handle,
    uint8_t body_handle,
    S64UciDataSink body_sink,
    void* body_context,
    char* response_header,
    uint16_t response_header_capacity,
    uint16_t* response_header_length
)
{
    uint8_t command[4];
    S64UciCommandPart part;
    S64UciReply reply;
    S64UciResult result;

    if (transport == 0 || transport->execute == 0
        || (response_header_capacity != 0u && response_header == 0)) {
        return S64_HTTP_INVALID_ARGUMENT;
    }

    command[0] = S64_HTTP_TARGET;
    command[1] = S64_HTTP_CMD_EXCHANGE_RAW;
    command[2] = header_handle;
    command[3] = body_handle;

    part.data = command;
    part.length = (uint16_t)sizeof(command);

    reply.data_sink = body_sink;
    reply.data_context = body_context;
    reply.status = response_header;
    reply.status_capacity = response_header_capacity;
    reply.status_length = 0u;

    result = transport->execute(&part, 1u, &reply);
    if (response_header_length != 0) {
        *response_header_length = reply.status_length;
    }

    return s64_http_map_transport(result);
}
