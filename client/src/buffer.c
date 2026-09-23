#include "buffer.h"

void s64_buffer_init(S64Buffer* buffer)
{
    buffer->gap_start = 0u;
    buffer->gap_end = S64_BUFFER_CAPACITY;
}

unsigned int s64_buffer_length(const S64Buffer* buffer)
{
    return (unsigned int)(S64_BUFFER_CAPACITY - (buffer->gap_end - buffer->gap_start));
}

unsigned int s64_buffer_cursor(const S64Buffer* buffer)
{
    return buffer->gap_start;
}

unsigned int s64_buffer_free(const S64Buffer* buffer)
{
    return (unsigned int)(buffer->gap_end - buffer->gap_start);
}

unsigned char s64_buffer_char_at(const S64Buffer* buffer, unsigned int index)
{
    if (index < buffer->gap_start) {
        return buffer->data[index];
    }

    return buffer->data[index + (buffer->gap_end - buffer->gap_start)];
}

int s64_buffer_move_cursor(S64Buffer* buffer, unsigned int position)
{
    unsigned int length = s64_buffer_length(buffer);

    if (position > length) {
        return 0;
    }

    while (buffer->gap_start > position) {
        --buffer->gap_start;
        --buffer->gap_end;
        buffer->data[buffer->gap_end] = buffer->data[buffer->gap_start];
    }

    while (buffer->gap_start < position) {
        buffer->data[buffer->gap_start] = buffer->data[buffer->gap_end];
        ++buffer->gap_start;
        ++buffer->gap_end;
    }

    return 1;
}

int s64_buffer_insert(S64Buffer* buffer, unsigned char value)
{
    if (buffer->gap_start == buffer->gap_end) {
        return 0;
    }

    buffer->data[buffer->gap_start++] = value;
    return 1;
}

int s64_buffer_backspace(S64Buffer* buffer)
{
    if (buffer->gap_start == 0u) {
        return 0;
    }

    --buffer->gap_start;
    return 1;
}

int s64_buffer_delete(S64Buffer* buffer)
{
    if (buffer->gap_end == S64_BUFFER_CAPACITY) {
        return 0;
    }

    ++buffer->gap_end;
    return 1;
}
