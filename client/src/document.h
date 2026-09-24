#ifndef SEE64YOU_DOCUMENT_H
#define SEE64YOU_DOCUMENT_H

#include <stdint.h>

#include "buffer.h"

#define S64_DOCUMENT_SLUG_CAPACITY 65u
#define S64_DOCUMENT_TITLE_CAPACITY 81u
#define S64_DOCUMENT_DATE_CAPACITY 41u

typedef enum {
    S64_DOCUMENT_PAGE = 1,
    S64_DOCUMENT_POST = 2
} S64DocumentType;

typedef enum {
    S64_DOCUMENT_DRAFT = 1,
    S64_DOCUMENT_PUBLISHED = 2
} S64DocumentStatus;

typedef enum {
    S64_DOCUMENT_OK = 0,
    S64_DOCUMENT_INVALID_ARGUMENT,
    S64_DOCUMENT_INVALID_METADATA,
    S64_DOCUMENT_SINK_ERROR
} S64DocumentResult;

typedef uint8_t (*S64DocumentSink)(const uint8_t* data, uint16_t length, void* context);

typedef struct {
    S64DocumentType type;
    char slug[S64_DOCUMENT_SLUG_CAPACITY];
    char title[S64_DOCUMENT_TITLE_CAPACITY];
    S64DocumentStatus status;
    char date[S64_DOCUMENT_DATE_CAPACITY];
    int32_t order;
    S64Buffer* body;
} S64Document;

void s64_document_init(S64Document* document, S64Buffer* body);
int s64_document_set_slug(S64Document* document, const char* slug);
int s64_document_set_title(S64Document* document, const char* title);
int s64_document_set_date(S64Document* document, const char* date);
int s64_document_slug_valid(const char* slug);
const char* s64_document_type_name(S64DocumentType type);
const char* s64_document_status_name(S64DocumentStatus status);
S64DocumentResult s64_document_write_wire(
    const S64Document* document,
    S64DocumentSink sink,
    void* context
);

#endif
