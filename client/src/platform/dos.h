#ifndef SEE64YOU_PLATFORM_DOS_H
#define SEE64YOU_PLATFORM_DOS_H

#include <stdint.h>

#include "uci.h"

#define S64_DOS_TARGET_PRIMARY 0x01u
#define S64_DOS_TARGET_SECONDARY 0x02u
#define S64_DOS_FILENAME_MAX 440u
#define S64_DOS_WRITE_CHUNK 512u

typedef enum {
    S64_DOS_OK = 0,
    S64_DOS_INVALID_ARGUMENT,
    S64_DOS_TRANSPORT_ERROR,
    S64_DOS_CONSUMER_ABORTED,
    S64_DOS_NOT_FOUND,
    S64_DOS_ACCESS_DENIED,
    S64_DOS_NO_FILE_OPEN,
    S64_DOS_TARGET_ERROR
} S64DosResult;

typedef struct {
    S64UciExecuteFn execute;
    uint8_t target;
} S64Dos;

void s64_dos_init(S64Dos* dos, S64UciExecuteFn execute, uint8_t target);

const char* s64_dos_result_message(S64DosResult result);

S64DosResult s64_dos_open_read(S64Dos* dos, const char* filename);
S64DosResult s64_dos_open_write(S64Dos* dos, const char* filename);
S64DosResult s64_dos_close(S64Dos* dos);
S64DosResult s64_dos_seek(S64Dos* dos, uint32_t position);
S64DosResult s64_dos_file_size(S64Dos* dos, uint32_t* size);
/* Reads the current open file position; verifies reported length/address. */
S64DosResult s64_dos_load_reu(S64Dos* dos, uint32_t address, uint32_t length);

S64DosResult s64_dos_read(
    S64Dos* dos,
    uint16_t requested,
    S64UciDataSink sink,
    void* sink_context,
    uint16_t* bytes_read
);

S64DosResult s64_dos_write(
    S64Dos* dos,
    const uint8_t* data,
    uint16_t length
);

S64DosResult s64_dos_file_exists(
    S64Dos* dos,
    const char* filename,
    uint8_t* exists
);
S64DosResult s64_dos_delete_file(S64Dos* dos, const char* filename);
S64DosResult s64_dos_rename_file(
    S64Dos* dos,
    const char* filename,
    const char* new_name
);
S64DosResult s64_dos_change_dir(S64Dos* dos, const char* directory);
S64DosResult s64_dos_create_dir(S64Dos* dos, const char* directory);

#endif
