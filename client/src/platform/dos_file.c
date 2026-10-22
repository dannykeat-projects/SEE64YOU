#include "dos.h"

#include <string.h>

#define S64_DOS_CMD_FILE_SEEK 0x06u
#define S64_DOS_CMD_FILE_INFO 0x07u
#define S64_DOS_STATUS_CAPACITY 64u

typedef struct {
    uint8_t bytes[4];
    uint16_t count;
} S64DosFileInfoSink;

static uint8_t target_valid(uint8_t target)
{
    return target == S64_DOS_TARGET_PRIMARY
        || target == S64_DOS_TARGET_SECONDARY;
}

static uint8_t status_ok_or_empty(const char* status, uint16_t length)
{
    return length == 0u
        || (status != 0
            && length >= 2u
            && status[0] == '0'
            && status[1] == '0');
}

static uint8_t status_code(const char* status, uint16_t length)
{
    if (status == 0 || length < 2u
        || status[0] < '0' || status[0] > '9'
        || status[1] < '0' || status[1] > '9') {
        return 0xffu;
    }

    return (uint8_t)(
        (uint8_t)(status[0] - '0') * 10u
        + (uint8_t)(status[1] - '0')
    );
}

static S64DosResult map_status(const char* status, uint16_t length)
{
    uint8_t code;

    if (status_ok_or_empty(status, length)) return S64_DOS_OK;

    code = status_code(status, length);
    if (code == 1u || code == 82u || code == 88u) return S64_DOS_NOT_FOUND;
    if (code == 85u) return S64_DOS_NO_FILE_OPEN;
    if (status != 0 && strstr(status, "ACCESS DENIED") != 0) {
        return S64_DOS_ACCESS_DENIED;
    }
    return S64_DOS_TARGET_ERROR;
}

static S64DosResult map_transport(S64UciResult result)
{
    if (result == S64_UCI_OK) return S64_DOS_OK;
    if (result == S64_UCI_CONSUMER_ABORTED) return S64_DOS_CONSUMER_ABORTED;
    return S64_DOS_TRANSPORT_ERROR;
}

static uint8_t file_info_sink(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64DosFileInfoSink* state = (S64DosFileInfoSink*)context;
    uint16_t i;

    for (i = 0u; i < length; ++i) {
        if (state->count < (uint16_t)sizeof(state->bytes)) {
            state->bytes[state->count] = data[i];
        }
        ++state->count;
    }
    return 1u;
}

S64DosResult s64_dos_seek(S64Dos* dos, uint32_t position)
{
    uint8_t command[6];
    S64UciCommandPart part;
    S64UciReply reply;
    S64UciResult transport;
    S64DosResult result;
    char status[S64_DOS_STATUS_CAPACITY];

    if (dos == 0 || dos->execute == 0 || !target_valid(dos->target)) {
        return S64_DOS_INVALID_ARGUMENT;
    }

    command[0] = dos->target;
    command[1] = S64_DOS_CMD_FILE_SEEK;
    command[2] = (uint8_t)(position & 0xffu);
    command[3] = (uint8_t)((position >> 8) & 0xffu);
    command[4] = (uint8_t)((position >> 16) & 0xffu);
    command[5] = (uint8_t)((position >> 24) & 0xffu);
    part.data = command;
    part.length = (uint16_t)sizeof(command);
    part.flags = 0u;

    reply.data_sink = 0;
    reply.data_context = 0;
    reply.status = status;
    reply.status_capacity = (uint16_t)sizeof(status);
    reply.status_length = 0u;

    transport = dos->execute(&part, 1u, &reply);
    if (transport != S64_UCI_OK) return map_transport(transport);
    if (reply.status_length == 0u) return S64_DOS_TARGET_ERROR;
    result = map_status(status, reply.status_length);
    return result;
}

S64DosResult s64_dos_file_size(S64Dos* dos, uint32_t* size)
{
    uint8_t command[2];
    S64UciCommandPart part;
    S64UciReply reply;
    S64UciResult transport;
    S64DosResult result;
    S64DosFileInfoSink info;
    char status[S64_DOS_STATUS_CAPACITY];

    if (size == 0) return S64_DOS_INVALID_ARGUMENT;
    *size = 0u;
    if (dos == 0 || dos->execute == 0 || !target_valid(dos->target)) {
        return S64_DOS_INVALID_ARGUMENT;
    }

    command[0] = dos->target;
    command[1] = S64_DOS_CMD_FILE_INFO;
    part.data = command;
    part.length = (uint16_t)sizeof(command);
    part.flags = 0u;

    info.count = 0u;
    reply.data_sink = file_info_sink;
    reply.data_context = &info;
    reply.status = status;
    reply.status_capacity = (uint16_t)sizeof(status);
    reply.status_length = 0u;

    transport = dos->execute(&part, 1u, &reply);
    if (transport != S64_UCI_OK) return map_transport(transport);

    result = map_status(status, reply.status_length);
    if (result != S64_DOS_OK) return result;
    if (info.count < (uint16_t)sizeof(info.bytes)) {
        return S64_DOS_TARGET_ERROR;
    }

    *size = (uint32_t)info.bytes[0]
        | ((uint32_t)info.bytes[1] << 8)
        | ((uint32_t)info.bytes[2] << 16)
        | ((uint32_t)info.bytes[3] << 24);
    return S64_DOS_OK;
}
