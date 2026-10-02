#include "dos.h"

#include <string.h>

#define S64_DOS_CMD_OPEN_FILE  0x02u
#define S64_DOS_CMD_CLOSE_FILE 0x03u
#define S64_DOS_CMD_READ_DATA  0x04u
#define S64_DOS_CMD_WRITE_DATA 0x05u
#define S64_DOS_CMD_FILE_STAT  0x08u
#define S64_DOS_CMD_DELETE     0x09u
#define S64_DOS_CMD_RENAME     0x0au
#define S64_DOS_CMD_CHANGE_DIR 0x11u
#define S64_DOS_CMD_CREATE_DIR 0x16u

#define S64_DOS_ATTR_READ 0x01u
/* WRITE | CREATE_ALWAYS; CREATE_NEW makes existing files fail on firmware 1.1. */
#define S64_DOS_ATTR_OVERWRITE 0x0au

#define S64_DOS_STATUS_CAPACITY 64u

typedef struct {
    S64UciDataSink sink;
    void* context;
    uint16_t count;
} S64DosReadSink;

static uint8_t target_valid(uint8_t target)
{
    return target == S64_DOS_TARGET_PRIMARY
        || target == S64_DOS_TARGET_SECONDARY;
}

static uint8_t text_length(
    const char* text,
    uint16_t maximum,
    uint16_t* length
)
{
    uint16_t i = 0u;

    if (text == 0 || text[0] == '\0' || length == 0) {
        return 0u;
    }

    while (text[i] != '\0') {
        if (i >= maximum
            || text[i] == '\r' || text[i] == '\n') {
            return 0u;
        }
        ++i;
    }

    *length = i;
    return 1u;
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

    /* Two decimal digits need only a byte; keep resident DOS code compact. */
    return (uint8_t)(
        (uint8_t)(status[0] - '0') * 10u
        + (uint8_t)(status[1] - '0')
    );
}

