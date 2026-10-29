#ifndef SEE64YOU_BUFFER_H
#define SEE64YOU_BUFFER_H

#include <stddef.h>

#ifdef S64_REU_BUFFER
#include "platform/reu_layout.h"
#define S64_BUFFER_CAPACITY S64_REU_DOCUMENT_CAPACITY

typedef struct {
    unsigned int gap_start;
    unsigned int gap_end;
    unsigned int cursor;
} S64Buffer;
#else
#define S64_BUFFER_CAPACITY 4096u

typedef struct {
    unsigned char data[S64_BUFFER_CAPACITY];
    unsigned int gap_start;
    unsigned int gap_end;
} S64Buffer;
#endif

void s64_buffer_init(S64Buffer* buffer);
unsigned int s64_buffer_length(const S64Buffer* buffer);
unsigned int s64_buffer_cursor(const S64Buffer* buffer);
unsigned int s64_buffer_free(const S64Buffer* buffer);
unsigned char s64_buffer_char_at(const S64Buffer* buffer, unsigned int index);
unsigned int s64_buffer_read(const S64Buffer* buffer, unsigned int start, unsigned char* output, unsigned int capacity);
unsigned int s64_buffer_find_byte(const S64Buffer* buffer, unsigned int start, unsigned char value);
int s64_buffer_move_cursor(S64Buffer* buffer, unsigned int position);
int s64_buffer_insert(S64Buffer* buffer, unsigned char value);
/* Stream loading only: append at an EOF cursor/gap, without moving existing text. */
int s64_buffer_append(S64Buffer* buffer, const unsigned char* data, unsigned int length);
int s64_buffer_backspace(S64Buffer* buffer);
int s64_buffer_delete(S64Buffer* buffer);
int s64_buffer_delete_range(S64Buffer* buffer, unsigned int start, unsigned int end);

#endif
