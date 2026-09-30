#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/dos.h"

#define MOCK_MAX_COMMANDS 8u
#define MOCK_MAX_BYTES 896u

typedef struct {
    unsigned int count;
    unsigned int lengths[MOCK_MAX_COMMANDS];
    unsigned char part_counts[MOCK_MAX_COMMANDS];
    unsigned char part_flags[MOCK_MAX_COMMANDS][4];
    unsigned char bytes[MOCK_MAX_COMMANDS][MOCK_MAX_BYTES];
    const char* status;
    unsigned char response[1024];
    unsigned int response_length;
    S64UciResult result;
} MockUci;

static MockUci mock;

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void mock_reset(void)
{
    memset(&mock, 0, sizeof(mock));
    mock.status = "00,OK";
    mock.result = S64_UCI_OK;
}

static S64UciResult mock_execute(
    const S64UciCommandPart* parts,
    uint8_t part_count,
    S64UciReply* reply
)
{
    unsigned int command_index = mock.count;
    unsigned int offset = 0u;
    unsigned int i;
    unsigned int j;
    size_t status_length;

    require(command_index < MOCK_MAX_COMMANDS, "mock command capacity");
    require(part_count <= 4u, "mock part capacity");
    mock.part_counts[command_index] = part_count;

    for (i = 0u; i < part_count; ++i) {
        mock.part_flags[command_index][i] = parts[i].flags;
        require(offset + parts[i].length <= MOCK_MAX_BYTES, "mock byte capacity");
        for (j = 0u; j < parts[i].length; ++j) {
            mock.bytes[command_index][offset++] = parts[i].data[j];
        }
    }

    mock.lengths[command_index] = offset;
    ++mock.count;

    if (reply != NULL) {
        status_length = strlen(mock.status);
        reply->status_length = (uint16_t)status_length;

        if (reply->status != NULL && reply->status_capacity != 0u) {
            size_t copy = status_length;
            if (copy >= reply->status_capacity) {
                copy = reply->status_capacity - 1u;
            }
            memcpy(reply->status, mock.status, copy);
            reply->status[copy] = '\0';
        }

        if (mock.response_length != 0u && reply->data_sink != NULL) {
            if (!reply->data_sink(
                    mock.response,
                    (uint16_t)mock.response_length,
                    reply->data_context
                )) {
                return S64_UCI_CONSUMER_ABORTED;
            }
        }
    }

    return mock.result;
}

typedef struct {
    unsigned char bytes[1024];
    unsigned int length;
    unsigned char accept;
} CollectSink;

static uint8_t collect(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    CollectSink* sink = (CollectSink*)context;

    if (!sink->accept || sink->length + length > sizeof(sink->bytes)) {
        return 0u;
    }

    memcpy(sink->bytes + sink->length, data, length);
    sink->length += length;
    return 1u;
}

