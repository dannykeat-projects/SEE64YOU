#include <stdint.h>

#include "buffer.h"
#include "platform/reu.h"

#define DIAG_PROGRESS (*(volatile uint8_t*)0xC010u)
#define DIAG_RESULT   (*(volatile uint8_t*)0xC011u)
#define DIAG_DETAIL   (*(volatile uint8_t*)0xC012u)

#define DIAG_PASS 0xA5u
#define DIAG_FAIL 0x5Au

static S64Buffer buffer;
static unsigned char readback[64];

static unsigned char original_value(unsigned int index)
{
    return (unsigned char)(((index * 7u) + 3u) & 0x7fu);
}

static unsigned char final_value(unsigned int position)
{
    if (position < 1000u) {
        return original_value(position);
    }
    if (position == 1000u) {
        return 0xe1u;
    }
    if (position < 2000u) {
        return original_value(position - 1u);
    }
    if (position < 4000u) {
        return original_value(position + 999u);
    }
    if (position == 4000u) {
        return 0xe2u;
    }
    return original_value(position + 998u);
}

static void fail(unsigned char detail)
{
    DIAG_DETAIL = detail;
    DIAG_RESULT = DIAG_FAIL;
    for (;;) {
    }
}

int main(void)
{
    unsigned int i;

    DIAG_PROGRESS = 0x10u;
    DIAG_RESULT = 0u;
    DIAG_DETAIL = 0u;

    if (!s64_reu_probe()) {
        fail(1u);
    }

    s64_buffer_init(&buffer);

    DIAG_PROGRESS = 0x20u;
    for (i = 0u; i < 6000u; ++i) {
        if (!s64_buffer_insert(&buffer, original_value(i))) {
            fail(2u);
        }
    }

    if (s64_buffer_length(&buffer) != 6000u
        || s64_buffer_char_at(&buffer, 0u) != original_value(0u)
        || s64_buffer_char_at(&buffer, 4095u) != original_value(4095u)
        || s64_buffer_char_at(&buffer, 4096u) != original_value(4096u)
        || s64_buffer_char_at(&buffer, 5999u) != original_value(5999u)) {
        fail(3u);
    }

    DIAG_PROGRESS = 0x30u;
    if (!s64_buffer_move_cursor(&buffer, 1000u)
        || !s64_buffer_insert(&buffer, 0xe1u)
        || !s64_buffer_move_cursor(&buffer, 5000u)
        || !s64_buffer_insert(&buffer, 0xe2u)) {
        fail(4u);
    }

    DIAG_PROGRESS = 0x40u;
    if (!s64_buffer_delete_range(&buffer, 2000u, 3000u)
        || s64_buffer_length(&buffer) != 5002u) {
        fail(5u);
    }

    if (s64_buffer_char_at(&buffer, 999u) != final_value(999u)
        || s64_buffer_char_at(&buffer, 1000u) != final_value(1000u)
        || s64_buffer_char_at(&buffer, 1001u) != final_value(1001u)
        || s64_buffer_char_at(&buffer, 1999u) != final_value(1999u)
        || s64_buffer_char_at(&buffer, 2000u) != final_value(2000u)
        || s64_buffer_char_at(&buffer, 3999u) != final_value(3999u)
        || s64_buffer_char_at(&buffer, 4000u) != final_value(4000u)
        || s64_buffer_char_at(&buffer, 4001u) != final_value(4001u)
        || s64_buffer_char_at(&buffer, 5001u) != final_value(5001u)) {
        fail(6u);
    }

    DIAG_PROGRESS = 0x50u;
    if (!s64_buffer_move_cursor(&buffer, 3995u)
        || s64_buffer_read(&buffer, 3970u, readback, sizeof(readback)) != sizeof(readback)) {
        fail(7u);
    }
    for (i = 0u; i < sizeof(readback); ++i) {
        if (readback[i] != final_value((unsigned int)(3970u + i))) {
            fail(8u);
        }
    }

    DIAG_PROGRESS = 0x60u;
    if (!s64_buffer_move_cursor(&buffer, 0u)
        || s64_buffer_char_at(&buffer, 4000u) != final_value(4000u)
        || !s64_buffer_move_cursor(&buffer, s64_buffer_length(&buffer))
        || s64_buffer_char_at(&buffer, 5001u) != final_value(5001u)) {
        fail(9u);
    }

    DIAG_PROGRESS = 0x70u;
    DIAG_RESULT = DIAG_PASS;
    for (;;) {
    }
}
