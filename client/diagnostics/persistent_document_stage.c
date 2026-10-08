#include "publisher/persistent_services.h"

#include "platform/reu.h"
#include "platform/reu_layout.h"
#define S64_STAGE_CHUNK_CAPACITY 64u
static uint8_t stage_chunk[S64_STAGE_CHUNK_CAPACITY];

static uint8_t flush_header(S64PersistentOpenStage* stage)
{
    if (!stage->header_chunk_length) return 1u;
    if ((uint16_t)(stage->header_length + stage->header_chunk_length)
        > S64_REU_HISTORY_PAYLOAD_CAPACITY) {
        stage->result = S64_DOCUMENT_SINK_ERROR;
        return 0u;
    }
    if (!s64_reu_store(
            S64_REU_HISTORY_PAYLOAD_BANK,
            (uint16_t)(S64_REU_HISTORY_PAYLOAD_BASE + stage->header_length),
            stage_chunk,
            stage->header_chunk_length)) {
        stage->result = S64_DOCUMENT_SINK_ERROR;
        return 0u;
    }
    stage->header_length = (uint16_t)(
        stage->header_length + stage->header_chunk_length);
    stage->header_chunk_length = 0u;
    return 1u;
}

static uint8_t append_header(S64PersistentOpenStage* stage, uint8_t byte)
{
    stage_chunk[stage->header_chunk_length++] = byte;
    if (stage->header_chunk_length == S64_STAGE_CHUNK_CAPACITY) {
        return flush_header(stage);
    }
    return 1u;
}

static uint8_t store_body(
    S64PersistentOpenStage* stage,
    const uint8_t* data,
    uint16_t length)
{
    if (length > (uint16_t)(S64_REU_DOCUMENT_CAPACITY - stage->body_length)) {
        stage->result = S64_DOCUMENT_SINK_ERROR;
        return 0u;
    }
    if (length && !s64_reu_store(
            S64_REU_DOCUMENT_BANK,
            (uint16_t)(S64_REU_DOCUMENT_BASE + stage->body_length),
            data,
            length)) {
        stage->result = S64_DOCUMENT_SINK_ERROR;
        return 0u;
    }
    stage->body_length = (uint16_t)(stage->body_length + length);
    return 1u;
}

void s64_persistent_open_stage_init(S64PersistentOpenStage* stage)
{
    if (!stage) return;
    stage->header_length = 0u;
    stage->body_length = 0u;
    stage->header_chunk_length = 0u;
    stage->line_empty = 1u;
    stage->body_started = 0u;
    stage->result = S64_DOCUMENT_OK;
}

uint8_t s64_persistent_open_stage_write(
    const uint8_t* data,
    uint16_t length,
    void* context)
{
    S64PersistentOpenStage* stage = (S64PersistentOpenStage*)context;
    uint16_t i = 0u;
    uint8_t byte;

    if (!stage || (length && !data) || stage->result != S64_DOCUMENT_OK) {
        return 0u;
    }

    if (stage->body_started) return store_body(stage, data, length);

    while (i < length) {
        byte = data[i++];
        if (byte == 0x0du) continue;

        if (byte == 0x0au) {
            if (stage->line_empty) {
                if (!flush_header(stage)) return 0u;
                stage->body_started = 1u;
                return store_body(
                    stage, data + i, (uint16_t)(length - i));
            }
            if (!append_header(stage, 0x0au)) return 0u;
            stage->line_empty = 1u;
        } else {
            if (!append_header(stage, byte)) return 0u;
            stage->line_empty = 0u;
        }
    }
    return 1u;
}
