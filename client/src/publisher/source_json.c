#include "source_json.h"
#include "charset.h"
#include "publisher/workspace.h"

#include <string.h>

typedef struct {
    S64DocumentSink sink;
    void* context;
} JsonSink;

/* Each codec chunk is 64 bytes, at most doubled by escaping. The synchronous
 * request body precedes response text, so their scratch can overlap. */
#define OUTPUT_CAPACITY 128u
#define output (&s64_publisher_workspace.bytes[576])

static uint8_t emit_text(S64DocumentSink sink, void* context, const char* text)
{
    uint16_t length = (uint16_t)strlen(text);
    uint16_t i;
    if (length > OUTPUT_CAPACITY) return 0u;
    for (i = 0u; i < length; ++i) {
        if (!s64_charset_native_to_ascii((uint8_t)text[i], &output[i])) return 0u;
    }
    return sink(output, length, context);
}

static uint8_t escape_source(const uint8_t* data, uint16_t length, void* context)
{
    JsonSink* target = (JsonSink*)context;
    uint16_t i, used = 0u;
    uint8_t byte;
    for (i = 0u; i < length; ++i) {
        byte = data[i];
        if (used + 2u > OUTPUT_CAPACITY) {
            if (!target->sink(output, used, target->context)) return 0u;
            used = 0u;
        }
        if (byte == 9u || byte == 10u || byte == 34u || byte == 92u) {
            output[used++] = 92u;
            if (byte == 9u) byte = 116u;
            else if (byte == 10u) byte = 110u;
        }
        output[used++] = byte;
    }
    return !used || target->sink(output, used, target->context);
}

S64DocumentResult s64_source_write_json(
    const S64Document* document, S64DocumentSink sink, void* context)
{
    const char* type;
    const char* status;
    JsonSink target;
    S64DocumentResult result;
    if (!document || !document->body || !sink) return S64_DOCUMENT_INVALID_ARGUMENT;
    type = s64_document_type_name(document->type);
    status = s64_document_status_name(document->status);
    if (!type || !status || !s64_document_slug_valid(document->slug))
        return S64_DOCUMENT_INVALID_METADATA;
    if (!emit_text(sink, context, "{\"type\":\"")
        || !emit_text(sink, context, type)
        || !emit_text(sink, context, "\",\"slug\":\"")
        || !emit_text(sink, context, document->slug)
        || !emit_text(sink, context, "\",\"status\":\"")
        || !emit_text(sink, context, status)
        || !emit_text(sink, context, "\",\"raw8tml\":\"")) return S64_DOCUMENT_SINK_ERROR;
    target.sink = sink;
    target.context = context;
    result = s64_document_write_wire(document, escape_source, &target);
    if (result != S64_DOCUMENT_OK) return result;
    return emit_text(sink, context, "\"}") ? S64_DOCUMENT_OK : S64_DOCUMENT_SINK_ERROR;
}
