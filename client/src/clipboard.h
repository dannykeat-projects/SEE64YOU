#ifndef SEE64YOU_CLIPBOARD_H
#define SEE64YOU_CLIPBOARD_H

#define S64_CLIPBOARD_CAPACITY 512u

typedef struct {
    unsigned char data[S64_CLIPBOARD_CAPACITY];
    unsigned int length;
} S64Clipboard;

void s64_clipboard_init(S64Clipboard* clipboard);

#endif
