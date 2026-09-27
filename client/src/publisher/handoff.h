#ifndef SEE64YOU_PUBLISHER_HANDOFF_H
#define SEE64YOU_PUBLISHER_HANDOFF_H

#include <stdint.h>

#include "buffer.h"
#include "document.h"
#include "history.h"
#include "storage/local.h"

#ifndef S64_REU_BUFFER
#error "publisher handoff requires S64_REU_BUFFER"
#endif

#define S64_HANDOFF_FLAG_DIRTY 0x01u
#define S64_HANDOFF_FLAG_INSERT_MODE 0x02u
#define S64_HANDOFF_FLAG_WORD_WRAP 0x04u

typedef struct {
    uint8_t flags;
    S64Buffer buffer;
    S64History history;
    S64DocumentType document_type;
    S64DocumentStatus document_status;
    int32_t order;
    char slug[S64_DOCUMENT_SLUG_CAPACITY];
    char title[S64_DOCUMENT_TITLE_CAPACITY];
    char date[S64_DOCUMENT_DATE_CAPACITY];
    char filename[S64_LOCAL_FILENAME_CAPACITY];
} S64PublisherHandoff;

void s64_handoff_init(S64PublisherHandoff* handoff);

int s64_handoff_capture(
    S64PublisherHandoff* handoff,
    const S64Buffer* buffer,
    const S64Document* document,
    const S64History* history,
    const char* filename,
    uint8_t dirty,
    uint8_t insert_mode,
    uint8_t word_wrap
);

int s64_handoff_validate(const S64PublisherHandoff* handoff);

int s64_handoff_store(const S64PublisherHandoff* handoff);
int s64_handoff_load(S64PublisherHandoff* handoff);
int s64_handoff_clear(void);

int s64_handoff_apply_document(
    const S64PublisherHandoff* handoff,
    S64Buffer* buffer,
    S64Document* document
);

#endif
