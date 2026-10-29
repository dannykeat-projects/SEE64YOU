#ifndef SEE64YOU_PLATFORM_DOS_DIR_H
#define SEE64YOU_PLATFORM_DOS_DIR_H

#include <stdint.h>
#include "platform/dos.h"

/* A source the Publisher can actually open has at most 33 native characters.
   Longer/unsupported directory names are skipped, never silently truncated. */
#define S64_DOS_DIR_NAME_CAPACITY 34u
#define S64_DOS_DIR_ATTRIBUTE_DIRECTORY 0x10u

typedef struct {
    char name[S64_DOS_DIR_NAME_CAPACITY];
    uint8_t attributes;
    uint8_t skipped;
} S64DosDirEntry;

/* Calls OPEN_DIR on the DOS target's current directory.
   An empty directory is success with *empty set. */
S64DosResult s64_dos_dir_open(S64Dos* dos, uint8_t* empty);

/* Each READ_DIR command returns at most one entry.
   *found==0 means end of directory; skipped entries do not fit the local
   filename/ASCII contract and must not be opened by a truncated name.
   This module does not change directories or touch the DOS file handle. */
S64DosResult s64_dos_dir_next(S64Dos* dos,
    S64DosDirEntry* entry, uint8_t* found);

#endif
