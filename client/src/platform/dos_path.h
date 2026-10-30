#ifndef SEE64YOU_PLATFORM_DOS_PATH_H
#define SEE64YOU_PLATFORM_DOS_PATH_H

#include "platform/dos.h"

/* Read-only GET_PATH. Output is bounded native text, cleared on failure. */
S64DosResult s64_dos_get_path(S64Dos* dos, char* output, uint16_t capacity);

#endif
