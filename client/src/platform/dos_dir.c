#include "platform/dos_dir.h"
#include "charset.h"
#include <string.h>

#define DOS_CMD_OPEN_DIR 0x13u
#define DOS_CMD_READ_DIR 0x14u

typedef struct {
    S64DosDirEntry* entry;
    uint16_t count;
    uint8_t ended;
} DirRead;

static uint8_t valid_dos(const S64Dos* dos)
{
    return dos && dos->execute
        && (dos->target == S64_DOS_TARGET_PRIMARY
            || dos->target == S64_DOS_TARGET_SECONDARY);
}

static uint8_t success(const S64UciReply* reply)
{
    return reply->status_length == 0u
        || (reply->status_length >= 2u
            && reply->status[0] == '0' && reply->status[1] == '0');
}

static S64DosResult transport_result(S64UciResult result)
{
    return result == S64_UCI_CONSUMER_ABORTED
        ? S64_DOS_CONSUMER_ABORTED : S64_DOS_TRANSPORT_ERROR;
}

static uint8_t directory_entry(const uint8_t* data,
    uint16_t length, void* context)
{
    DirRead* read = (DirRead*)context;
    uint16_t i;
    uint8_t native;
    for (i = 0u; i < length; ++i) {
        if (read->count == 0u) {
            read->entry->attributes = data[i];
        } else if (read->ended) {
            /* Some firmware replies include the trailing terminator only. */
            if (data[i] != 0u) return 0u;
        } else if (data[i] == 0u) {
            read->ended = 1u;
        } else if (!read->entry->skipped) {
            if (read->count >= S64_DOS_DIR_NAME_CAPACITY
                || data[i] < 32u || data[i] > 126u
                || data[i] == '/' || data[i] == '\\'
                || !s64_charset_ascii_to_native(data[i], &native)) {
                read->entry->skipped = 1u;
            } else {
                read->entry->name[read->count - 1u] = (char)native;
                read->entry->name[read->count] = 0;
            }
        }
        if (read->count == 65535u) return 0u;
        ++read->count;
    }
    return 1u;
}

S64DosResult s64_dos_dir_open(S64Dos* dos, uint8_t* empty)
{
    uint8_t command[2];
    S64UciCommandPart part;
    S64UciReply reply;
    S64UciResult result;
    char status[48];

    if (empty) *empty = 0u;
    if (!valid_dos(dos) || !empty) return S64_DOS_INVALID_ARGUMENT;
    command[0] = dos->target;
    command[1] = DOS_CMD_OPEN_DIR;
    part.data = command;
    part.length = sizeof(command);
    part.flags = 0u;
    reply.data_sink = 0;
    reply.data_context = 0;
    reply.status = status;
    reply.status_capacity = sizeof(status);
    reply.status_length = 0u;
    result = dos->execute(&part, 1u, &reply);
    if (result != S64_UCI_OK) return transport_result(result);
    if (reply.status_length < 2u || reply.status_length >= sizeof(status))
        return S64_DOS_TARGET_ERROR;
    if (status[0] == '0' && status[1] == '1') {
        *empty = 1u;
        return S64_DOS_OK;
    }
    return success(&reply) ? S64_DOS_OK : S64_DOS_TARGET_ERROR;
}

S64DosResult s64_dos_dir_next(S64Dos* dos,
    S64DosDirEntry* entry, uint8_t* found)
{
    uint8_t command[2];
    S64UciCommandPart part;
    S64UciReply reply;
    S64UciResult result;
    DirRead read;
    char status[48];

    if (found) *found = 0u;
    if (entry) memset(entry, 0, sizeof(*entry));
    if (!valid_dos(dos) || !entry || !found)
        return S64_DOS_INVALID_ARGUMENT;
    command[0] = dos->target;
    command[1] = DOS_CMD_READ_DIR;
    part.data = command;
    part.length = sizeof(command);
    part.flags = 0u;
    read.entry = entry;
    read.count = 0u;
    read.ended = 0u;
    reply.data_sink = directory_entry;
    reply.data_context = &read;
    reply.status = status;
    reply.status_capacity = sizeof(status);
    reply.status_length = 0u;
    result = dos->execute(&part, 1u, &reply);
    if (result != S64_UCI_OK) return transport_result(result);
    if (reply.status_length >= sizeof(status) || !success(&reply))
        return S64_DOS_TARGET_ERROR;
    if (!read.count) return S64_DOS_OK;
    if (read.count < 2u) return S64_DOS_TARGET_ERROR;
    *found = 1u;
    if (!entry->name[0]) entry->skipped = 1u;
    return S64_DOS_OK;
}
