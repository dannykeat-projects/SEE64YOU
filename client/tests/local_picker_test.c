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
static unsigned char dir_cursor;
static unsigned char key_pos;
static const char* keys;
static unsigned char unreadable;

static S64UciResult fake_uci(const S64UciCommandPart* parts,
    uint8_t count, S64UciReply* reply)
{
    uint8_t command, attribute;
    const char* name;
    unsigned char bytes[36];
    uint16_t length;

    assert(count == 1u && parts[0].length == 2u);
    command = parts[0].data[1];
    assert(parts[0].data[0] == S64_DOS_TARGET_PRIMARY);
    if (command == 0x13u) {
        dir_cursor = 0u;
        strcpy(reply->status, unreadable ? "86,CAN'T READ DIRECTORY" : "00,OK");
    } else {
        assert(command == 0x14u);
        strcpy(reply->status, "00,OK");
        if (dir_cursor < (sizeof(files)/sizeof(files[0]))) {
            name = files[dir_cursor];
            attribute = dir_cursor == 0u ? 0x10u
                : dir_cursor == 1u ? 0x02u : 0x20u;
            ++dir_cursor;
            length = (uint16_t)strlen(name);
            bytes[0] = attribute;
            memcpy(bytes + 1u, name, length + 1u);
            assert(reply->data_sink(bytes, (uint16_t)(length + 2u),
                reply->data_context));
        }
    }
    reply->status_length = (uint16_t)strlen(reply->status);
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
void cputs(const char* s) {(void)s;}
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
    s64_dos_init(&dos, fake_uci, S64_DOS_TARGET_PRIMARY);

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

    puts("local_picker host tests passed");
    return 0;
}
