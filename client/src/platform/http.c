#include "http.h"
#include "charset.h"

#define S64_HTTP_TARGET              0x06u
#define S64_HTTP_CMD_FREE_ALL        0x10u
#define S64_HTTP_CMD_HEADER_CREATE   0x11u
#define S64_HTTP_CMD_HEADER_FREE     0x12u
#define S64_HTTP_CMD_HEADER_ADD      0x13u
#define S64_HTTP_CMD_BODY_CREATE     0x21u
#define S64_HTTP_CMD_BODY_FREE       0x22u
#define S64_HTTP_CMD_BODY_QUERY      0x2Au
#define S64_HTTP_CMD_BODY_ADD_STRING 0x25u
#define S64_HTTP_CMD_BODY_ADD_BINARY 0x2Cu
#define S64_HTTP_CMD_EXCHANGE_OBJECT 0x31u
#define S64_HTTP_CMD_EXCHANGE_RAW    0x32u

#define S64_HTTP_SMALL_STATUS_CAPACITY 32u
#define S64_HTTP_BINARY_PREFIX_SIZE     3u
#define S64_HTTP_BINARY_CHUNK_MAX     (S64_UCI_COMMAND_CAPACITY - S64_HTTP_BINARY_PREFIX_SIZE)

typedef struct {
    uint8_t value;
    uint16_t count;
} S64HttpHandleSink;

typedef struct {
    uint8_t values[2];
    uint16_t count;
} S64HttpPairSink;

typedef struct {
    char* value;
    uint16_t capacity;
    uint16_t total;
    uint16_t received;
    uint16_t expected;
    uint8_t invalid;
} S64HttpStringQuerySink;

typedef struct {
    uint8_t bytes[5];
    uint16_t count;
    uint8_t invalid;
} S64HttpIntegerQuerySink;


static uint8_t s64_http_capture_pair(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64HttpPairSink* sink = (S64HttpPairSink*)context;
    uint16_t i;

    for (i = 0u; i < length; ++i) {
        if (sink->count < 2u) {
            sink->values[sink->count] = data[i];
        }
        ++sink->count;
    }
    return 1u;
}

static uint8_t s64_http_capture_string_query(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64HttpStringQuerySink* sink = (S64HttpStringQuerySink*)context;
    uint16_t i;

    for (i = 0u; i < length; ++i) {
        uint8_t byte = data[i];

        if (sink->total == 0u) {
            if (byte != 0x03u) {
                sink->invalid = 1u;
            }
        } else if (sink->total == 1u) {
            sink->expected = byte;
            if ((uint16_t)(sink->expected + 1u) > sink->capacity) {
                sink->invalid = 1u;
            }
        } else {
            uint8_t native;

            if (sink->received < sink->expected
                && sink->received + 1u < sink->capacity
                && sink->value != 0
                && s64_charset_ascii_to_native(byte, &native)) {
                sink->value[sink->received] = (char)native;
            } else {
                sink->invalid = 1u;
            }
            ++sink->received;
        }
        ++sink->total;
    }
    return 1u;
}

static uint8_t s64_http_capture_integer_query(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64HttpIntegerQuerySink* sink = (S64HttpIntegerQuerySink*)context;
    uint16_t i;

    for (i = 0u; i < length; ++i) {
        if (sink->count < 5u) {
            sink->bytes[sink->count] = data[i];
        } else {
            sink->invalid = 1u;
        }
        ++sink->count;
    }
    return 1u;
}

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
    if (status == 0 || length < 3u
        || status[0] != '0' || status[1] != '0') {
        return 0u;
    }

    /*
     * Current HTTP-target documentation uses "000 OK". Stock Commodore C64
     * Ultimate 1.1 hardware has been observed to expose the same target while
     * retaining the older Ultimate "00,OK" status convention. Accept both
     * explicit success forms; do not reduce this to a two-byte prefix check.
     */
    return status[2] == '0' || status[2] == ',';
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
    part.flags = 0u;
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
    parts[0].flags = 0u;
    parts[1].data = (const uint8_t*)url;
    parts[1].length = url_length;
    parts[1].flags = S64_UCI_PART_NATIVE_TEXT;

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
    parts[0].flags = 0u;
    parts[1].data = (const uint8_t*)header_line;
    parts[1].length = line_length;
    parts[1].flags = S64_UCI_PART_NATIVE_TEXT;

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
    part.flags = 0u;
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
    part.flags = 0u;
    return s64_http_create_handle(transport, &part, 1u, handle);
}

