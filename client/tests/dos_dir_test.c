#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "platform/dos_dir.h"

static const uint8_t* response_data;
static uint16_t response_size;
static const char* response_status;
static S64UciResult response_result;
static uint8_t expected_opcode;

static S64UciResult fake_uci(const S64UciCommandPart* parts,
    uint8_t part_count, S64UciReply* reply)
{
    uint16_t length;
    assert(part_count == 1u);
    assert(parts[0].length == 2u);
    assert(parts[0].data[0] == S64_DOS_TARGET_PRIMARY);
    assert(parts[0].data[1] == expected_opcode);
    if (response_result != S64_UCI_OK) return response_result;

    length = (uint16_t)strlen(response_status);
    assert(length < reply->status_capacity);
    memcpy(reply->status, response_status, length + 1u);
    reply->status_length = length;
    if (response_size && reply->data_sink) {
        /* Exercise both halves of a split UCI response. */
        if (response_size > 3u) {
            assert(reply->data_sink(response_data, 3u, reply->data_context));
            assert(reply->data_sink(response_data + 3u,
                (uint16_t)(response_size - 3u), reply->data_context));
        } else {
            assert(reply->data_sink(response_data, response_size,
                reply->data_context));
        }
    }
    return S64_UCI_OK;
}

static void set_reply(uint8_t opcode, const char* status,
    const uint8_t* data, uint16_t length)
{
    expected_opcode = opcode;
    response_status = status;
    response_data = data;
    response_size = length;
    response_result = S64_UCI_OK;
}

int main(void)
{
    S64Dos dos;
    S64DosDirEntry entry;
    uint8_t found, empty;
    uint8_t long_name[36];
    static const uint8_t file[] = {
        0x20u, 'I','N','D','E','X','.','8','T','M','L',0u
    };
    static const uint8_t folder[] = {
        0x10u, 'p','a','g','e','s',0u
    };
    static const uint8_t malformed[] = {0x20u, 'a', '/', 'b', 0u};
    static const uint8_t attribute_only[] = {0x20u};
    uint8_t i;

    s64_dos_init(&dos, fake_uci, S64_DOS_TARGET_PRIMARY);
    set_reply(0x13u, "01,DIRECTORY EMPTY", 0, 0u);
    assert(s64_dos_dir_open(&dos, &empty) == S64_DOS_OK);
    assert(empty == 1u);

    set_reply(0x13u, "00,OK", 0, 0u);
    assert(s64_dos_dir_open(&dos, &empty) == S64_DOS_OK);
    assert(empty == 0u);

    set_reply(0x14u, "00,OK", file, sizeof(file));
    assert(s64_dos_dir_next(&dos, &entry, &found) == S64_DOS_OK);
    assert(found == 1u && !entry.skipped);
    assert(entry.attributes == 0x20u);
    assert(!strcmp(entry.name, "INDEX.8TML"));

    set_reply(0x14u, "00,OK", folder, sizeof(folder));
    assert(s64_dos_dir_next(&dos, &entry, &found) == S64_DOS_OK);
    assert(found == 1u && !entry.skipped);
    assert(entry.attributes == S64_DOS_DIR_ATTRIBUTE_DIRECTORY);
    assert(!strcmp(entry.name, "pages"));

    long_name[0] = 0x20u;
    for (i = 1u; i < sizeof(long_name)-1u; ++i) long_name[i] = 'x';
    long_name[sizeof(long_name)-1u] = 0u;
    set_reply(0x14u, "00,OK", long_name, sizeof(long_name));
    assert(s64_dos_dir_next(&dos, &entry, &found) == S64_DOS_OK);
    assert(found == 1u && entry.skipped);

    set_reply(0x14u, "00,OK", malformed, sizeof(malformed));
    assert(s64_dos_dir_next(&dos, &entry, &found) == S64_DOS_OK);
    assert(found == 1u && entry.skipped);

    set_reply(0x14u, "00,OK", 0, 0u);
    assert(s64_dos_dir_next(&dos, &entry, &found) == S64_DOS_OK);
    assert(found == 0u);

    set_reply(0x14u, "00,OK", attribute_only, sizeof(attribute_only));
    assert(s64_dos_dir_next(&dos, &entry, &found) == S64_DOS_TARGET_ERROR);

    set_reply(0x13u, "86,CAN'T READ DIRECTORY", 0, 0u);
    assert(s64_dos_dir_open(&dos, &empty) == S64_DOS_TARGET_ERROR);

    set_reply(0x14u, "85,NO FILE OPEN", 0, 0u);
    assert(s64_dos_dir_next(&dos, &entry, &found) == S64_DOS_TARGET_ERROR);

    set_reply(0x14u, "00,OK", file, sizeof(file));
    response_result = S64_UCI_TIMEOUT;
    assert(s64_dos_dir_next(&dos, &entry, &found) == S64_DOS_TRANSPORT_ERROR);
    assert(found == 0u);

    puts("dos_dir host tests passed");
    return 0;
}
