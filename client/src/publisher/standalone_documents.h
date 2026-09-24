#ifndef SEE64YOU_PUBLISHER_STANDALONE_DOCUMENTS_H
#define SEE64YOU_PUBLISHER_STANDALONE_DOCUMENTS_H

#include <stdint.h>

#include "document.h"
#include "standalone.h"

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
