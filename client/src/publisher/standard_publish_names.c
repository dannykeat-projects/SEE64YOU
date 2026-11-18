#include "publisher/standard_publish_names.h"

#include <stdint.h>
#include <string.h>

unsigned char s64_publish_source_kind(const char* filename)
{
    const char* suffix;
    uint8_t length;
#define FOLD(c) (((uint8_t)(c) | 0x20u) & 0x7fu)
    if (!filename || !(suffix = strrchr(filename, '.'))) return 0u;
    length = (uint8_t)strlen(suffix);
    if (length == 5u && suffix[1] == '8' && FOLD(suffix[2]) == 0x74u
        && FOLD(suffix[3]) == 0x6du && FOLD(suffix[4]) == 0x6cu) return 1u;
    if (length == 4u) {
        if (suffix[1] == '8' && FOLD(suffix[2]) == 0x73u && FOLD(suffix[3]) == 0x73u) return 2u;
        if (FOLD(suffix[1]) == 0x74u && FOLD(suffix[2]) == 0x78u && FOLD(suffix[3]) == 0x74u) return 3u;
    }
    return 0u;
#undef FOLD
}

int s64_publish_id_valid(const char* id)
{
    uint8_t count = 0u;
    char ch;
    if (!id) return 0;
    while ((ch = *id++) != 0) {
        if (++count > 32u || !((ch >= 'a' && ch <= 'z')
            || (ch >= '0' && ch <= '9') || ch == '-')) return 0;
    }
    return count != 0u;
}

int s64_publish_name_valid(const char* name)
{
    uint8_t count = 0u, segment = 0u, dots = 0u;
    char ch;
    if (!name) return 0;
    while ((ch = *name++) != 0) {
        if (++count > 64u) return 0;
        if (ch == '/') {
            if (!segment || (dots == segment && dots <= 2u)) return 0;
            segment = dots = 0u;
        } else {
            if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
                || (ch >= '0' && ch <= '9') || ch == '-' || ch == '_'
                || ch == '.')) return 0;
            ++segment;
            if (ch == '.' && dots < 3u) ++dots;
            else if (ch != '.') dots = 3u;
        }
    }
    return segment != 0u && !(dots == segment && dots <= 2u);
}

int s64_publish_challenge_valid(const char* challenge)
{
    uint8_t i;
    if (!challenge || strlen(challenge) != 32u) return 0;
    for (i = 0u; i < 32u; ++i) {
        if (!((challenge[i] >= '0' && challenge[i] <= '9')
            || (challenge[i] >= 'a' && challenge[i] <= 'f'))) return 0;
    }
    return 1;
}
