#ifndef SEE64YOU_SOURCE_JSON_H
#define SEE64YOU_SOURCE_JSON_H

#include "document.h"

/* Service metadata stays outside the unmodified portable source. */
S64DocumentResult s64_source_write_json(
    const S64Document* document, S64DocumentSink sink, void* context);

#endif