static S64DosResult map_status(const char* status, uint16_t length)
{
    uint8_t code;

    if (status_ok_or_empty(status, length)) {
        return S64_DOS_OK;
    }

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

static S64DosResult run_status_command(
    S64Dos* dos,
    const S64UciCommandPart* parts,
    uint8_t part_count,
    uint8_t allow_empty_status
)
{
    S64UciReply reply;
    S64UciResult result;
    char status[S64_DOS_STATUS_CAPACITY];

    if (dos == 0 || dos->execute == 0 || !target_valid(dos->target)) {
        return S64_DOS_INVALID_ARGUMENT;
    }

    reply.data_sink = 0;
    reply.data_context = 0;
    reply.status = status;
    reply.status_capacity = (uint16_t)sizeof(status);
    reply.status_length = 0u;

    result = dos->execute(parts, part_count, &reply);
    if (result != S64_UCI_OK) {
        return map_transport(result);
    }

    if (!allow_empty_status && reply.status_length == 0u) {
        return S64_DOS_TARGET_ERROR;
    }

    return map_status(status, reply.status_length);
}

static S64DosResult open_file(
    S64Dos* dos,
    const char* filename,
    uint8_t attributes
)
{
    uint8_t prefix[3];
    uint16_t filename_length;
    S64UciCommandPart parts[2];

    if (dos == 0 || dos->execute == 0 || !target_valid(dos->target)
        || !text_length(filename, S64_DOS_FILENAME_MAX, &filename_length)) {
        return S64_DOS_INVALID_ARGUMENT;
    }

    prefix[0] = dos->target;
    prefix[1] = S64_DOS_CMD_OPEN_FILE;
    prefix[2] = attributes;

    parts[0].data = prefix;
    parts[0].length = (uint16_t)sizeof(prefix);
    parts[0].flags = 0u;
    parts[1].data = (const uint8_t*)filename;
    parts[1].length = filename_length;
    parts[1].flags = S64_UCI_PART_NATIVE_TEXT;

    return run_status_command(dos, parts, 2u, 0u);
}

static S64DosResult filename_command(
    S64Dos* dos,
    uint8_t opcode,
    const char* filename
)
{
    uint8_t prefix[2];
    uint16_t filename_length;
    S64UciCommandPart parts[2];

    if (dos == 0 || dos->execute == 0 || !target_valid(dos->target)
        || !text_length(filename, S64_DOS_FILENAME_MAX, &filename_length)) {
        return S64_DOS_INVALID_ARGUMENT;
    }

    prefix[0] = dos->target;
    prefix[1] = opcode;

    /* FILE_STAT consumes a C string without terminating the command buffer.
     * Include the filename's NUL so earlier, longer commands cannot leak in. */
    if (opcode == S64_DOS_CMD_FILE_STAT) ++filename_length;

    parts[0].data = prefix;
    parts[0].length = (uint16_t)sizeof(prefix);
    parts[0].flags = 0u;
    parts[1].data = (const uint8_t*)filename;
    parts[1].length = filename_length;
    parts[1].flags = S64_UCI_PART_NATIVE_TEXT;

    return run_status_command(dos, parts, 2u, 0u);
}

static uint8_t read_sink(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64DosReadSink* state = (S64DosReadSink*)context;

    if ((uint32_t)state->count + length > 65535UL) {
        return 0u;
    }

    if (state->sink != 0
        && !state->sink(data, length, state->context)) {
        return 0u;
    }

    state->count = (uint16_t)(state->count + length);
    return 1u;
}

void s64_dos_init(S64Dos* dos, S64UciExecuteFn execute, uint8_t target)
{
    if (dos == 0) return;

    dos->execute = execute;
    dos->target = target_valid(target) ? target : 0u;
}

const char* s64_dos_result_message(S64DosResult result)
{
    switch (result) {
        case S64_DOS_OK:
            return "OK";
        case S64_DOS_INVALID_ARGUMENT:
            return "Invalid file request";
        case S64_DOS_TRANSPORT_ERROR:
            return "Ultimate DOS unavailable";
        case S64_DOS_CONSUMER_ABORTED:
            return "File read stopped";
        case S64_DOS_NOT_FOUND:
            return "File not found";
        case S64_DOS_ACCESS_DENIED:
            return "File access denied";
        case S64_DOS_NO_FILE_OPEN:
            return "No file open";
        case S64_DOS_TARGET_ERROR:
        default:
            return "Ultimate DOS error";
    }
}

S64DosResult s64_dos_open_read(S64Dos* dos, const char* filename)
{
    return open_file(dos, filename, S64_DOS_ATTR_READ);
}

S64DosResult s64_dos_open_write(S64Dos* dos, const char* filename)
{
    return open_file(dos, filename, S64_DOS_ATTR_OVERWRITE);
}

S64DosResult s64_dos_close(S64Dos* dos)
{
    uint8_t command[2];
    S64UciCommandPart part;

    if (dos == 0 || !target_valid(dos->target)) {
        return S64_DOS_INVALID_ARGUMENT;
    }

    command[0] = dos->target;
    command[1] = S64_DOS_CMD_CLOSE_FILE;
    part.data = command;
    part.length = (uint16_t)sizeof(command);
    part.flags = 0u;

    return run_status_command(dos, &part, 1u, 0u);
}

S64DosResult s64_dos_read(
    S64Dos* dos,
    uint16_t requested,
    S64UciDataSink sink,
    void* sink_context,
    uint16_t* bytes_read
)
{
    uint8_t command[4];
    S64UciCommandPart part;
    S64UciReply reply;
    S64UciResult transport;
    S64DosReadSink read_state;
    char status[S64_DOS_STATUS_CAPACITY];

    if (bytes_read != 0) *bytes_read = 0u;

    if (dos == 0 || dos->execute == 0 || !target_valid(dos->target)
        || requested == 0u) {
        return S64_DOS_INVALID_ARGUMENT;
    }

    command[0] = dos->target;
    command[1] = S64_DOS_CMD_READ_DATA;
    command[2] = (uint8_t)(requested & 0xffu);
    command[3] = (uint8_t)(requested >> 8);

    part.data = command;
    part.length = (uint16_t)sizeof(command);
    part.flags = 0u;

    read_state.sink = sink;
    read_state.context = sink_context;
    read_state.count = 0u;

    reply.data_sink = read_sink;
    reply.data_context = &read_state;
    reply.status = status;
    reply.status_capacity = (uint16_t)sizeof(status);
    reply.status_length = 0u;

    transport = dos->execute(&part, 1u, &reply);
    if (bytes_read != 0) *bytes_read = read_state.count;

    if (transport != S64_UCI_OK) {
        return map_transport(transport);
    }

    return map_status(status, reply.status_length);
}

S64DosResult s64_dos_write(
    S64Dos* dos,
    const uint8_t* data,
    uint16_t length
)
{
    uint8_t prefix[4];
    uint16_t offset = 0u;
    uint16_t chunk;
    S64UciCommandPart parts[2];
    S64DosResult result;

    if (dos == 0 || dos->execute == 0 || !target_valid(dos->target)
        || (length != 0u && data == 0)) {
        return S64_DOS_INVALID_ARGUMENT;
    }

    if (length == 0u) return S64_DOS_OK;

    prefix[0] = dos->target;
    prefix[1] = S64_DOS_CMD_WRITE_DATA;
    prefix[2] = 0u;
    prefix[3] = 0u;

    parts[0].data = prefix;
    parts[0].length = (uint16_t)sizeof(prefix);
    parts[0].flags = 0u;

    while (offset < length) {
        chunk = (uint16_t)(length - offset);
        if (chunk > S64_DOS_WRITE_CHUNK) {
            chunk = S64_DOS_WRITE_CHUNK;
        }

        parts[1].data = data + offset;
        parts[1].length = chunk;
        parts[1].flags = 0u;

        result = run_status_command(dos, parts, 2u, 1u);
        if (result != S64_DOS_OK) {
            return result;
        }

        offset = (uint16_t)(offset + chunk);
    }

    return S64_DOS_OK;
}

S64DosResult s64_dos_file_exists(
    S64Dos* dos,
    const char* filename,
    uint8_t* exists
)
{
    S64DosResult result;

    if (exists == 0) {
        return S64_DOS_INVALID_ARGUMENT;
    }
    *exists = 0u;

    result = filename_command(dos, S64_DOS_CMD_FILE_STAT, filename);
    if (result == S64_DOS_OK) {
        *exists = 1u;
        return S64_DOS_OK;
    }
    if (result == S64_DOS_NOT_FOUND) {
        return S64_DOS_OK;
    }
    return result;
}

S64DosResult s64_dos_delete_file(S64Dos* dos, const char* filename)
{
    return filename_command(dos, S64_DOS_CMD_DELETE, filename);
}

S64DosResult s64_dos_rename_file(
    S64Dos* dos,
    const char* filename,
    const char* new_name
)
{
    uint8_t prefix[2];
    uint8_t separator = 0u;
    uint16_t old_length;
    uint16_t new_length;
    S64UciCommandPart parts[4];

    if (dos == 0 || dos->execute == 0 || !target_valid(dos->target)
        || !text_length(filename, S64_DOS_FILENAME_MAX, &old_length)
        || !text_length(new_name, S64_DOS_FILENAME_MAX, &new_length)
        || (uint32_t)old_length + new_length + 3UL > S64_UCI_COMMAND_CAPACITY) {
        return S64_DOS_INVALID_ARGUMENT;
    }

    prefix[0] = dos->target;
    prefix[1] = S64_DOS_CMD_RENAME;

    parts[0].data = prefix;
    parts[0].length = (uint16_t)sizeof(prefix);
    parts[0].flags = 0u;
    parts[1].data = (const uint8_t*)filename;
    parts[1].length = old_length;
    parts[1].flags = S64_UCI_PART_NATIVE_TEXT;
    parts[2].data = &separator;
    parts[2].length = 1u;
    parts[2].flags = 0u;
    parts[3].data = (const uint8_t*)new_name;
    parts[3].length = new_length;
    parts[3].flags = S64_UCI_PART_NATIVE_TEXT;

    return run_status_command(dos, parts, 4u, 0u);
}

S64DosResult s64_dos_change_dir(S64Dos* dos, const char* directory)
{
    return filename_command(dos, S64_DOS_CMD_CHANGE_DIR, directory);
}

S64DosResult s64_dos_create_dir(S64Dos* dos, const char* directory)
{
    return filename_command(dos, S64_DOS_CMD_CREATE_DIR, directory);
}
