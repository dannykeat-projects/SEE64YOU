#include "publisher/standard_publish_names.h"

#include <stdint.h>

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
