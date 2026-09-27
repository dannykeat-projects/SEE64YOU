#include "publisher/handoff.h"

#include "platform/reu.h"
#include "platform/reu_layout.h"

#include <stddef.h>
#include <string.h>

#define S64_HANDOFF_VERSION 1u
#define S64_HANDOFF_MAGIC_0 'S'
#define S64_HANDOFF_MAGIC_1 '6'
#define S64_HANDOFF_MAGIC_2 '4'
#define S64_HANDOFF_MAGIC_3 'H'
#define S64_HANDOFF_FLAGS_MASK (S64_HANDOFF_FLAG_DIRTY \
    | S64_HANDOFF_FLAG_INSERT_MODE | S64_HANDOFF_FLAG_WORD_WRAP)

typedef struct {
    char magic[4];
    uint8_t version;
    uint8_t flags;
    uint16_t gap_start;
    uint16_t gap_end;
    uint16_t cursor;
    uint8_t document_type;
    uint8_t document_status;
    int32_t order;
    char slug[S64_DOCUMENT_SLUG_CAPACITY];
    char title[S64_DOCUMENT_TITLE_CAPACITY];
    char date[S64_DOCUMENT_DATE_CAPACITY];
    char filename[S64_LOCAL_FILENAME_CAPACITY];
    S64History history;
    uint16_t checksum;
} S64HandoffRecord;

typedef char S64HandoffRecordFits[
    sizeof(S64HandoffRecord) <= S64_REU_HANDOFF_CAPACITY ? 1 : -1
];

static S64HandoffRecord record;

static uint8_t text_valid(
    const char* text,
    uint16_t capacity
)
{
    uint16_t i;

    if (text == 0 || capacity == 0u) {
        return 0u;
    }

    for (i = 0u; i < capacity; ++i) {
        if (text[i] == '\0') {
            return 1u;
        }
        if (text[i] == '\r' || text[i] == '\n') {
            return 0u;
        }
    }

    return 0u;
}

static uint16_t record_checksum(const S64HandoffRecord* value)
{
    const uint8_t* bytes = (const uint8_t*)value;
    uint16_t sum = 0x6411u;
    uint16_t i;

    for (i = 0u; i < (uint16_t)offsetof(S64HandoffRecord, checksum); ++i) {
        sum = (uint16_t)((sum << 1) ^ bytes[i] ^ (uint8_t)i);
    }

    return sum;
}

static uint8_t history_valid(const S64History* history)
{
    return history != 0
        && history->start < S64_HISTORY_LEVELS
        && history->count <= S64_HISTORY_LEVELS
        && history->cursor <= history->count
        && history->pending_valid == 0u
        && history->saved_valid <= 1u
        && (!history->saved_valid || history->saved_cursor <= history->count);
}

static uint8_t buffer_valid(const S64Buffer* buffer)
{
    unsigned int length;

    if (buffer == 0
        || buffer->gap_start > buffer->gap_end
        || buffer->gap_end > S64_BUFFER_CAPACITY) {
        return 0u;
    }

    length = (unsigned int)(
        S64_BUFFER_CAPACITY - (buffer->gap_end - buffer->gap_start)
    );
    return buffer->cursor <= length;
}

void s64_handoff_init(S64PublisherHandoff* handoff)
{
    if (handoff != 0) {
        memset(handoff, 0, sizeof(*handoff));
        handoff->buffer.gap_end = S64_BUFFER_CAPACITY;
        handoff->document_type = S64_DOCUMENT_PAGE;
        handoff->document_status = S64_DOCUMENT_DRAFT;
    }
}

int s64_handoff_capture(
    S64PublisherHandoff* handoff,
    const S64Buffer* buffer,
    const S64Document* document,
    const S64History* history,
    const char* filename,
    uint8_t dirty,
    uint8_t insert_mode,
    uint8_t word_wrap
)
{
    if (handoff == 0 || buffer == 0 || document == 0 || history == 0
        || filename == 0 || document->body != buffer
        || !buffer_valid(buffer) || !history_valid(history)
        || !text_valid(document->slug, S64_DOCUMENT_SLUG_CAPACITY)
        || !text_valid(document->title, S64_DOCUMENT_TITLE_CAPACITY)
        || !text_valid(document->date, S64_DOCUMENT_DATE_CAPACITY)
        || !text_valid(filename, S64_LOCAL_FILENAME_CAPACITY)) {
        return 0;
    }

    s64_handoff_init(handoff);
    if (dirty) handoff->flags |= S64_HANDOFF_FLAG_DIRTY;
    if (insert_mode) handoff->flags |= S64_HANDOFF_FLAG_INSERT_MODE;
    if (word_wrap) handoff->flags |= S64_HANDOFF_FLAG_WORD_WRAP;

    handoff->buffer = *buffer;
    handoff->history = *history;
    handoff->document_type = document->type;
    handoff->document_status = document->status;
    handoff->order = document->order;
    memcpy(handoff->slug, document->slug, sizeof(handoff->slug));
    memcpy(handoff->title, document->title, sizeof(handoff->title));
    memcpy(handoff->date, document->date, sizeof(handoff->date));
    memcpy(handoff->filename, filename, sizeof(handoff->filename));

    return s64_handoff_validate(handoff);
}

