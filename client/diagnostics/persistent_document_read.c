#include "publisher/persistent_services.h"

#include "charset.h"
#include "platform/reu.h"
#include "platform/reu_layout.h"
#include "publisher/workspace.h"

#include <limits.h>
#include <string.h>

#define S64_STAGE_READ_CHUNK 96u
#define read_chunk ((uint8_t*)&s64_publisher_workspace.bytes[0])
#define verify_chunk ((uint8_t*)&s64_publisher_workspace.bytes[S64_STAGE_READ_CHUNK])

#define S64_WIRE_PHASE_VERSION 0u
#define S64_WIRE_PHASE_HEADERS 1u

#define S64_WIRE_SEEN_TYPE   0x01u
#define S64_WIRE_SEEN_SLUG   0x02u
#define S64_WIRE_SEEN_TITLE  0x04u
#define S64_WIRE_SEEN_STATUS 0x08u
#define S64_WIRE_SEEN_DATE   0x10u
#define S64_WIRE_SEEN_ORDER  0x20u
#define S64_WIRE_SEEN_REQUIRED 0x3fu

static uint8_t copy_text(char* output, uint16_t capacity, const char* text)
{
    uint16_t length = (uint16_t)strlen(text);
    if (length >= capacity) return 0u;
    memcpy(output, text, length + 1u);
    return 1u;
}

static uint8_t parse_order(const char* text, int32_t* output)
{
    uint8_t negative = 0u;
    uint8_t digits = 0u;
    uint8_t digit;
    uint32_t value = 0u;

    if (*text == '-') {
        negative = 1u;
        ++text;
    } else if (*text == '+') {
        ++text;
    }

    while (*text >= '0' && *text <= '9') {
        digit = (uint8_t)(*text - '0');
        if (value > 214748364UL
            || (value == 214748364UL
                && digit > (uint8_t)(negative ? 8u : 7u))) return 0u;
        value = value * 10u + digit;
        ++digits;
        ++text;
    }
    if (!digits || *text) return 0u;

    if (negative && value == 2147483648UL) {
        *output = (int32_t)INT32_MIN;
    } else {
        *output = negative ? -(int32_t)value : (int32_t)value;
    }
    return 1u;
}

static uint8_t parse_header(S64DocumentWireReader* reader)
{
    char* colon;
    const char* value;
    uint8_t bit = 0u;

    colon = strchr(reader->line, ':');
    if (!colon || colon[1] != ' ') return 0u;
    *colon = 0;
    value = colon + 2;

    if (!strcmp(reader->line, "Type")) {
        bit = S64_WIRE_SEEN_TYPE;
        if (!strcmp(value, "page")) reader->document->type = S64_DOCUMENT_PAGE;
        else if (!strcmp(value, "post")) reader->document->type = S64_DOCUMENT_POST;
        else return 0u;
    } else if (!strcmp(reader->line, "Slug")) {
        bit = S64_WIRE_SEEN_SLUG;
        if (!s64_persistent_document_slug_valid(value)
            || !copy_text(reader->document->slug,
                S64_DOCUMENT_SLUG_CAPACITY, value)) return 0u;
    } else if (!strcmp(reader->line, "Title")) {
        bit = S64_WIRE_SEEN_TITLE;
        if (!*value || !copy_text(reader->document->title,
            S64_DOCUMENT_TITLE_CAPACITY, value)) return 0u;
    } else if (!strcmp(reader->line, "Status")) {
        bit = S64_WIRE_SEEN_STATUS;
        if (!strcmp(value, "draft")) reader->document->status = S64_DOCUMENT_DRAFT;
        else if (!strcmp(value, "published")) {
            reader->document->status = S64_DOCUMENT_PUBLISHED;
        } else return 0u;
    } else if (!strcmp(reader->line, "Date")) {
        bit = S64_WIRE_SEEN_DATE;
        if (!copy_text(reader->document->date,
            S64_DOCUMENT_DATE_CAPACITY, value)) return 0u;
    } else if (!strcmp(reader->line, "Order")) {
        bit = S64_WIRE_SEEN_ORDER;
        if (!parse_order(value, &reader->document->order)) return 0u;
    } else {
        return 1u;
    }

    if (reader->seen & bit) return 0u;
    reader->seen = (uint8_t)(reader->seen | bit);
    return 1u;
}

static uint8_t finish_line(S64DocumentWireReader* reader)
{
    reader->line[reader->line_length] = 0;
    if (reader->phase == S64_WIRE_PHASE_VERSION) {
        if (strcmp(reader->line, "SEE64YOU/1")) return 0u;
        reader->phase = S64_WIRE_PHASE_HEADERS;
    } else if (!parse_header(reader)) {
        return 0u;
    }
    reader->line_length = 0u;
    return 1u;
}

static uint8_t parse_staged_header(
    const S64PersistentOpenStage* stage,
    S64Document* document)
{
    S64DocumentWireReader reader;
    uint16_t offset = 0u;
    uint16_t amount;
    uint16_t i;
    uint8_t native;

    memset(&reader, 0, sizeof(reader));
    reader.document = document;

    while (offset < stage->header_length) {
        amount = (uint16_t)(stage->header_length - offset);
        if (amount > S64_STAGE_READ_CHUNK) amount = S64_STAGE_READ_CHUNK;
        if (!s64_reu_load(
                (uint8_t)((stage->address + offset) >> 16),
                (uint16_t)(stage->address + offset),
                read_chunk,
                amount)) return 0u;

        for (i = 0u; i < amount; ++i) {
            if (read_chunk[i] == 0x0au) {
                if (!finish_line(&reader)) return 0u;
                continue;
            }
            if (!s64_charset_ascii_to_native(read_chunk[i], &native)) return 0u;
            if (reader.line_length + 1u >= S64_DOCUMENT_WIRE_LINE_CAPACITY)
                return 0u;
            reader.line[reader.line_length++] = (char)native;
        }
        offset = (uint16_t)(offset + amount);
    }

    return reader.phase == S64_WIRE_PHASE_HEADERS
        && reader.line_length == 0u
        && reader.seen == S64_WIRE_SEEN_REQUIRED;
}

