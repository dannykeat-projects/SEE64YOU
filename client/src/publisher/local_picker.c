#include <conio.h>
#include <string.h>
#include "publisher/local_picker.h"
#include "platform/dos_dir.h"
#include "charset.h"

#define PICKER_ROWS 6u
#define PICKER_SCAN_LIMIT 1024u

/* Static to avoid the small C64 software stack. One page only. */
static char names[PICKER_ROWS][S64_DOS_DIR_NAME_CAPACITY];
static uint8_t count, more, page;

static uint8_t is_source(const char* name)
{
    uint8_t c, i;
    uint8_t n = (uint8_t)strlen(name);
    static const uint8_t suffix[] = {0x2Eu, 0x38u, 0x74u, 0x6Du, 0x6Cu};
    if (n < 5u) return 0u;
    for (i = 0u; i < 5u; ++i) {
        if (!s64_charset_native_to_ascii(
                (uint8_t)name[(uint8_t)(n - 5u + i)], &c)) return 0u;
        if (c >= 0x41u && c <= 0x5Au) c = (uint8_t)(c + 32u);
        if (c != (uint8_t)suffix[i]) return 0u;
    }
    return 1u;
}

typedef struct {
    uint16_t scanned;
    uint16_t eligible;
} PickerScan;

static uint8_t on_directory_entry(const S64DosDirEntry* entry, void* context)
{
    PickerScan* scan = (PickerScan*)context;

    if (scan->scanned == PICKER_SCAN_LIMIT) return 0u;
    ++scan->scanned;
    if ((entry->attributes & 0x1Eu) != 0u
        || !is_source(entry->name)) return 1u;
    if (scan->eligible < (uint16_t)page * PICKER_ROWS) {
        ++scan->eligible;
        return 1u;
    }
    ++scan->eligible;
    if (count < PICKER_ROWS) {
        strcpy(names[count++], entry->name);
    } else {
        more = 1u;
    }
    return 1u;
}

static uint8_t read_page(S64Dos* dos)
{
    uint8_t empty;
    PickerScan scan;
    S64DosResult result;

    count = more = 0u;
    scan.scanned = scan.eligible = 0u;
    result = s64_dos_dir_open(dos, &empty);
    if (result != S64_DOS_OK) return 0u;
    if (empty) return 1u;

    result = s64_dos_dir_read(dos, s64_uci_execute_packets,
                              on_directory_entry, &scan);
    return result == S64_DOS_OK;
}

static void show_page(void)
{
    uint8_t i, row;
    clrscr();
    cputs("SEE64YOU - LOCAL .8TML FILES\r\n");
    cputs("---------------------------------------\r\n");
    for (i = 0u; i < count; ++i) {
        row = (uint8_t)(3u + i * 2u);
        gotoxy(1u, row);
        revers(1u);
        cputc('[');
        cputc((char)('1' + i));
        cputs("] ");
        cputs(names[i]);
        revers(0u);
    }
    if (!count) {
        gotoxy(1u, 5u);
        cputs("No .8tml files on this page");
    }
    gotoxy(0u, 17u);
    cputs("[7] TYPE NAME  [8] PREV  [9] NEXT\r\n");
    cputs("[0] BACK\r\n");
    if (page) cputs("Previous page available\r\n");
    if (more) cputs("Next page available\r\n");
    cputs("SELECT NUMBER OR RUN/STOP TO BACK");
}

uint8_t s64_local_pick(S64Dos* dos, char* filename, uint8_t capacity)
{
    unsigned char key;
    page = 0u;
    if (!dos || !filename || capacity < S64_DOS_DIR_NAME_CAPACITY)
        return 3u;

    for (;;) {
        if (!read_page(dos)) return 3u;
        show_page();
        key = cgetc();
        if (key == CH_STOP || key == '0') return 0u;
        if (key == '7') return 2u;
        if (key == '8' && page) {
            --page;
        } else if (key == '9' && more) {
            if (page >= (PICKER_SCAN_LIMIT / PICKER_ROWS)) return 3u;
            ++page;
        } else if (key >= '1' && key < (unsigned char)('1' + count)) {
            strcpy(filename, names[(uint8_t)(key - '1')]);
            return 1u;
        }
    }
}
