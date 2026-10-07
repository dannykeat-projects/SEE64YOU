#include "document.h"

int s64_document_slug_valid(const char* slug)
{
    uint16_t i;
    uint8_t expect_alnum = 1u;

    if (slug == 0 || slug[0] == '\0') return 0;

    for (i = 0u; i < S64_DOCUMENT_SLUG_CAPACITY; ++i) {
        if (slug[i] == '\0') return !expect_alnum;
        if ((slug[i] >= 'a' && slug[i] <= 'z')
            || (slug[i] >= '0' && slug[i] <= '9')) {
            expect_alnum = 0u;
        } else if (slug[i] == '-' && !expect_alnum) {
            expect_alnum = 1u;
        } else {
            return 0;
        }
    }
    return 0;
}