int s64_handoff_validate(const S64PublisherHandoff* handoff)
{
    if (handoff == 0
        || (handoff->flags & (uint8_t)~S64_HANDOFF_FLAGS_MASK) != 0u
        || !buffer_valid(&handoff->buffer)
        || !history_valid(&handoff->history)
        || (handoff->document_type != S64_DOCUMENT_PAGE
            && handoff->document_type != S64_DOCUMENT_POST)
        || (handoff->document_status != S64_DOCUMENT_DRAFT
            && handoff->document_status != S64_DOCUMENT_PUBLISHED)
        || !text_valid(handoff->slug, S64_DOCUMENT_SLUG_CAPACITY)
        || !text_valid(handoff->title, S64_DOCUMENT_TITLE_CAPACITY)
        || !text_valid(handoff->date, S64_DOCUMENT_DATE_CAPACITY)
        || !text_valid(handoff->filename, S64_LOCAL_FILENAME_CAPACITY)) {
        return 0;
    }

    return 1;
}

int s64_handoff_store(const S64PublisherHandoff* handoff)
{
    if (!s64_handoff_validate(handoff)) {
        return 0;
    }

    memset(&record, 0, sizeof(record));
    record.magic[0] = S64_HANDOFF_MAGIC_0;
    record.magic[1] = S64_HANDOFF_MAGIC_1;
    record.magic[2] = S64_HANDOFF_MAGIC_2;
    record.magic[3] = S64_HANDOFF_MAGIC_3;
    record.version = S64_HANDOFF_VERSION;
    record.flags = handoff->flags;
    record.gap_start = handoff->buffer.gap_start;
    record.gap_end = handoff->buffer.gap_end;
    record.cursor = handoff->buffer.cursor;
    record.document_type = (uint8_t)handoff->document_type;
    record.document_status = (uint8_t)handoff->document_status;
    record.order = handoff->order;
    memcpy(record.slug, handoff->slug, sizeof(record.slug));
    memcpy(record.title, handoff->title, sizeof(record.title));
    memcpy(record.date, handoff->date, sizeof(record.date));
    memcpy(record.filename, handoff->filename, sizeof(record.filename));
    record.history = handoff->history;
    record.checksum = record_checksum(&record);

    return s64_reu_store(
        S64_REU_HANDOFF_BANK,
        S64_REU_HANDOFF_BASE,
        &record,
        (uint16_t)sizeof(record)
    );
}

int s64_handoff_load(S64PublisherHandoff* handoff)
{
    if (handoff == 0
        || !s64_reu_load(
            S64_REU_HANDOFF_BANK,
            S64_REU_HANDOFF_BASE,
            &record,
            (uint16_t)sizeof(record)
        )
        || record.magic[0] != S64_HANDOFF_MAGIC_0
        || record.magic[1] != S64_HANDOFF_MAGIC_1
        || record.magic[2] != S64_HANDOFF_MAGIC_2
        || record.magic[3] != S64_HANDOFF_MAGIC_3
        || record.version != S64_HANDOFF_VERSION
        || record.checksum != record_checksum(&record)) {
        return 0;
    }

    s64_handoff_init(handoff);
    handoff->flags = record.flags;
    handoff->buffer.gap_start = record.gap_start;
    handoff->buffer.gap_end = record.gap_end;
    handoff->buffer.cursor = record.cursor;
    handoff->history = record.history;
    handoff->document_type = (S64DocumentType)record.document_type;
    handoff->document_status = (S64DocumentStatus)record.document_status;
    handoff->order = record.order;
    memcpy(handoff->slug, record.slug, sizeof(handoff->slug));
    memcpy(handoff->title, record.title, sizeof(handoff->title));
    memcpy(handoff->date, record.date, sizeof(handoff->date));
    memcpy(handoff->filename, record.filename, sizeof(handoff->filename));

    return s64_handoff_validate(handoff);
}

int s64_handoff_clear(void)
{
    static const uint8_t invalid_magic[4] = {0u, 0u, 0u, 0u};

    return s64_reu_store(
        S64_REU_HANDOFF_BANK,
        S64_REU_HANDOFF_BASE,
        invalid_magic,
        (uint16_t)sizeof(invalid_magic)
    );
}

int s64_handoff_apply_document(
    const S64PublisherHandoff* handoff,
    S64Buffer* buffer,
    S64Document* document
)
{
    if (!s64_handoff_validate(handoff) || buffer == 0 || document == 0) {
        return 0;
    }

    *buffer = handoff->buffer;
    s64_document_init(document, buffer);
    document->type = handoff->document_type;
    document->status = handoff->document_status;
    document->order = handoff->order;
    memcpy(document->slug, handoff->slug, sizeof(document->slug));
    memcpy(document->title, handoff->title, sizeof(document->title));
    memcpy(document->date, handoff->date, sizeof(document->date));
    return 1;
}
