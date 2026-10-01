#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/companion_loader.h"

#define MOCK_MAX_COMMANDS 8u
#define MOCK_MAX_BYTES 64u

typedef struct {
    unsigned int count;
    unsigned int lengths[MOCK_MAX_COMMANDS];
    unsigned char bytes[MOCK_MAX_COMMANDS][MOCK_MAX_BYTES];
    uint32_t file_size;
    uint8_t header[2];
    uint8_t fail_opcode;
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
    mock.file_size = 48133UL;
    mock.header[0] = 0x01u;
    mock.header[1] = 0x08u;
}

static void set_status(S64UciReply* reply, const char* status)
{
    size_t length;
    size_t copy;

    if (reply == 0) return;
    length = strlen(status);
    reply->status_length = (uint16_t)length;
    if (reply->status == 0 || reply->status_capacity == 0u) return;
    copy = length;
    if (copy >= reply->status_capacity) copy = reply->status_capacity - 1u;
    memcpy(reply->status, status, copy);
    reply->status[copy] = '\0';
}

static S64UciResult mock_execute(
    const S64UciCommandPart* parts,
    uint8_t part_count,
    S64UciReply* reply
)
{
    unsigned int command = mock.count;
    unsigned int offset = 0u;
    unsigned int part;
    unsigned int i;
    uint8_t opcode;

    require(parts != 0 && part_count != 0u, "mock command present");
    require(command < MOCK_MAX_COMMANDS, "mock command capacity");
    opcode = parts[0].data[1];
    for (part = 0u; part < part_count; ++part) {
        require(offset + parts[part].length <= MOCK_MAX_BYTES,
            "mock byte capacity");
        for (i = 0u; i < parts[part].length; ++i) {
            mock.bytes[command][offset++] = parts[part].data[i];
        }
    }
    mock.lengths[command] = offset;
    ++mock.count;

    if (opcode == mock.fail_opcode) {
        set_status(reply, "99,TEST FAILURE");
        return S64_UCI_OK;
    }

    set_status(reply, opcode == 0x04u ? "" : "00,OK");
    if (reply != 0 && reply->data_sink != 0) {
        if (opcode == 0x07u) {
            uint8_t info[12];
            memset(info, 0, sizeof(info));
            info[0] = (uint8_t)(mock.file_size & 0xffu);
            info[1] = (uint8_t)((mock.file_size >> 8) & 0xffu);
            info[2] = (uint8_t)((mock.file_size >> 16) & 0xffu);
            info[3] = (uint8_t)((mock.file_size >> 24) & 0xffu);
            if (!reply->data_sink(info, sizeof(info), reply->data_context)) {
                return S64_UCI_CONSUMER_ABORTED;
            }
        } else if (opcode == 0x04u) {
            require(parts[0].length == 4u
                && parts[0].data[2] == 2u && parts[0].data[3] == 0u,
                "preflight reads two-byte PRG header");
            if (!reply->data_sink(mock.header, 2u, reply->data_context)) {
                return S64_UCI_CONSUMER_ABORTED;
            }
        }
    }
    return S64_UCI_OK;
}

static unsigned int find_opcode(uint8_t opcode)
{
    unsigned int i;
    for (i = 0u; i < mock.count; ++i) {
        if (mock.lengths[i] >= 2u && mock.bytes[i][1] == opcode) return i;
    }
    return MOCK_MAX_COMMANDS;
}

static void require_open_name(const char* name)
{
    unsigned int i = find_opcode(0x02u);
    require(i < MOCK_MAX_COMMANDS, "open command present");
    require(mock.lengths[i] == 3u + strlen(name), "open command length");
    require(memcmp(mock.bytes[i] + 3u, name, strlen(name)) == 0,
        "exact companion filename");
}

int main(void)
{
    S64Dos dos;
    S64CompanionImage image;
    S64CompanionResult result;

    s64_dos_init(&dos, mock_execute, S64_DOS_TARGET_PRIMARY);

    mock_reset();
    result = s64_companion_open_preflight(
        &dos, S64_COMPANION_PUBLISHER, &image);
    require(result == S64_COMPANION_OK, "publisher preflight succeeds");
    require_open_name("S64PUB.PRG");
    require(image.load_address == 0x0801u, "publisher load address");
    require(image.payload_size == 48131u, "publisher payload size");
    require(image.load_end == 0xc403u, "publisher load end");
    require(find_opcode(0x03u) == MOCK_MAX_COMMANDS,
        "success leaves file open at payload");

    mock_reset();
    result = s64_companion_open_preflight(
        &dos, S64_COMPANION_EDITOR, &image);
    require(result == S64_COMPANION_OK, "editor preflight succeeds");
    require_open_name("S64EDIT.PRG");

    mock_reset();
    mock.file_size = 27536UL;
    result = s64_companion_open_preflight(
        &dos, S64_COMPANION_SETTINGS, &image);
    require(result == S64_COMPANION_OK, "settings preflight succeeds");
    require_open_name("S64CFG.PRG");

    mock_reset();
    mock.header[0] = 0x00u;
    result = s64_companion_open_preflight(
        &dos, S64_COMPANION_PUBLISHER, &image);
    require(result == S64_COMPANION_INVALID_IMAGE,
        "wrong PRG load address rejected");
    require(find_opcode(0x03u) < MOCK_MAX_COMMANDS,
        "invalid image closes file");

    mock_reset();
    mock.file_size = 2UL + (0xc800UL - 0x0801UL) + 1UL;
    result = s64_companion_open_preflight(
        &dos, S64_COMPANION_PUBLISHER, &image);
    require(result == S64_COMPANION_INVALID_IMAGE,
        "image overlapping resident loader rejected");

    mock_reset();
    mock.file_size = 4096UL;
    result = s64_companion_open_named_preflight(
        &dos, "S64LDTST.PRG", &image);
    require(result == S64_COMPANION_OK,
        "named diagnostic target preflight succeeds");
    require_open_name("S64LDTST.PRG");

    mock_reset();
    mock.file_size = 14UL;
    result = s64_companion_open_preflight(
        &dos, S64_COMPANION_PUBLISHER, &image);
    require(result == S64_COMPANION_INVALID_IMAGE,
        "image without cc65 startup rejected");

    mock_reset();
    mock.fail_opcode = 0x07u;
    result = s64_companion_open_preflight(
        &dos, S64_COMPANION_PUBLISHER, &image);
    require(result == S64_COMPANION_DOS_ERROR
        && image.dos_result == S64_DOS_TARGET_ERROR,
        "file-info error preserved");
    require(find_opcode(0x03u) < MOCK_MAX_COMMANDS,
        "metadata failure closes file");

    mock_reset();
    mock.fail_opcode = 0x11u;
    result = s64_companion_open_preflight(
        &dos, S64_COMPANION_PUBLISHER, &image);
    require(result == S64_COMPANION_DOS_ERROR,
        "path failure propagated");
    require(find_opcode(0x03u) == MOCK_MAX_COMMANDS,
        "path failure does not close unopened file");

    require(s64_companion_filename(S64_COMPANION_COUNT) == 0,
        "invalid companion has no filename");

    puts("companion loader preflight tests passed");
    return 0;
}
