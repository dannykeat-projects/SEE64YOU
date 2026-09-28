#include "publisher/editor_handoff.h"

#pragma optimize(push, on)

#include "platform/reu.h"
#include "platform/reu_layout.h"
#include "publisher/handoff_record.h"

#include <string.h>

extern S64HandoffRecord s64_editor_handoff_record;

#define record s64_editor_handoff_record

static unsigned char record_header_valid(void)
{
    return record.magic[0] == S64_HANDOFF_MAGIC_0
        && record.magic[1] == S64_HANDOFF_MAGIC_1
        && record.magic[2] == S64_HANDOFF_MAGIC_2
        && record.magic[3] == S64_HANDOFF_MAGIC_3
        && record.version == S64_HANDOFF_VERSION
        && record.checksum == s64_handoff_record_checksum(&record);
}

int s64_editor_handoff_store(
    const S64Editor* editor,
    const S64Document* document,
    const S64LocalDocument* local
)
{
    record.magic[0] = S64_HANDOFF_MAGIC_0;
    record.magic[1] = S64_HANDOFF_MAGIC_1;
    record.magic[2] = S64_HANDOFF_MAGIC_2;
    record.magic[3] = S64_HANDOFF_MAGIC_3;
    record.version = S64_HANDOFF_VERSION;
    record.flags = 0u;
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
    if (!s64_reu_load(
            S64_REU_HANDOFF_BANK,
            S64_REU_HANDOFF_BASE,
            &record,
            (uint16_t)sizeof(record)
        )
        || !record_header_valid()
        || (record.flags & S64_HANDOFF_FLAG_RETURN_TO_EDITOR) == 0u) {
        return 0;
    }

    record.flags &= (uint8_t)~S64_HANDOFF_FLAG_RETURN_TO_EDITOR;
    if (!s64_reu_store(
            S64_REU_HANDOFF_BANK,
            (uint16_t)(S64_REU_HANDOFF_BASE
                + offsetof(S64HandoffRecord, flags)),
            &record.flags,
            1u
        )) {
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


#pragma optimize(pop)
