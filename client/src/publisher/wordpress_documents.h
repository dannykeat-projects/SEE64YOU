#ifndef SEE64YOU_PUBLISHER_WORDPRESS_DOCUMENTS_H
#define SEE64YOU_PUBLISHER_WORDPRESS_DOCUMENTS_H

#include <stdint.h>

#include "document.h"
#include "wordpress.h"

#define S64_WORDPRESS_ID_CAPACITY 21u

typedef struct {
    char id[S64_WORDPRESS_ID_CAPACITY];
    S64DocumentType type;
    char slug[S64_DOCUMENT_SLUG_CAPACITY];
    char title[S64_DOCUMENT_TITLE_CAPACITY];
    S64DocumentStatus status;
    char date[S64_DOCUMENT_DATE_CAPACITY];
    int32_t order;
} S64WordPressDocumentSummary;

typedef uint8_t (*S64WordPressDocumentSummarySink)(
    const S64WordPressDocumentSummary* summary,
    uint16_t index,
    void* context
);

S64WordPressResult s64_wordpress_list_documents(
    S64HttpTransport* http,
    const char* list_url,
    uint8_t allow_insecure_http,
    const S64WordPressSession* session,
    S64WordPressDocumentSummarySink sink,
    void* sink_context,
    uint16_t* total_count,
    uint16_t* delivered_count
);

S64WordPressResult s64_wordpress_open_document(
    S64HttpTransport* http,
    const char* open_url,
    uint8_t allow_insecure_http,
    const S64WordPressSession* session,
    S64Document* document
);

/*
 * save_url should target /documents?ack=1 for create or
 * /documents/<id>?ack=1 for update.
 */
S64WordPressResult s64_wordpress_save_document(
    S64HttpTransport* http,
    const char* save_url,
    uint8_t allow_insecure_http,
    const S64WordPressSession* session,
    const S64Document* document,
    uint8_t updating,
    char* id,
    uint16_t id_capacity
);

S64WordPressResult s64_wordpress_delete_document(
    S64HttpTransport* http,
    const char* delete_url,
    uint8_t allow_insecure_http,
    const S64WordPressSession* session
);

#endif
