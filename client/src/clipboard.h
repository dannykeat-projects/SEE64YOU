#ifndef SEE64YOU_CLIPBOARD_H
#define SEE64YOU_CLIPBOARD_H

#ifdef S64_REU_BUFFER
#include "platform/reu_layout.h"
#define S64_CLIPBOARD_CAPACITY S64_REU_CLIPBOARD_CAPACITY

typedef struct {
    unsigned int length;
} S64Clipboard;
#else
#define S64_CLIPBOARD_CAPACITY 256u

typedef struct {
    unsigned char data[S64_CLIPBOARD_CAPACITY];
    unsigned int length;
} S64Clipboard;
#endif

void s64_clipboard_init(S64Clipboard* clipboard);

#ifdef S64_REU_BUFFER
int s64_clipboard_write(
    S64Clipboard* clipboard,
    unsigned int offset,
    const unsigned char* data,
    unsigned int length
);
unsigned int s64_clipboard_read(
    const S64Clipboard* clipboard,
    unsigned int offset,
    unsigned char* data,
    unsigned int capacity
);
#endif

#endif
