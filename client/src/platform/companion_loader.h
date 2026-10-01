#ifndef SEE64YOU_PLATFORM_COMPANION_LOADER_H
#define SEE64YOU_PLATFORM_COMPANION_LOADER_H

#include <stdint.h>

#include "dos.h"

#define S64_COMPANION_LOAD_ADDRESS 0x0801u
#define S64_COMPANION_START_ADDRESS 0x080du
#define S64_COMPANION_STACK_BASE 0xcc00u

typedef enum {
    S64_COMPANION_EDITOR = 0,
    S64_COMPANION_PUBLISHER,
    S64_COMPANION_SETTINGS,
    S64_COMPANION_COUNT
} S64CompanionProgram;

typedef enum {
    S64_COMPANION_OK = 0,
    S64_COMPANION_INVALID_ARGUMENT,
    S64_COMPANION_DOS_ERROR,
    S64_COMPANION_INVALID_IMAGE
} S64CompanionResult;

typedef struct {
    uint16_t load_address;
    uint16_t payload_size;
    uint16_t load_end;
    S64DosResult dos_result;
} S64CompanionImage;

const char* s64_companion_filename(S64CompanionProgram program);

/*
 * Select the current proven SD root, open one packaged companion, obtain its
 * exact size, and consume/validate its two-byte PRG load address. On success
 * the file remains open and positioned at the first payload byte so a
 * replacement-safe resident transfer routine can take over without using the
 * caller's overwritten C runtime. On failure any opened file is closed.
 */
S64CompanionResult s64_companion_open_preflight(
    S64Dos* dos,
    S64CompanionProgram program,
    S64CompanionImage* image
);

#endif
