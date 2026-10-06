#include <string.h>
#include "platform/managed_publisher.h"
#include "platform/application_state.h"
#include "publisher/workspace.h"

static S64ApplicationState application;
extern S64ModuleRuntime* s64_managed_publisher_document_owner;

static void copy_metadata(S64DocumentSummary* target, const S64Document* document)
{
    target->type = document->type;
    target->status = document->status;
    target->order = document->order;
    memcpy(target->slug, document->slug, sizeof(target->slug));
    memcpy(target->title, document->title, sizeof(target->title));
    memcpy(target->date, document->date, sizeof(target->date));
}

static uint8_t metadata_changed(const S64Document* document)
{
    return application.document.type != document->type
        || application.document.status != document->status
        || application.document.order != document->order
        || memcmp(application.document.slug, document->slug,
            sizeof(application.document.slug)) != 0
        || memcmp(application.document.title, document->title,
            sizeof(application.document.title)) != 0
        || memcmp(application.document.date, document->date,
            sizeof(application.document.date)) != 0;
}

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

uint8_t s64_managed_publisher_return_state(S64ModuleRuntime* runtime,
    const S64Buffer* body, const S64Document* document, uint8_t replaced,
    uint8_t* wire)
{
    uint8_t changed;
    uint8_t insert_mode;
    uint8_t word_wrap;
    uint32_t previous_generation;
    uint8_t previous_source;

    if (!runtime || !body || !document || !wire || replaced > 1u
        || document->body != body
        || !application.generation
        || application.generation == 0xffffffffUL
        || runtime->current_module != S64_MODULE_PUBLISHER
        || !runtime->lifecycle.active_record_written
        || runtime->lifecycle.clean_record_written
        || runtime->shared_invalidated
        || !runtime->system_state.captured
        || runtime->system_state.restored
        || runtime->system_state.mode != S64_SYSTEM_MODE_INTERACTIVE
        || !(runtime->reu_control.state_flags
            & S64_REU_RUNTIME_STATE_APPLICATION_VALID)) {
        return 0u;
    }

    changed = metadata_changed(document);
    if (s64_system_state_set_mode(&runtime->system_state,
            S64_SYSTEM_MODE_IO_SAFE) != S64_SYSTEM_OK) {
        return 2u;
    }

    /* Revoke the old authoritative snapshot before overwriting it. A verified
     * clear makes a failed rewrite safely retryable from this Publisher. */
    runtime->reu_control.state_flags &=
        (uint8_t)~S64_REU_RUNTIME_STATE_APPLICATION_VALID;
    if (!s64_reu_runtime_store_verified(&runtime->reu_control)) {
        return 2u;
    }

    previous_generation = application.generation;
    previous_source = application.source_module;
    if (replaced) {
        insert_mode = application.editor.insert_mode;
        word_wrap = application.editor.word_wrap;
        memset(&application.editor, 0, sizeof(application.editor));
        application.editor.buffer = *body;
        application.editor.buffer.cursor = 0u;
        application.editor.line = 1u;
        application.editor.column = 1u;
        application.editor.insert_mode = insert_mode;
        application.editor.word_wrap = word_wrap;
        application.editor.dirty = 1u;
        memset(application.filename, 0, sizeof(application.filename));
        application.document_replaced = 1u;
    } else if (changed) {
        application.editor.dirty = 1u;
    }

    copy_metadata(&application.document, document);
    application.source_module = S64_MODULE_PUBLISHER;
    ++application.generation;

    if (!s64_application_state_store(&runtime->reu_control,
            &runtime->lifecycle.journal, &application, wire)) {
        application.generation = previous_generation;
        application.source_module = previous_source;
        return s64_module_runtime_resume_interactive(runtime) == S64_SYSTEM_OK
            ? 0u : 2u;
    }

    runtime->reu_control.state_flags |= S64_REU_RUNTIME_STATE_APPLICATION_VALID;
    if (!s64_reu_runtime_store_verified(&runtime->reu_control)) {
        runtime->reu_control.state_flags &=
            (uint8_t)~S64_REU_RUNTIME_STATE_APPLICATION_VALID;
        application.generation = previous_generation;
        application.source_module = previous_source;
        if (!s64_reu_runtime_store_verified(&runtime->reu_control)) {
            return 2u;
        }
        return s64_module_runtime_resume_interactive(runtime) == S64_SYSTEM_OK
            ? 0u : 2u;
    }

    return s64_module_runtime_resume_interactive(runtime) == S64_SYSTEM_OK
        ? 1u : 2u;
}
