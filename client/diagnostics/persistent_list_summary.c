#include "publisher/persistent_services.h"

#include "publisher/workspace.h"

#include <string.h>

#define list_reply ((char*)&s64_publisher_workspace.bytes[S64_PUBLISH_WORK_DIRECT_REPLY_OFFSET])

uint8_t s64_persistent_list_summary_parse(
    S64DocumentType type,
    uint16_t index,
    S64DocumentSummary* summary,
    uint16_t* total_count)
{
    static const char prefix[] = "SEE64YOU-LIST/1\nCount: ";
    static const char slug_prefix[] = "Slug: ";
    static const char title_prefix[] = "Title: ";
    uint16_t count = 0u;
    uint16_t value_length;
    char* cursor;
    char* end;

    if (!summary || !total_count
        || (type != S64_DOCUMENT_PAGE && type != S64_DOCUMENT_POST)) {
        return 0u;
    }

    cursor = list_reply;
    if (strncmp(cursor, prefix, sizeof(prefix) - 1u) != 0) return 0u;
    cursor += sizeof(prefix) - 1u;
    if (*cursor < '0' || *cursor > '9') return 0u;
    while (*cursor >= '0' && *cursor <= '9') {
        uint8_t digit = (uint8_t)(*cursor++ - '0');
        if (count > 6553u || (count == 6553u && digit > 5u)) return 0u;
        count = (uint16_t)(count * 10u + digit);
    }
    if (*cursor++ != '\n') return 0u;
    *total_count = count;

    memset(summary, 0, sizeof(*summary));
    summary->type = type;
    summary->status = S64_DOCUMENT_DRAFT;

    if (index >= count) return *cursor == '\n' && cursor[1] == 0;

    if (strncmp(cursor, slug_prefix, sizeof(slug_prefix) - 1u) != 0) return 0u;
    cursor += sizeof(slug_prefix) - 1u;
    end = strchr(cursor, '\n');
    if (!end) return 0u;
    value_length = (uint16_t)(end - cursor);
    if (!value_length || value_length >= sizeof(summary->slug)) return 0u;
    memcpy(summary->slug, cursor, value_length);
    summary->slug[value_length] = 0;
    if (!s64_persistent_document_slug_valid(summary->slug)) return 0u;

    cursor = end + 1;
    if (strncmp(cursor, title_prefix, sizeof(title_prefix) - 1u) != 0) return 0u;
    cursor += sizeof(title_prefix) - 1u;
    end = strchr(cursor, '\n');
    if (!end) return 0u;
    value_length = (uint16_t)(end - cursor);
    if (!value_length || value_length >= sizeof(summary->title)
        || end[1] != '\n' || end[2] != 0) return 0u;
    memcpy(summary->title, cursor, value_length);
    summary->title[value_length] = 0;
    return 1u;
}
