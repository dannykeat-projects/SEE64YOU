#ifndef SEE64YOU_PUBLISHER_PLAINTEXT_H
#define SEE64YOU_PUBLISHER_PLAINTEXT_H

#include <stdint.h>

#include "buffer.h"
#include "platform/dos.h"

#define S64_PLAINTEXT_FILENAME_CAPACITY 34u

typedef enum {
    S64_PLAINTEXT_OK = 0,
    S64_PLAINTEXT_INVALID_ARGUMENT,
    S64_PLAINTEXT_EXISTS,
    S64_PLAINTEXT_INVALID_TEXT,
    S64_PLAINTEXT_TOO_LARGE,
    S64_PLAINTEXT_IO_ERROR
} S64PlainTextResult;

const char* s64_plaintext_result_message(S64PlainTextResult result);

S64PlainTextResult s64_plaintext_export(
    S64Dos* dos,
    const S64Buffer* buffer,
    const char* filename,
    uint8_t overwrite
);

S64PlainTextResult s64_plaintext_import(
    S64Dos* dos,
    S64Buffer* buffer,
    const char* filename
);

#endif
