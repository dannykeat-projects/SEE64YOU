#ifndef SEE64YOU_DOCUMENT_H
#define SEE64YOU_DOCUMENT_H

#include <stdint.h>

#include "buffer.h"

#if defined(S64_INDEPENDENT_EDITOR) || defined(S64_INDEPENDENT_PUBLISHER)
#define S64_DOCUMENT_8TML 1
#endif

#define S64_DOCUMENT_SLUG_CAPACITY 65u
#define S64_DOCUMENT_TITLE_CAPACITY 81u
#define S64_DOCUMENT_DATE_CAPACITY 41u
#define S64_DOCUMENT_WIRE_LINE_CAPACITY 96u

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

typedef struct {
    S64DocumentType type;
    char slug[S64_DOCUMENT_SLUG_CAPACITY];
    char title[S64_DOCUMENT_TITLE_CAPACITY];
    S64DocumentStatus status;
    char date[S64_DOCUMENT_DATE_CAPACITY];
    int32_t order;
} S64DocumentSummary;

typedef struct {
    S64Document* document;
    char line[S64_DOCUMENT_WIRE_LINE_CAPACITY];
    uint16_t line_length;
    uint8_t phase;
    uint8_t seen;
    S64DocumentResult result;
} S64DocumentWireReader;

void s64_document_init(S64Document* document, S64Buffer* body);
int s64_document_set_slug(S64Document* document, const char* slug);
int s64_document_set_title(S64Document* document, const char* title);
int s64_document_set_date(S64Document* document, const char* date);
int s64_document_slug_valid(const char* slug);
const char* s64_document_type_name(S64DocumentType type);
const char* s64_document_status_name(S64DocumentStatus status);
int s64_document_type_from_name(const char* name, S64DocumentType* type);
int s64_document_status_from_name(const char* name, S64DocumentStatus* status);

void s64_document_wire_reader_init(S64DocumentWireReader* reader, S64Document* document);
uint8_t s64_document_wire_reader_write(
    const uint8_t* data,
    uint16_t length,
    void* context
);
S64DocumentResult s64_document_wire_reader_finish(S64DocumentWireReader* reader);

S64DocumentResult s64_document_write_wire(
    const S64Document* document,
    S64DocumentSink sink,
    void* context
);

#endif
