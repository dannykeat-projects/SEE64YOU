#ifndef SEE64YOU_BUFFER_H
#define SEE64YOU_BUFFER_H

#include <stddef.h>

#define S64_BUFFER_CAPACITY 8192u

typedef struct {
    unsigned char data[S64_BUFFER_CAPACITY];
    unsigned int gap_start;
    unsigned int gap_end;
} S64Buffer;

void s64_buffer_init(S64Buffer* buffer);
unsigned int s64_buffer_length(const S64Buffer* buffer);
unsigned int s64_buffer_cursor(const S64Buffer* buffer);
unsigned int s64_buffer_free(const S64Buffer* buffer);
unsigned char s64_buffer_char_at(const S64Buffer* buffer, unsigned int index);
int s64_buffer_move_cursor(S64Buffer* buffer, unsigned int position);
int s64_buffer_insert(S64Buffer* buffer, unsigned char value);
int s64_buffer_backspace(S64Buffer* buffer);
int s64_buffer_delete(S64Buffer* buffer);

#endif