S64HttpResult s64_http_body_add_string(
    S64HttpTransport* transport,
    uint8_t handle,
    const char* key,
    const char* value
)
{
    uint8_t prefix[4];
    uint8_t value_length_byte;
    uint16_t key_length;
    uint16_t value_length;
    S64UciCommandPart parts[4];

    if (!s64_http_string_length(key, 255u, &key_length)
        || key_length == 0u
        || !s64_http_string_length(value, 255u, &value_length)) {
        return S64_HTTP_INVALID_ARGUMENT;
    }

    prefix[0] = S64_HTTP_TARGET;
    prefix[1] = S64_HTTP_CMD_BODY_ADD_STRING;
    prefix[2] = handle;
    prefix[3] = (uint8_t)key_length;
    value_length_byte = (uint8_t)value_length;

    parts[0].data = prefix;
    parts[0].length = (uint16_t)sizeof(prefix);
    parts[0].flags = 0u;
    parts[1].data = (const uint8_t*)key;
    parts[1].length = key_length;
    parts[1].flags = S64_UCI_PART_NATIVE_TEXT;
    parts[2].data = &value_length_byte;
    parts[2].length = 1u;
    parts[2].flags = 0u;
    parts[3].data = (const uint8_t*)value;
    parts[3].length = value_length;
    parts[3].flags = S64_UCI_PART_NATIVE_TEXT;

    return s64_http_run_status_command(transport, parts, 4u);
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
    parts[0].flags = 0u;

    while (offset < length) {
        chunk = (uint16_t)(length - offset);
        if (chunk > S64_HTTP_BINARY_CHUNK_MAX) {
            chunk = S64_HTTP_BINARY_CHUNK_MAX;
        }

        parts[1].data = data + offset;
        parts[1].length = chunk;
        parts[1].flags = 0u;

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
    part.flags = 0u;
    return s64_http_run_status_command(transport, &part, 1u);
}


S64HttpResult s64_http_body_query_string(
    S64HttpTransport* transport,
    uint8_t handle,
    const char* path,
    char* value,
    uint16_t value_capacity
)
{
    uint8_t prefix[3];
    uint16_t path_length;
    S64UciCommandPart parts[2];
    S64UciReply reply;
    S64UciResult execute_result;
    S64HttpStringQuerySink sink;
    char status[S64_HTTP_SMALL_STATUS_CAPACITY];

    if (transport == 0 || transport->execute == 0
        || value == 0 || value_capacity == 0u
        || !s64_http_string_length(
            path,
            (uint16_t)(S64_UCI_COMMAND_CAPACITY - sizeof(prefix)),
            &path_length
        )
        || path_length == 0u) {
        return S64_HTTP_INVALID_ARGUMENT;
    }

    prefix[0] = S64_HTTP_TARGET;
    prefix[1] = S64_HTTP_CMD_BODY_QUERY;
    prefix[2] = handle;

    parts[0].data = prefix;
    parts[0].length = (uint16_t)sizeof(prefix);
    parts[0].flags = 0u;
    parts[1].data = (const uint8_t*)path;
    parts[1].length = path_length;
    parts[1].flags = S64_UCI_PART_NATIVE_TEXT;

    sink.value = value;
    sink.capacity = value_capacity;
    sink.total = 0u;
    sink.received = 0u;
    sink.expected = 0u;
    sink.invalid = 0u;
    value[0] = '\0';

    reply.data_sink = s64_http_capture_string_query;
    reply.data_context = &sink;
    reply.status = status;
    reply.status_capacity = (uint16_t)sizeof(status);
    reply.status_length = 0u;

    execute_result = transport->execute(parts, 2u, &reply);
    if (execute_result != S64_UCI_OK) {
        return s64_http_map_transport(execute_result);
    }
    if (!s64_http_status_ok(status, reply.status_length)) {
        return S64_HTTP_TARGET_ERROR;
    }
    if (sink.invalid || sink.total < 2u || sink.received != sink.expected) {
        return S64_HTTP_TRANSPORT_ERROR;
    }

    value[sink.expected] = '\0';
    return S64_HTTP_OK;
}

S64HttpResult s64_http_body_query_integer(
    S64HttpTransport* transport,
    uint8_t handle,
    const char* path,
    int32_t* value
)
{
    uint8_t prefix[3];
    uint16_t path_length;
    S64UciCommandPart parts[2];
    S64UciReply reply;
    S64UciResult execute_result;
    S64HttpIntegerQuerySink sink;
    char status[S64_HTTP_SMALL_STATUS_CAPACITY];
    uint32_t raw;

    if (transport == 0 || transport->execute == 0 || value == 0
        || !s64_http_string_length(
            path,
            (uint16_t)(S64_UCI_COMMAND_CAPACITY - sizeof(prefix)),
            &path_length
        )
        || path_length == 0u) {
        return S64_HTTP_INVALID_ARGUMENT;
    }

    prefix[0] = S64_HTTP_TARGET;
    prefix[1] = S64_HTTP_CMD_BODY_QUERY;
    prefix[2] = handle;

    parts[0].data = prefix;
    parts[0].length = (uint16_t)sizeof(prefix);
    parts[0].flags = 0u;
    parts[1].data = (const uint8_t*)path;
    parts[1].length = path_length;
    parts[1].flags = S64_UCI_PART_NATIVE_TEXT;

    sink.count = 0u;
    sink.invalid = 0u;

    reply.data_sink = s64_http_capture_integer_query;
    reply.data_context = &sink;
    reply.status = status;
    reply.status_capacity = (uint16_t)sizeof(status);
    reply.status_length = 0u;

    execute_result = transport->execute(parts, 2u, &reply);
    if (execute_result != S64_UCI_OK) {
        return s64_http_map_transport(execute_result);
    }
    if (!s64_http_status_ok(status, reply.status_length)) {
        return S64_HTTP_TARGET_ERROR;
    }
    if (sink.invalid || sink.count != 5u || sink.bytes[0] != 0x01u) {
        return S64_HTTP_TRANSPORT_ERROR;
    }

    raw = (uint32_t)sink.bytes[1]
        | ((uint32_t)sink.bytes[2] << 8)
        | ((uint32_t)sink.bytes[3] << 16)
        | ((uint32_t)sink.bytes[4] << 24);
    *value = (int32_t)raw;
    return S64_HTTP_OK;
}

S64HttpResult s64_http_exchange_object(
    S64HttpTransport* transport,
    uint8_t header_handle,
    uint8_t body_handle,
    uint8_t* response_header_handle,
    uint8_t* response_body_handle,
    char* response_header,
    uint16_t response_header_capacity,
    uint16_t* response_header_length
)
{
    uint8_t command[4];
    S64UciCommandPart part;
    S64UciReply reply;
    S64UciResult result;
    S64HttpPairSink sink;

    if (transport == 0 || transport->execute == 0
        || response_header_handle == 0 || response_body_handle == 0
        || (response_header_capacity != 0u && response_header == 0)) {
        return S64_HTTP_INVALID_ARGUMENT;
    }

    command[0] = S64_HTTP_TARGET;
    command[1] = S64_HTTP_CMD_EXCHANGE_OBJECT;
    command[2] = header_handle;
    command[3] = body_handle;

    part.data = command;
    part.length = (uint16_t)sizeof(command);
    part.flags = 0u;

    sink.count = 0u;
    sink.values[0] = 0u;
    sink.values[1] = 0u;

    reply.data_sink = s64_http_capture_pair;
    reply.data_context = &sink;
    reply.status = response_header;
    reply.status_capacity = response_header_capacity;
    reply.status_length = 0u;

    result = transport->execute(&part, 1u, &reply);
    if (response_header_length != 0) {
        *response_header_length = reply.status_length;
    }
    if (result != S64_UCI_OK) {
        return s64_http_map_transport(result);
    }
    if (sink.count != 2u) {
        return S64_HTTP_TRANSPORT_ERROR;
    }

    *response_header_handle = sink.values[0];
    *response_body_handle = sink.values[1];
    return S64_HTTP_OK;
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
    part.flags = 0u;

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
