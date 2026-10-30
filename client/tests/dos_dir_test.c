#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "platform/dos_dir.h"

static const char* open_status = "00,OK";
static const char* read_status = "00,OK";
static uint8_t packet_count;
static uint8_t delivered;
static const uint8_t* packets[5];
static uint16_t sizes[5];
static uint8_t fail_transport;
static uint8_t deny_entry;

static void reply_status(S64UciReply* reply, const char* status)
{
    reply->status_length = (uint16_t)strlen(status);
    assert(reply->status_length < reply->status_capacity);
    strcpy(reply->status, status);
}

static S64UciResult fake_open(const S64UciCommandPart* parts,
    uint8_t part_count, S64UciReply* reply)
{
    assert(part_count == 1u && parts[0].length == 2u);
    assert(parts[0].data[0] == S64_DOS_TARGET_PRIMARY);
    assert(parts[0].data[1] == 0x13u);
    reply_status(reply, open_status);
    return S64_UCI_OK;
}

static S64UciResult fake_packets(const S64UciCommandPart* parts,
    uint8_t count, S64UciReply* reply,
    S64UciPacketSink sink, void* context)
{
    uint8_t i;
    assert(count == 1u && parts[0].length == 2u);
    assert(parts[0].data[0] == S64_DOS_TARGET_PRIMARY);
    assert(parts[0].data[1] == 0x14u);
    if (fail_transport) return S64_UCI_TIMEOUT;
    for (i = 0u; i < packet_count; ++i) {
        /* All entries arrive in ONE READ_DIR command. Chunk boundaries
         * are independent of packet boundaries. */
        if (sizes[i] > 4u) {
            if (!sink(packets[i], 4u, 0u, context)) return S64_UCI_CONSUMER_ABORTED;
            if (!sink(packets[i] + 4u, (uint16_t)(sizes[i] - 4u),
                      1u, context)) return S64_UCI_CONSUMER_ABORTED;
        } else if (!sink(packets[i], sizes[i], 1u, context)) {
            return S64_UCI_CONSUMER_ABORTED;
        }
    }
    reply_status(reply, read_status);
    return S64_UCI_OK;
}

static uint8_t collect(const S64DosDirEntry* entry, void* context)
{
    uint8_t* counter = (uint8_t*)context;
    ++delivered;
    ++*counter;
    if (delivered == 1u) {
        assert(entry->attributes == 0x20u);
        assert(!entry->skipped);
        assert(!strcmp(entry->name, "INDEX.8TML"));
    } else if (delivered == 2u) {
        assert(entry->attributes == 0x10u);
        assert(!strcmp(entry->name, "pages"));
    }
    return !deny_entry;
}

int main(void)
{
    S64Dos dos;
    uint8_t empty, count;
    uint8_t long_packet[40], i;
    static const uint8_t file[] = {
        0x20u, 'I','N','D','E','X','.','8','T','M','L',0u
    };
    static const uint8_t folder[] = {
        0x10u, 'p','a','g','e','s',0u
    };
    static const uint8_t bad[] = {0x20u,'a','/', 'b',0u};
    static const uint8_t malformed[] = {0x20u};
    static const uint8_t bad_term[] = {0x20u,'a',0u,'b'};
    s64_dos_init(&dos, fake_open, S64_DOS_TARGET_PRIMARY);
    open_status = "01,DIRECTORY EMPTY";
    assert(s64_dos_dir_open(&dos, &empty) == S64_DOS_OK && empty);
    open_status = "86,CAN'T READ DIRECTORY";
    assert(s64_dos_dir_open(&dos, &empty) == S64_DOS_TARGET_ERROR);
    open_status = "00,OK";
    assert(s64_dos_dir_open(&dos, &empty) == S64_DOS_OK && !empty);

    packets[0] = file; sizes[0] = sizeof(file);
    packets[1] = folder; sizes[1] = sizeof(folder);
    packet_count = 2u; delivered = count = 0u;
    assert(s64_dos_dir_read(&dos, fake_packets, collect, &count) == S64_DOS_OK);
    assert(count == 2u && delivered == 2u);

    packet_count = 0u; count = 0u;
    assert(s64_dos_dir_read(&dos, fake_packets, collect, &count) == S64_DOS_OK);
    assert(count == 0u);

    packets[0] = bad; sizes[0] = sizeof(bad);
    packet_count = 1u; delivered = count = 0u;
    assert(s64_dos_dir_read(&dos, fake_packets, collect, &count)
           == S64_DOS_OK);
    assert(count == 0u);

    long_packet[0] = 0x20u;
    for (i = 1u; i < sizeof(long_packet) - 1u; ++i) long_packet[i] = 'x';
    long_packet[sizeof(long_packet)-1u] = 0u;
    packets[0] = long_packet; sizes[0] = sizeof(long_packet);
    assert(s64_dos_dir_read(&dos, fake_packets, collect, &count)
           == S64_DOS_OK);
    assert(count == 0u);

    packets[0] = malformed; sizes[0] = sizeof(malformed);
    assert(s64_dos_dir_read(&dos, fake_packets, collect, &count)
           == S64_DOS_CONSUMER_ABORTED);
    packets[0] = bad_term; sizes[0] = sizeof(bad_term);
    assert(s64_dos_dir_read(&dos, fake_packets, collect, &count)
           == S64_DOS_CONSUMER_ABORTED);

    packets[0] = file; sizes[0] = sizeof(file);
    delivered = count = 0u; deny_entry = 1u;
    assert(s64_dos_dir_read(&dos, fake_packets, collect, &count)
           == S64_DOS_CONSUMER_ABORTED);
    deny_entry = 0u;
    delivered = count = 0u;
    read_status = "85,NO FILE OPEN";
    assert(s64_dos_dir_read(&dos, fake_packets, collect, &count)
           == S64_DOS_TARGET_ERROR);
    read_status = "00,OK";
    fail_transport = 1u;
    assert(s64_dos_dir_read(&dos, fake_packets, collect, &count)
           == S64_DOS_TRANSPORT_ERROR);
    puts("dos_dir packet-stream host tests passed");
    return 0;
}