static uint8_t convert_body(const S64PersistentOpenStage* stage)
{
    uint16_t offset = 0u;
    uint16_t amount;
    uint16_t i;

    while (offset < stage->body_length) {
        amount = (uint16_t)(stage->body_length - offset);
        if (amount > S64_STAGE_READ_CHUNK) amount = S64_STAGE_READ_CHUNK;
        if (!s64_reu_load(
                (uint8_t)((stage->address + S64_OPEN_BODY_OFFSET + offset) >> 16),
                (uint16_t)(stage->address + S64_OPEN_BODY_OFFSET + offset),
                read_chunk,
                amount)) return 0u;

        for (i = 0u; i < amount; ++i) {
            if (!s64_charset_ascii_to_native(read_chunk[i], &read_chunk[i]))
                return 0u;
        }

        if (!s64_reu_store(
                (uint8_t)((stage->address + S64_OPEN_BODY_OFFSET + offset) >> 16),
                (uint16_t)(stage->address + S64_OPEN_BODY_OFFSET + offset),
                read_chunk,
                amount)
            || !s64_reu_load(
                (uint8_t)((stage->address + S64_OPEN_BODY_OFFSET + offset) >> 16),
                (uint16_t)(stage->address + S64_OPEN_BODY_OFFSET + offset),
                verify_chunk, amount)
            || memcmp(read_chunk, verify_chunk, amount)) return 0u;
        offset = (uint16_t)(offset + amount);
    }
    return 1u;
}

/* Readback matters: the hardware DMA wrapper cannot report a bus failure. */
static uint8_t copy_verified(uint32_t source, uint32_t target, uint16_t length)
{
    uint16_t amount;

    while (length) {
        amount = length > S64_STAGE_READ_CHUNK ? S64_STAGE_READ_CHUNK : length;
        if (!s64_reu_load((uint8_t)(source >> 16), (uint16_t)source,
                read_chunk, amount)
            || !s64_reu_store((uint8_t)(target >> 16), (uint16_t)target,
                read_chunk, amount)
            || !s64_reu_load((uint8_t)(target >> 16), (uint16_t)target,
                verify_chunk, amount)
            || memcmp(read_chunk, verify_chunk, amount)) return 0u;
        source += amount;
        target += amount;
        length = (uint16_t)(length - amount);
    }
    return 1u;
}

uint8_t s64_persistent_open_stage_restore(S64PersistentOpenStage* stage)
{
    if (!stage || !stage->recovery_required) return 0u;
    if (!copy_verified(stage->address + S64_OPEN_BACKUP_OFFSET,
            S64_REU_DOCUMENT_BASE, S64_REU_DOCUMENT_CAPACITY)) return 0u;
    stage->recovery_required = 0u;
    return 1u;
}

S64DocumentResult s64_persistent_open_stage_commit(
    S64PersistentOpenStage* stage,
    S64Document* document)
{
    S64Document candidate;

    if (!stage || !document || !document->body)
        return S64_DOCUMENT_INVALID_ARGUMENT;
    if (stage->recovery_required || stage->result != S64_DOCUMENT_OK)
        return S64_DOCUMENT_SINK_ERROR;
    if (stage->complete != 1u || !stage->body_started
        || stage->address < S64_REU_RUNTIME_HEAP_START
        || stage->address > S64_REU_RUNTIME_HEAP_END - S64_OPEN_STORAGE_SIZE
        || stage->header_length > S64_OPEN_BODY_OFFSET
        || stage->body_length > S64_REU_DOCUMENT_CAPACITY)
        return S64_DOCUMENT_INVALID_METADATA;

    /* A stage is single-use: partial conversion must not be retried as ASCII. */
    stage->complete = 2u;
    stage->result = S64_DOCUMENT_INVALID_METADATA;
    memset(&candidate, 0, sizeof(candidate));
    candidate.body = document->body;
    if (!parse_staged_header(stage, &candidate)) return stage->result;

    stage->result = S64_DOCUMENT_SINK_ERROR;
    if (!convert_body(stage)
        || !copy_verified(S64_REU_DOCUMENT_BASE,
            stage->address + S64_OPEN_BACKUP_OFFSET, S64_REU_DOCUMENT_CAPACITY))
        return stage->result;

    /* Keep the original physical gap-buffer layout until replacement verifies.
     * No Editor controls, history or metadata have changed at this point. */
    stage->recovery_required = 1u;
    if (!copy_verified(stage->address + S64_OPEN_BODY_OFFSET,
            S64_REU_DOCUMENT_BASE, stage->body_length)) {
        (void)s64_persistent_open_stage_restore(stage);
        return stage->result;
    }
    stage->recovery_required = 0u;
    s64_buffer_init(document->body);
    document->body->gap_start = stage->body_length;
    document->body->gap_end = S64_BUFFER_CAPACITY;
    document->body->cursor = 0u;
    *document = candidate;
    stage->result = S64_DOCUMENT_OK;
    return S64_DOCUMENT_OK;
}
