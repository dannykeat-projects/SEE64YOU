#include "platform/dos_path.h"
#include "charset.h"

typedef struct {
    char* output;
    uint16_t capacity;
    uint16_t used;
    uint8_t ended;
    uint8_t invalid;
} S64DosPathRead;

static uint8_t path_read(const uint8_t* bytes, uint16_t length, void* context)
{
    S64DosPathRead* read = (S64DosPathRead*)context;
    uint16_t i;
    uint8_t native;
    for (i = 0u; i < length; ++i) {
        if (read->ended) goto invalid;
        if (bytes[i] == 0u) {
            read->ended = 1u;
            continue;
        }
        if (bytes[i] < 32u || bytes[i] > 126u
            || read->used + 1u >= read->capacity
            || !s64_charset_ascii_to_native(bytes[i], &native)) goto invalid;
        read->output[read->used++] = (char)native;
    }
    read->output[read->used] = 0;
    return 1u;
invalid:
    read->invalid = 1u;
    return 0u;
}

S64DosResult s64_dos_get_path(S64Dos* dos, char* output, uint16_t capacity)
{
    uint8_t command[2];
    S64UciCommandPart part;
    S64UciReply reply;
    S64UciResult result;
    S64DosPathRead read;
    char status[32];
    if (!output || capacity == 0u) return S64_DOS_INVALID_ARGUMENT;
    output[0] = 0;
    if (!dos || !dos->execute || capacity < 2u
        || (dos->target != S64_DOS_TARGET_PRIMARY
            && dos->target != S64_DOS_TARGET_SECONDARY))
        return S64_DOS_INVALID_ARGUMENT;
    command[0] = dos->target;
    command[1] = 0x12u;
    part.data = command;
    part.length = sizeof(command);
    part.flags = 0u;
    read.output = output;
    read.capacity = capacity;
    read.used = 0u;
    read.ended = read.invalid = 0u;
    reply.data_sink = path_read;
    reply.data_context = &read;
    reply.status = status;
    reply.status_capacity = sizeof(status);
    reply.status_length = 0u;
    status[0] = 0;
    result = dos->execute(&part, 1u, &reply);
    if (result != S64_UCI_OK || read.invalid) {
        output[0] = 0;
        return read.invalid ? S64_DOS_TARGET_ERROR : S64_DOS_TRANSPORT_ERROR;
    }
    if (reply.status_length < 2u || reply.status_length >= sizeof(status)
        || status[0] != '0' || status[1] != '0'
        || read.used == 0u || output[0] != '/') {
        output[0] = 0;
        return S64_DOS_TARGET_ERROR;
    }
    return S64_DOS_OK;
}
