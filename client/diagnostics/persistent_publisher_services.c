#include "document.h"

int s64_document_slug_valid(const char* slug)
{
    uint16_t i = 0u;
    unsigned char c;

    if (slug == 0 || slug[0] == '\0') return 0;
    while (slug[i] != '\0') {
        c = (unsigned char)slug[i];
        if (i >= S64_DOCUMENT_SLUG_CAPACITY - 1u
            || !((c >= 'a' && c <= 'z')
                || (c >= 'A' && c <= 'Z')
                || (c >= '0' && c <= '9')
                || c == '-' || c == '_')) {
            return 0;
        }
        ++i;
    }
    return 1;
}
