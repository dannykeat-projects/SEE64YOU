#ifndef SEE64YOU_PUBLISHER_STANDALONE_DOCUMENTS_H
#define SEE64YOU_PUBLISHER_STANDALONE_DOCUMENTS_H

#include <stdint.h>

#include "document.h"
#include "standalone.h"

typedef uint8_t (*S64DocumentSummarySink)(
    const S64DocumentSummary* summary,
    uint16_t index,
    void* context
);

S64StandaloneResult s64_standalone_list_documents(
    S64HttpTransport* http,
    const char* list_url,
    uint8_t allow_insecure_http,
    const S64StandaloneSession* session,
    S64DocumentSummarySink sink,
    void* sink_context,
    uint16_t* total_count,
    uint16_t* delivered_count
);

S64StandaloneResult s64_standalone_open_document(
    S64HttpTransport* http,
    const char* open_url,
    uint8_t allow_insecure_http,
    const S64StandaloneSession* session,
    S64Document* document
);

S64StandaloneResult s64_standalone_save_document(
    S64HttpTransport* http,
    const char* save_url,
    uint8_t allow_insecure_http,
    const S64StandaloneSession* session,
    const S64Document* document
);

S64StandaloneResult s64_standalone_delete_document(
    S64HttpTransport* http,
    const char* delete_url,
    uint8_t allow_insecure_http,
    const S64StandaloneSession* session,
    S64DocumentType type,
    const char* slug
);

#endif
