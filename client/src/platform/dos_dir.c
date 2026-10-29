#include "platform/dos_dir.h"
#include "charset.h"
#include <string.h>

#define DOS_CMD_OPEN_DIR 0x13u
#define DOS_CMD_READ_DIR 0x14u

typedef struct {
    S64DosDirEntry entry;
    S64DosDirEntrySink on_entry;
    void* context;
    uint16_t bytes_in_packet;
    uint8_t name_length;
    uint8_t terminated;
    uint8_t malformed;
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

/* Each UCI packet is a DOS directory entry: one raw attribute byte and an
 * ASCII filename (possibly NUL terminated). Packet boundaries, not chunk
 * boundaries or repeated READ_DIR commands, delimit directory entries. */
static uint8_t on_packet(const uint8_t* data,
    uint16_t length, uint8_t final_chunk, void* context)
{
    DirRead* read = (DirRead*)context;
    uint16_t i;
    uint8_t native, byte;
    for (i = 0u; i < length; ++i) {
        byte = data[i];
        if (!read->bytes_in_packet) {
            read->entry.attributes = byte;
        } else if (read->terminated) {
            if (byte != 0u) read->malformed = 1u;
        } else if (byte == 0u) {
            read->terminated = 1u;
        } else {
            if (byte < 32u || byte > 126u
                || byte == 0x2fu || byte == 0x5cu
                || read->name_length >= S64_DOS_DIR_NAME_CAPACITY - 1u
                || !s64_charset_ascii_to_native(byte, &native)) {
                read->entry.skipped = 1u;
            } else if (!read->entry.skipped) {
                read->entry.name[read->name_length] = (char)native;
                read->entry.name[read->name_length + 1u] = 0;
            }
            if (read->name_length < 255u) ++read->name_length;
        }
        if (read->bytes_in_packet == 65535u) return 0u;
        ++read->bytes_in_packet;
    }
    if (!final_chunk) return 1u;
    if (read->malformed || read->bytes_in_packet < 2u
        || !read->name_length) return 0u;
    if (!read->entry.skipped
        && !read->on_entry(&read->entry, read->context)) return 0u;

    memset(&read->entry, 0, sizeof(read->entry));
    read->bytes_in_packet = 0u;
    read->name_length = read->terminated = read->malformed = 0u;
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

S64DosResult s64_dos_dir_read(
    S64Dos* dos, S64UciPacketExecuteFn execute_packets,
    S64DosDirEntrySink on_entry, void* context)
{
    uint8_t command[2];
    S64UciCommandPart part;
    S64UciReply reply;
    S64UciResult result;
    DirRead read;
    char status[48];

    if (!valid_dos(dos) || !execute_packets || !on_entry)
        return S64_DOS_INVALID_ARGUMENT;
    command[0] = dos->target;
    command[1] = DOS_CMD_READ_DIR;
    part.data = command;
    part.length = sizeof(command);
    part.flags = 0u;
    memset(&read, 0, sizeof(read));
    read.on_entry = on_entry;
    read.context = context;
    reply.data_sink = 0;
    reply.data_context = 0;
    reply.status = status;
    reply.status_capacity = sizeof(status);
    reply.status_length = 0u;
    result = execute_packets(&part, 1u, &reply, on_packet, &read);
    if (result != S64_UCI_OK) return transport_result(result);
    if (reply.status_length >= sizeof(status) || !success(&reply))
        return S64_DOS_TARGET_ERROR;
    return S64_DOS_OK;
}
