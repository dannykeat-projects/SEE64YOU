#include "clipboard.h"

#ifdef S64_REU_BUFFER
#include "platform/reu.h"
#include "platform/reu_layout.h"
#endif

void s64_clipboard_init(S64Clipboard* clipboard)
{
    clipboard->length = 0u;
}

#ifdef S64_REU_BUFFER
int s64_clipboard_write(
    S64Clipboard* clipboard,
    unsigned int offset,
    const unsigned char* data,
    unsigned int length
)
{
    if (clipboard == 0 || data == 0
        || offset > S64_CLIPBOARD_CAPACITY
        || length > S64_CLIPBOARD_CAPACITY - offset) {
        return 0;
    }

    if (length == 0u) {
        return 1;
    }

    return s64_reu_store(
        S64_REU_CLIPBOARD_BANK,
        (uint16_t)(S64_REU_CLIPBOARD_BASE + offset),
        data,
        (unsigned int)length
    );
}

unsigned int s64_clipboard_read(
    const S64Clipboard* clipboard,
    unsigned int offset,
    unsigned char* data,
    unsigned int capacity
)
{
    unsigned int count;

    if (clipboard == 0 || data == 0 || capacity == 0u
        || offset >= clipboard->length) {
        return 0u;
    }

    count = clipboard->length - offset;
    if (count > capacity) {
        count = capacity;
    }

    if (!s64_reu_load(
            S64_REU_CLIPBOARD_BANK,
            (uint16_t)(S64_REU_CLIPBOARD_BASE + offset),
            data,
            (unsigned int)count
        )) {
        return 0u;
    }

    return count;
}
#endif
