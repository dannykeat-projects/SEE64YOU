#include "buffer.h"
#include "platform/reu.h"

#ifndef S64_REU_BUFFER
#error "buffer_reu.c requires S64_REU_BUFFER"
#endif

#define S64_REU_BUFFER_BANK 0u
#define S64_REU_WINDOW_SIZE 64u

static unsigned char reu_window[S64_REU_WINDOW_SIZE];
static unsigned char reu_insert_value;
static const S64Buffer* reu_window_owner;
static unsigned int reu_window_start;
static unsigned char reu_window_length;

static void invalidate_window(const S64Buffer* buffer)
{
    if (reu_window_owner == buffer) {
        reu_window_owner = 0;
        reu_window_length = 0u;
    }
}

static unsigned int physical_position(
    const S64Buffer* buffer,
    unsigned int logical
)
{
    if (logical < buffer->gap_start) {
        return logical;
    }

    return (unsigned int)(
        logical + (buffer->gap_end - buffer->gap_start)
    );
}

static unsigned char load_window(
    const S64Buffer* buffer,
    unsigned int physical
)
{
    unsigned int count;

    if (physical >= S64_BUFFER_CAPACITY) {
        return 0u;
    }

    count = S64_BUFFER_CAPACITY - physical;
    if (count > S64_REU_WINDOW_SIZE) {
        count = S64_REU_WINDOW_SIZE;
    }

    if (!s64_reu_load(
            S64_REU_BUFFER_BANK,
            (uint16_t)physical,
            reu_window,
            (uint16_t)count
        )) {
        reu_window_owner = 0;
        reu_window_length = 0u;
        return 0u;
    }

    reu_window_owner = buffer;
    reu_window_start = physical;
    reu_window_length = (unsigned char)count;
    return 1u;
}

void s64_buffer_init(S64Buffer* buffer)
{
    buffer->gap_start = 0u;
    buffer->gap_end = S64_BUFFER_CAPACITY;
    invalidate_window(buffer);
}

unsigned int s64_buffer_length(const S64Buffer* buffer)
{
    return (unsigned int)(
        S64_BUFFER_CAPACITY - (buffer->gap_end - buffer->gap_start)
    );
}

unsigned int s64_buffer_cursor(const S64Buffer* buffer)
{
    return buffer->gap_start;
}

unsigned int s64_buffer_free(const S64Buffer* buffer)
{
    return (unsigned int)(buffer->gap_end - buffer->gap_start);
}

unsigned char s64_buffer_char_at(
    const S64Buffer* buffer,
    unsigned int index
)
{
    unsigned int physical;

    if (index >= s64_buffer_length(buffer)) {
        return 0u;
    }

    physical = physical_position(buffer, index);
    if (reu_window_owner != buffer
        || physical < reu_window_start
        || physical >= reu_window_start + reu_window_length) {
        if (!load_window(buffer, physical)) {
            return 0u;
        }
    }

    return reu_window[physical - reu_window_start];
}

unsigned int s64_buffer_read(
    const S64Buffer* buffer,
    unsigned int start,
    unsigned char* output,
    unsigned int capacity
)
{
    unsigned int length = s64_buffer_length(buffer);
    unsigned int count;
    unsigned int first;
    unsigned int gap_size = buffer->gap_end - buffer->gap_start;

    if (output == 0 || capacity == 0u || start >= length) {
        return 0u;
    }

    count = length - start;
    if (count > capacity) {
        count = capacity;
    }

    if (start < buffer->gap_start) {
        first = buffer->gap_start - start;
        if (first > count) {
            first = count;
        }

        if (!s64_reu_load(
                S64_REU_BUFFER_BANK,
                (uint16_t)start,
                output,
                (uint16_t)first
            )) {
            return 0u;
        }

        if (first < count
            && !s64_reu_load(
                S64_REU_BUFFER_BANK,
                (uint16_t)buffer->gap_end,
                output + first,
                (uint16_t)(count - first)
            )) {
            return 0u;
        }
    } else if (!s64_reu_load(
            S64_REU_BUFFER_BANK,
            (uint16_t)(start + gap_size),
            output,
            (uint16_t)count
        )) {
        return 0u;
    }

    return count;
}

unsigned int s64_buffer_find_byte(
    const S64Buffer* buffer,
    unsigned int start,
    unsigned char value
)
{
    unsigned int length = s64_buffer_length(buffer);

    while (start < length && s64_buffer_char_at(buffer, start) != value) {
        ++start;
    }

    return start;
}

int s64_buffer_move_cursor(S64Buffer* buffer, unsigned int position)
{
    unsigned int length = s64_buffer_length(buffer);
    unsigned int distance;
    unsigned int chunk;
    unsigned int source;
    unsigned int destination;

    if (position > length) {
        return 0;
    }

    invalidate_window(buffer);

    while (buffer->gap_start > position) {
        distance = buffer->gap_start - position;
        chunk = distance > S64_REU_WINDOW_SIZE
            ? S64_REU_WINDOW_SIZE
            : distance;
        source = buffer->gap_start - chunk;
        destination = buffer->gap_end - chunk;

        if (!s64_reu_load(
                S64_REU_BUFFER_BANK,
                (uint16_t)source,
                reu_window,
                (uint16_t)chunk
            )
            || !s64_reu_store(
                S64_REU_BUFFER_BANK,
                (uint16_t)destination,
                reu_window,
                (uint16_t)chunk
            )) {
            return 0;
        }

        buffer->gap_start -= chunk;
        buffer->gap_end -= chunk;
    }

    while (buffer->gap_start < position) {
        distance = position - buffer->gap_start;
        chunk = distance > S64_REU_WINDOW_SIZE
            ? S64_REU_WINDOW_SIZE
            : distance;
        source = buffer->gap_end;
        destination = buffer->gap_start;

        if (!s64_reu_load(
                S64_REU_BUFFER_BANK,
                (uint16_t)source,
                reu_window,
                (uint16_t)chunk
            )
            || !s64_reu_store(
                S64_REU_BUFFER_BANK,
                (uint16_t)destination,
                reu_window,
                (uint16_t)chunk
            )) {
            return 0;
        }

        buffer->gap_start += chunk;
        buffer->gap_end += chunk;
    }

    return 1;
}

int s64_buffer_insert(S64Buffer* buffer, unsigned char value)
{
    if (buffer->gap_start == buffer->gap_end) {
        return 0;
    }

    invalidate_window(buffer);
    reu_insert_value = value;
    if (!s64_reu_store(
            S64_REU_BUFFER_BANK,
            (uint16_t)buffer->gap_start,
            &reu_insert_value,
            1u
        )) {
        return 0;
    }

    ++buffer->gap_start;
    return 1;
}

int s64_buffer_backspace(S64Buffer* buffer)
{
    if (buffer->gap_start == 0u) {
        return 0;
    }

    invalidate_window(buffer);
    --buffer->gap_start;
    return 1;
}

int s64_buffer_delete(S64Buffer* buffer)
{
    if (buffer->gap_end == S64_BUFFER_CAPACITY) {
        return 0;
    }

    invalidate_window(buffer);
    ++buffer->gap_end;
    return 1;
}

int s64_buffer_delete_range(
    S64Buffer* buffer,
    unsigned int start,
    unsigned int end
)
{
    unsigned int length = s64_buffer_length(buffer);

    if (start > end || end > length) {
        return 0;
    }

    if (start == end) {
        return 1;
    }

    if (!s64_buffer_move_cursor(buffer, start)) {
        return 0;
    }

    invalidate_window(buffer);
    buffer->gap_end = (unsigned int)(buffer->gap_end + (end - start));
    return 1;
}
