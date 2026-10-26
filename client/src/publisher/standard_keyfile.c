#include "publisher/standard_keyfile.h"
#include "publisher/standard_publish.h"
#include "publisher/direct_auth.h"
#include "charset.h"

#include <string.h>

static int name_is(const uint8_t* data, uint16_t length, const char* native)
{
    uint16_t i;
    uint8_t converted;
    if (strlen(native) != length) return 0;
    for (i = 0u; i < length; ++i) {
        if (!s64_charset_ascii_to_native(data[i], &converted)
            || converted != (uint8_t)native[i]) return 0;
    }
    return 1;
}

static int copy_native(char* dest, uint16_t capacity,
    const uint8_t* data, uint16_t length)
{
    uint16_t i;
    uint8_t converted;
    if (!length || length >= capacity) return 0;
    for (i = 0u; i < length; ++i) {
        if (!s64_charset_ascii_to_native(data[i], &converted)) return 0;
        dest[i] = (char)converted;
    }
    dest[length] = 0;
    return 1;
}

static int url_valid(const char* url)
{
    const char* authority;
    const char* path;
    if (strncmp(url, "http://", 7u) == 0) authority = url + 7;
    else if (strncmp(url, "https://", 8u) == 0) authority = url + 8;
    else return 0;
    path = strchr(authority, '/');
    return path && path != authority && path[1] != 0
        && !strpbrk(authority, "@?# \r\n\t");
}

int s64_publish_keyfile_parse(const uint8_t* data, uint16_t length,
    S64PublishKeyFile* output)
{
    uint16_t start = 0u, end, name_end, value_start, value_end, i;
    uint8_t seen = 0u, field;
    uint8_t raw_key[32];
    if (!output) return 0;
    memset(output, 0, sizeof(*output));
    if (!data || length == 0u || length > S64_PUBLISH_KEYFILE_MAX_BYTES)
        return 0;
    while (start < length) {
        end = start;
        while (end < length && data[end] != '\n') {
            if ((uint16_t)(end - start) >= 255u) goto invalid;
            if (data[end] != '\r' && data[end] != '\t'
                && (data[end] < 32u || data[end] > 126u)) goto invalid;
            ++end;
        }
        value_end = end;
        if (value_end > start && data[value_end - 1u] == '\r') --value_end;
        for (i = start; i < value_end; ++i)
            if (data[i] == '\r') goto invalid;
        name_end = start;
        while (name_end < value_end && data[name_end] != ':') ++name_end;
        if (name_end == value_end) {
            if (value_end != start) goto invalid;
            start = end + 1u;
            continue;
        }
        value_start = name_end + 1u;
        while (value_start < value_end &&
               (data[value_start] == ' ' || data[value_start] == '\t'))
            ++value_start;
        while (value_end > value_start &&
               (data[value_end - 1u] == ' ' || data[value_end - 1u] == '\t'))
            --value_end;
        field = 0u;
        if (name_is(data + start, name_end - start, "8tml-publish")) field = 1u;
        else if (name_is(data + start, name_end - start, "url")) field = 2u;
        else if (name_is(data + start, name_end - start, "id")) field = 4u;
        else if (name_is(data + start, name_end - start, "key")) field = 8u;
        if (field) {
            if (seen & field) goto invalid;
            seen |= field;
            if (field == 1u) {
                if (!name_is(data + value_start, value_end - value_start, "1"))
                    goto invalid;
            } else if (field == 2u) {
                if (!copy_native(output->url, sizeof(output->url),
                    data + value_start, value_end - value_start)
                    || !url_valid(output->url)) goto invalid;
            } else if (field == 4u) {
                if (!copy_native(output->id, sizeof(output->id),
                    data + value_start, value_end - value_start)
                    || !s64_publish_id_valid(output->id)) goto invalid;
            } else {
                if (!copy_native(output->key, sizeof(output->key),
                    data + value_start, value_end - value_start)
                    || !s64_direct_key_parse(output->key, raw_key)) goto invalid;
                memset(raw_key, 0, sizeof(raw_key));
            }
        }
        start = end + 1u;
    }
    if (seen == 15u) return 1;
invalid:
    memset(raw_key, 0, sizeof(raw_key));
    memset(output, 0, sizeof(*output));
    return 0;
}
