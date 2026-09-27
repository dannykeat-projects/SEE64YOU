#include "publisher/editor_handoff.h"

#include "platform/reu.h"
#include "platform/reu_layout.h"
#include "publisher/handoff_record.h"

#include <string.h>

static S64HandoffRecord record;
static const uint8_t invalid_magic[4] = {0u, 0u, 0u, 0u};

static unsigned char record_valid(void)
{
    unsigned int length;

    if (record.magic[0] != S64_HANDOFF_MAGIC_0
        || record.magic[1] != S64_HANDOFF_MAGIC_1
        || record.magic[2] != S64_HANDOFF_MAGIC_2
        || record.magic[3] != S64_HANDOFF_MAGIC_3
        || record.version != S64_HANDOFF_VERSION
        || record.checksum != s64_handoff_record_checksum(&record)
        || (record.flags & (uint8_t)~S64_HANDOFF_FLAGS_MASK) != 0u
        || record.gap_start > record.gap_end
        || record.gap_end > S64_BUFFER_CAPACITY
        || record.history.start >= S64_HISTORY_LEVELS
        || record.history.count > S64_HISTORY_LEVELS
        || record.history.cursor > record.history.count
        || record.history.pending_valid != 0u
        || record.history.saved_valid > 1u
        || (record.history.saved_valid
            && record.history.saved_cursor > record.history.count)
        || (record.document_type != S64_DOCUMENT_PAGE
            && record.document_type != S64_DOCUMENT_POST)
        || (record.document_status != S64_DOCUMENT_DRAFT
            && record.document_status != S64_DOCUMENT_PUBLISHED)) {
        return 0u;
    }

    length = (unsigned int)(
        S64_BUFFER_CAPACITY - (record.gap_end - record.gap_start)
    );
    return record.cursor <= length;
}

int s64_editor_handoff_store(
    const S64Editor* editor,
    const S64Document* document,
    const S64LocalDocument* local
)
{
    if (editor == 0 || document == 0 || local == 0
        || document->body != &editor->buffer
        || sizeof(S64HandoffRecord) > S64_REU_HANDOFF_CAPACITY) {
        return 0;
    }

    memset(&record, 0, sizeof(record));
    record.magic[0] = S64_HANDOFF_MAGIC_0;
    record.magic[1] = S64_HANDOFF_MAGIC_1;
    record.magic[2] = S64_HANDOFF_MAGIC_2;
    record.magic[3] = S64_HANDOFF_MAGIC_3;
    record.version = S64_HANDOFF_VERSION;
    if (editor->dirty) record.flags |= S64_HANDOFF_FLAG_DIRTY;
    if (editor->insert_mode) record.flags |= S64_HANDOFF_FLAG_INSERT_MODE;
    if (editor->word_wrap) record.flags |= S64_HANDOFF_FLAG_WORD_WRAP;
    record.gap_start = editor->buffer.gap_start;
    record.gap_end = editor->buffer.gap_end;
    record.cursor = editor->buffer.cursor;
    record.document_type = (uint8_t)document->type;
    record.document_status = (uint8_t)document->status;
    record.order = document->order;
    memcpy(record.slug, document->slug, sizeof(record.slug));
    memcpy(record.title, document->title, sizeof(record.title));
    memcpy(record.date, document->date, sizeof(record.date));
    memcpy(record.filename, local->filename, sizeof(record.filename));
    record.history = editor->history;
    record.checksum = s64_handoff_record_checksum(&record);

    return s64_reu_store(
        S64_REU_HANDOFF_BANK,
        S64_REU_HANDOFF_BASE,
        &record,
        (uint16_t)sizeof(record)
    );
}

int s64_editor_handoff_restore(
    S64Editor* editor,
    S64Document* document,
    S64LocalDocument* local
)
{
    if (editor == 0 || document == 0 || local == 0
        || sizeof(S64HandoffRecord) > S64_REU_HANDOFF_CAPACITY
        || !s64_reu_load(
            S64_REU_HANDOFF_BANK,
            S64_REU_HANDOFF_BASE,
            &record,
            (uint16_t)sizeof(record)
        )
        || !record_valid()) {
        return 0;
    }

    editor->buffer.gap_start = record.gap_start;
    editor->buffer.gap_end = record.gap_end;
    editor->buffer.cursor = record.cursor;
    editor->history = record.history;
    editor->dirty = (record.flags & S64_HANDOFF_FLAG_DIRTY) != 0u;
    editor->insert_mode =
        (record.flags & S64_HANDOFF_FLAG_INSERT_MODE) != 0u;
    editor->word_wrap =
        (record.flags & S64_HANDOFF_FLAG_WORD_WRAP) != 0u;

    s64_document_init(document, &editor->buffer);
    document->type = (S64DocumentType)record.document_type;
    document->status = (S64DocumentStatus)record.document_status;
    document->order = record.order;
    memcpy(document->slug, record.slug, sizeof(document->slug));
    memcpy(document->title, record.title, sizeof(document->title));
    memcpy(document->date, record.date, sizeof(document->date));
    memcpy(local->filename, record.filename, sizeof(local->filename));
    return 1;
}

int s64_editor_handoff_clear(void)
{
    return s64_reu_store(
        S64_REU_HANDOFF_BANK,
        S64_REU_HANDOFF_BASE,
        invalid_magic,
        (uint16_t)sizeof(invalid_magic)
    );
}