int main(void)
{
    S64Dos dos;
    S64DosResult result;
    CollectSink sink;
    unsigned char payload[1000];
    uint16_t bytes_read = 0u;
    uint8_t exists = 0u;
    unsigned int i;

    s64_dos_init(&dos, mock_execute, S64_DOS_TARGET_PRIMARY);

    mock_reset();
    result = s64_dos_open_read(&dos, "drafts/hello.s64");
    require(result == S64_DOS_OK, "open read succeeds");
    require(mock.count == 1u, "open read command count");
    require(mock.bytes[0][0] == 0x01u && mock.bytes[0][1] == 0x02u, "open read target/opcode");
    require(mock.bytes[0][2] == 0x01u, "open read attribute");
    require(mock.part_counts[0] == 2u, "open read part count");
    require(mock.part_flags[0][0] == 0u, "open prefix is binary");
    require(mock.part_flags[0][1] == S64_UCI_PART_NATIVE_TEXT, "open filename is text");
    require(
        memcmp(mock.bytes[0] + 3u, "drafts/hello.s64", 16u) == 0,
        "open read filename"
    );

    mock_reset();
    result = s64_dos_open_write(&dos, "drafts/hello.s64");
    require(result == S64_DOS_OK, "open overwrite succeeds");
    require(mock.bytes[0][2] == 0x0eu, "overwrite/create attribute");

    mock_reset();
    memcpy(mock.response, "hello world", 11u);
    mock.response_length = 11u;
    mock.status = "";
    memset(&sink, 0, sizeof(sink));
    sink.accept = 1u;
    result = s64_dos_read(&dos, 700u, collect, &sink, &bytes_read);
    require(result == S64_DOS_OK, "stream read succeeds with quiet success status");
    require(mock.lengths[0] == 4u, "read command length");
    require(mock.bytes[0][0] == 0x01u && mock.bytes[0][1] == 0x04u, "read target/opcode");
    require(mock.bytes[0][2] == 0xbcu && mock.bytes[0][3] == 0x02u, "read length little endian");
    require(bytes_read == 11u && sink.length == 11u, "read byte count");
    require(memcmp(sink.bytes, "hello world", 11u) == 0, "read data streamed");

    mock_reset();
    memcpy(mock.response, "abc", 3u);
    mock.response_length = 3u;
    mock.status = "";
    memset(&sink, 0, sizeof(sink));
    sink.accept = 0u;
    result = s64_dos_read(&dos, 3u, collect, &sink, &bytes_read);
    require(result == S64_DOS_CONSUMER_ABORTED, "consumer abort remains distinguishable");

    for (i = 0u; i < sizeof(payload); ++i) {
        payload[i] = (unsigned char)(i & 0xffu);
    }
    mock_reset();
    mock.status = "";
    result = s64_dos_write(&dos, payload, (uint16_t)sizeof(payload));
    require(result == S64_DOS_OK, "chunked write succeeds");
    require(mock.count == 2u, "1000-byte write uses two commands");
    require(mock.lengths[0] == 516u, "first write command uses 512-byte payload");
    require(mock.lengths[1] == 492u, "second write command contains remainder");
    require(mock.bytes[0][0] == 0x01u && mock.bytes[0][1] == 0x05u, "write target/opcode");
    require(mock.bytes[0][2] == 0u && mock.bytes[0][3] == 0u, "write alignment bytes");
    require(memcmp(mock.bytes[0] + 4u, payload, 512u) == 0, "first write payload");
    require(memcmp(mock.bytes[1] + 4u, payload + 512u, 488u) == 0, "second write payload");
    require(mock.part_flags[0][0] == 0u && mock.part_flags[0][1] == 0u,
        "file data remains binary");

    mock_reset();
    result = s64_dos_close(&dos);
    require(result == S64_DOS_OK, "close succeeds");
    require(mock.lengths[0] == 2u, "close command length");
    require(mock.bytes[0][1] == 0x03u, "close opcode");

    mock_reset();
    result = s64_dos_file_exists(&dos, "old.s64", &exists);
    require(result == S64_DOS_OK && exists == 1u, "existing file stat succeeds");
    require(mock.bytes[0][1] == 0x08u, "file stat opcode");
    require(mock.lengths[0] == 10u, "stat includes the filename terminator");
    require(memcmp(mock.bytes[0] + 2u, "old.s64", 8u) == 0,
        "stat filename ends before stale command buffer bytes");
    require(mock.part_flags[0][1] == S64_UCI_PART_NATIVE_TEXT,
        "stat filename including NUL remains native text");

    mock_reset();
    mock.status = "82,FILE NOT FOUND";
    exists = 1u;
    result = s64_dos_file_exists(&dos, "missing.s64", &exists);
    require(result == S64_DOS_OK && exists == 0u,
        "firmware 1.1 missing stat maps to normal absence");

    mock_reset();
    mock.status = "88,FILE NOT FOUND";
    exists = 1u;
    result = s64_dos_file_exists(&dos, "missing.s64", &exists);
    require(result == S64_DOS_OK && exists == 0u, "missing file stat is normal absence");

    mock_reset();
    result = s64_dos_delete_file(&dos, "old.s64");
    require(result == S64_DOS_OK, "delete succeeds");
    require(mock.bytes[0][1] == 0x09u, "delete opcode");
    require(memcmp(mock.bytes[0] + 2u, "old.s64", 7u) == 0, "delete filename");

    mock_reset();
    result = s64_dos_rename_file(&dos, "old.s64", "new.s64");
    require(result == S64_DOS_OK, "rename succeeds");
    require(mock.bytes[0][1] == 0x0au, "rename opcode");
    require(memcmp(mock.bytes[0] + 2u, "old.s64", 7u) == 0, "rename old name");
    require(mock.bytes[0][9] == 0u, "rename separator");
    require(memcmp(mock.bytes[0] + 10u, "new.s64", 7u) == 0, "rename new name");
    require(mock.part_counts[0] == 4u, "rename part count");
    require(mock.part_flags[0][1] == S64_UCI_PART_NATIVE_TEXT, "rename old name is text");
    require(mock.part_flags[0][2] == 0u, "rename separator is binary");
    require(mock.part_flags[0][3] == S64_UCI_PART_NATIVE_TEXT, "rename new name is text");

    mock_reset();
    result = s64_dos_change_dir(&dos, "SEE64YOU");
    require(result == S64_DOS_OK, "change directory succeeds");
    require(mock.bytes[0][1] == 0x11u, "change directory opcode");

    mock_reset();
    result = s64_dos_create_dir(&dos, "SEE64YOU");
    require(result == S64_DOS_OK, "create directory succeeds");
    require(mock.bytes[0][1] == 0x16u, "create directory opcode");

    mock_reset();
    mock.status = "88,FILE NOT FOUND";
    require(
        s64_dos_open_read(&dos, "missing.s64") == S64_DOS_NOT_FOUND,
        "file not found status mapped"
    );

    mock_reset();
    mock.status = "87,ACCESS DENIED";
    require(
        s64_dos_open_write(&dos, "locked.s64") == S64_DOS_ACCESS_DENIED,
        "access denied status mapped"
    );

    mock_reset();
    mock.status = "85,NO FILE OPEN";
    require(
        s64_dos_write(&dos, payload, 1u) == S64_DOS_NO_FILE_OPEN,
        "no-open-file status mapped"
    );

    mock_reset();
    mock.result = S64_UCI_TIMEOUT;
    require(
        s64_dos_close(&dos) == S64_DOS_TRANSPORT_ERROR,
        "transport timeout mapped"
    );

    mock_reset();
    mock.status = "99,UNKNOWN COMMAND";
    require(s64_dos_close(&dos) == S64_DOS_TARGET_ERROR,
        "largest two-digit status remains a target error");
    mock.status = "9X,MALFORMED";
    require(s64_dos_close(&dos) == S64_DOS_TARGET_ERROR,
        "malformed status sentinel remains a target error");

    mock_reset();
    require(
        s64_dos_open_read(&dos, "bad\nname.s64") == S64_DOS_INVALID_ARGUMENT,
        "control characters in filenames rejected"
    );

    s64_dos_init(&dos, mock_execute, 0x06u);
    require(
        s64_dos_open_read(&dos, "bad.s64") == S64_DOS_INVALID_ARGUMENT,
        "invalid UCI target rejected"
    );

    puts("ultimate dos tests passed");
    return 0;
}
