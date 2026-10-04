#include <string.h>
#include "platform/managed_publisher.h"
#include "platform/application_state.h"
#include "publisher/workspace.h"

static S64ApplicationState application;
extern S64ModuleRuntime* s64_managed_publisher_document_owner;

uint8_t s64_managed_publisher_document(S64ModuleRuntime* runtime,
    S64Buffer* body, S64Document* document, uint8_t* dirty, uint8_t* replaced)
{
    uint8_t authorized = runtime && s64_managed_publisher_document_owner == runtime;
    s64_managed_publisher_document_owner = 0;
    if (!authorized || !body || !document || !dirty || !replaced
        || runtime->current_module != S64_MODULE_PUBLISHER
        || !s64_application_state_decode_payload(&application,
            s64_publisher_workspace.bytes)) return 0u;
    *body = application.editor.buffer;
    document->body = body;
    document->type = application.document.type;
    document->status = application.document.status;
    document->order = application.document.order;
    memcpy(document->slug, application.document.slug, sizeof(document->slug));
    memcpy(document->title, application.document.title, sizeof(document->title));
    memcpy(document->date, application.document.date, sizeof(document->date));
    *dirty = application.editor.dirty;
    *replaced = application.document_replaced;
    return 1u;
}
