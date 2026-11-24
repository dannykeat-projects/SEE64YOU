#ifndef SEE64YOU_WRAP_H
#define SEE64YOU_WRAP_H

#include <string.h>

#define S64_WRAP_COLUMNS 40u

/* Input includes one lookahead byte so a word exactly at the edge fits. */
static unsigned char s64_wrap_width(const unsigned char* row, unsigned char count)
{
    unsigned char width = count > S64_WRAP_COLUMNS ? S64_WRAP_COLUMNS : count;
    const unsigned char* newline = memchr(row, '\n', width);
    unsigned char split;

    if (newline != 0) return (unsigned char)(newline - row);
    if (count == width || row[width] == '\n' || row[width] == ' ') return width;

    for (split = width; split > 0u; --split) {
        if (row[split - 1u] == ' ') return split;
    }
    return width;
}

#endif
