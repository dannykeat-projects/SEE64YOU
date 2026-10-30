#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "publisher/local_picker.h"
#include "platform/dos_dir.h"

static const char* files[] = {
    "folder", "HIDDEN.8TML", "FILE01.8TML", "FILE02.8TML",
    "README.TXT", "FILE03.8TML", "FILE04.8TML",
    "FILE05.8TML", "FILE06.8TML", "FILE07.8TML",
    "FILE08.8TML", "FILE09.8TML"
};
static unsigned char key_pos;
static const char* keys;
static unsigned char unreadable;
static unsigned char working_displayed;

static S64UciResult fake_open(const S64UciCommandPart* parts,
    uint8_t count, S64UciReply* reply)
{
    const char* status;
    assert(count == 1u && parts[0].length == 2u);
    assert(parts[0].data[0] == S64_DOS_TARGET_PRIMARY);
    assert(parts[0].data[1] == 0x13u);
    /* The user must see pending feedback BEFORE the synchronous DOS call. */
    assert(working_displayed);
    working_displayed = 0u;
    status = unreadable ? "86,CAN'T READ DIRECTORY" : "00,OK";
    strcpy(reply->status, status);
    reply->status_length = (uint16_t)strlen(status);
    return S64_UCI_OK;
}

S64UciResult s64_uci_execute_packets(const S64UciCommandPart* parts,
    uint8_t count, S64UciReply* reply,
    S64UciPacketSink sink, void* context)
{
    uint8_t i, attribute;
    uint16_t length;
    unsigned char bytes[36];
    const char* name;
    assert(count == 1u && parts[0].length == 2u);
    assert(parts[0].data[0] == S64_DOS_TARGET_PRIMARY);
    assert(parts[0].data[1] == 0x14u);

    /* One READ_DIR command streams all entries, each in its own packet. */
    for (i = 0u; i < sizeof(files)/sizeof(files[0]); ++i) {
        name = files[i];
        attribute = i == 0u ? 0x10u : i == 1u ? 0x02u : 0x20u;
        length = (uint16_t)strlen(name);
        bytes[0] = attribute;
        memcpy(bytes + 1u, name, length + 1u);
        if (!sink(bytes, (uint16_t)(length + 2u), 1u, context))
            return S64_UCI_CONSUMER_ABORTED;
    }
    strcpy(reply->status, "00,OK");
    reply->status_length = 5u;
    return S64_UCI_OK;
}

unsigned char cgetc(void)
{
    unsigned char key = (unsigned char)keys[key_pos];
    assert(key != 0u);
    ++key_pos;
    return key;
}
void clrscr(void) {}
void cputs(const char* s) {
    if (strstr(s, "READING LOCAL FILES...") && strstr(s, "PLEASE WAIT..."))
        working_displayed = 1u;
}
void cputc(char c) {(void)c;}
void gotoxy(unsigned char x,unsigned char y) {(void)x;(void)y;}
void revers(unsigned char r) {(void)r;}

static void set_keys(const char* sequence)
{
    keys = sequence;
    key_pos = 0u;
}

int main(void)
{
    S64Dos dos;
    char filename[65];
    s64_dos_init(&dos, fake_open, S64_DOS_TARGET_PRIMARY);

    set_keys("91");
    assert(s64_local_pick(&dos, filename, sizeof(filename)) == 1u);
    assert(!strcmp(filename, "FILE07.8TML"));
    set_keys("92");
    assert(s64_local_pick(&dos, filename, sizeof(filename)) == 1u);
    assert(!strcmp(filename, "FILE08.8TML"));
    set_keys("91");
    assert(s64_local_pick(&dos, filename, sizeof(filename)) == 1u);
    assert(!strcmp(filename, "FILE07.8TML"));

    strcpy(filename, "UNCHANGED");
    set_keys("0");
    assert(s64_local_pick(&dos, filename, sizeof(filename)) == 0u);
    assert(!strcmp(filename, "UNCHANGED"));
    set_keys("7");
    assert(s64_local_pick(&dos, filename, sizeof(filename)) == 2u);
    assert(!strcmp(filename, "UNCHANGED"));
    unreadable = 1u;
    assert(s64_local_pick(&dos, filename, sizeof(filename)) == 3u);
    assert(!strcmp(filename, "UNCHANGED"));

    puts("local_picker packet-stream host tests passed");
    return 0;
}
