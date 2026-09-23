#include <stdio.h>
#include <stdlib.h>
#include "buffer.h"

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

int main(void)
{
    S64Buffer buffer;

    s64_buffer_init(&buffer);
    require(s64_buffer_length(&buffer) == 0u, "new buffer is empty");
    require(s64_buffer_free(&buffer) == S64_BUFFER_CAPACITY, "new buffer is fully free");

    require(s64_buffer_insert(&buffer, 'A'), "insert A");
    require(s64_buffer_insert(&buffer, 'B'), "insert B");
    require(s64_buffer_insert(&buffer, 'C'), "insert C");
    require(s64_buffer_length(&buffer) == 3u, "length after insert");
    require(s64_buffer_char_at(&buffer, 1u) == 'B', "char lookup");

    require(s64_buffer_move_cursor(&buffer, 1u), "move cursor into text");
    require(s64_buffer_insert(&buffer, 'X'), "insert in middle");
    require(s64_buffer_char_at(&buffer, 0u) == 'A', "middle insert A");
    require(s64_buffer_char_at(&buffer, 1u) == 'X', "middle insert X");
    require(s64_buffer_char_at(&buffer, 2u) == 'B', "middle insert B");
    require(s64_buffer_char_at(&buffer, 3u) == 'C', "middle insert C");

    require(s64_buffer_backspace(&buffer), "backspace middle insert");
    require(s64_buffer_length(&buffer) == 3u, "length after backspace");
    require(s64_buffer_char_at(&buffer, 1u) == 'B', "backspace restored text");

    require(s64_buffer_delete(&buffer), "forward delete");
    require(s64_buffer_length(&buffer) == 2u, "length after forward delete");
    require(s64_buffer_char_at(&buffer, 0u) == 'A', "delete keeps A");
    require(s64_buffer_char_at(&buffer, 1u) == 'C', "delete removes B");

    require(s64_buffer_move_cursor(&buffer, 2u), "move to end for range fixture");
    require(s64_buffer_insert(&buffer, 'D'), "append D");
    require(s64_buffer_insert(&buffer, 'E'), "append E");
    require(s64_buffer_delete_range(&buffer, 1u, 3u), "delete range");
    require(s64_buffer_length(&buffer) == 2u, "range deletion length");
    require(s64_buffer_char_at(&buffer, 0u) == 'A', "range deletion keeps prefix");
    require(s64_buffer_char_at(&buffer, 1u) == 'E', "range deletion keeps suffix");
    require(s64_buffer_cursor(&buffer) == 1u, "range deletion cursor at start");
    require(!s64_buffer_delete_range(&buffer, 2u, 9u), "reject invalid range");
    require(!s64_buffer_move_cursor(&buffer, 99u), "reject cursor past end");

    puts("buffer tests passed");
    return 0;
}
