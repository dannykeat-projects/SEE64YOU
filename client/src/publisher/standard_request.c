#include "publisher/standard_request.h"
#include "publisher/standard_publish_names.h"

#include <string.h>

static int append(char* out, uint16_t cap, uint16_t* used, const char* text)
{
    uint16_t i = 0u;
    while (text[i]) {
        if ((uint32_t)*used + i + 1u >= cap) return 0;
        out[*used + i] = text[i];
        ++i;
    }
    *used = (uint16_t)(*used + i);
    out[*used] = 0;
    return 1;
}

static int append_size(char* out, uint16_t cap, uint16_t* used, uint32_t size)
{
    char digits[10];
    uint8_t n = 0u;
    do {
        digits[n++] = (char)('0' + (size % 10u));
        size /= 10u;
    } while (size);
    while (n) {
        char digit[2];
        digit[0] = digits[--n]; digit[1] = 0;
        if (!append(out, cap, used, digit)) return 0;
    }
    return 1;
}

int s64_publish_request_path(char* output, uint16_t capacity,
    const char* endpoint_path, S64PublishRequestKind kind,
    const char* id, const char* name, uint32_t size,
    const char* challenge)
{
    uint16_t used = 0u, i;
    if (!output || capacity == 0u) return 0;
    output[0] = 0;
    if (!endpoint_path || endpoint_path[0] != '/') return 0;
    for (i = 0u; endpoint_path[i] != 0; ++i) {
        char ch = endpoint_path[i];
        if (i >= capacity || ch == '?' || ch == '#'
            || ch == '\r' || ch == '\n' || ch == ' '
            || ch == '\t') return 0;
    }
    if ((kind != S64_PUBLISH_REQUEST_STATUS
            && (!id || !s64_publish_id_valid(id)))
        || (kind == S64_PUBLISH_REQUEST_LIST && size != 0u)
        || (kind == S64_PUBLISH_REQUEST_PUT
            && (!name || !s64_publish_name_valid(name)))
        || ((kind == S64_PUBLISH_REQUEST_LIST || kind == S64_PUBLISH_REQUEST_PUT)
            && !s64_publish_challenge_valid(challenge))) return 0;
    if (!append(output, capacity, &used, endpoint_path)) goto invalid;
    switch (kind) {
        case S64_PUBLISH_REQUEST_STATUS:
            if (!append(output, capacity, &used, "?op=status")) goto invalid;
            break;
        case S64_PUBLISH_REQUEST_CHALLENGE:
            if (!append(output, capacity, &used, "?op=challenge&id=")
                || !append(output, capacity, &used, id)) goto invalid;
            break;
        case S64_PUBLISH_REQUEST_LIST:
            if (!append(output, capacity, &used, "?op=list&id=")
                || !append(output, capacity, &used, id)
                || !append(output, capacity, &used, "&size=0&c=")
                || !append(output, capacity, &used, challenge)) goto invalid;
            break;
        case S64_PUBLISH_REQUEST_PUT:
            if (!append(output, capacity, &used, "?op=put&id=")
                || !append(output, capacity, &used, id)
                || !append(output, capacity, &used, "&name=")
                || !append(output, capacity, &used, name)
                || !append(output, capacity, &used, "&size=")
                || !append_size(output, capacity, &used, size)
                || !append(output, capacity, &used, "&c=")
                || !append(output, capacity, &used, challenge)) goto invalid;
            break;
        default: goto invalid;
    }
    return 1;
invalid:
    output[0] = 0;
    return 0;
}
