#ifndef SEE64YOU_STORAGE_LOCAL_H
#define SEE64YOU_STORAGE_LOCAL_H

#include <stdint.h>

#include "document.h"
#include "platform/dos.h"

#define S64_LOCAL_FILENAME_CAPACITY 34u
#define S64_LOCAL_RECOVERY_FILENAME "SEE64YOU.REC"

typedef enum {
    S64_LOCAL_OK = 0,
    S64_LOCAL_INVALID_ARGUMENT,
    S64_LOCAL_DIRTY,
    S64_LOCAL_NO_FILENAME,
    S64_LOCAL_EXISTS,
    S64_LOCAL_INVALID_DOCUMENT,
    S64_LOCAL_DOCUMENT_TOO_LARGE,
    S64_LOCAL_IO_ERROR
} S64LocalResult;

typedef struct {
    S64Dos* dos;
    S64Document* document;
    char filename[S64_LOCAL_FILENAME_CAPACITY];
    S64DosResult dos_result;
    uint8_t document_replaced;
} S64LocalDocument;

void s64_local_init(
    S64LocalDocument* local,
    S64Dos* dos,
    S64Document* document
);

const char* s64_local_result_message(const S64LocalDocument* local, S64LocalResult result);
uint8_t s64_local_has_filename(const S64LocalDocument* local);
uint8_t s64_local_document_replaced(const S64LocalDocument* local);

S64LocalResult s64_local_new(S64LocalDocument* local, uint8_t discard_dirty);
S64LocalResult s64_local_open(
    S64LocalDocument* local,
    const char* filename,
    uint8_t discard_dirty
);
S64LocalResult s64_local_save(S64LocalDocument* local);
S64LocalResult s64_local_save_as(
    S64LocalDocument* local,
    const char* filename,
    uint8_t overwrite
);
S64LocalResult s64_local_recovery_exists(
    S64LocalDocument* local,
    uint8_t* exists
);
S64LocalResult s64_local_save_recovery(S64LocalDocument* local);
S64LocalResult s64_local_open_recovery(S64LocalDocument* local);
S64LocalResult s64_local_delete_recovery(S64LocalDocument* local);

#endif
