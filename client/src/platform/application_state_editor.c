#include "platform/application_state.h"
#include <string.h>

static void metadata(S64ApplicationState* state, const S64Document* document)
{
    state->document.type = document->type; state->document.status = document->status;
    state->document.order = document->order;
    memcpy(state->document.slug, document->slug, sizeof(state->document.slug));
    memcpy(state->document.title, document->title, sizeof(state->document.title));
    memcpy(state->document.date, document->date, sizeof(state->document.date));
}

uint8_t s64_application_state_capture(S64ApplicationState* state, S64Editor* editor,
    const S64Document* document, const S64LocalDocument* local,
    const S64Clipboard* clipboard, const char* search, uint8_t search_valid,
    uint32_t generation)
{
    if (!state || !editor || !document || !local || !clipboard || !search
        || !generation || search_valid > 1 || clipboard->length > S64_CLIPBOARD_CAPACITY
        || !memchr(search, 0, S64_APPLICATION_SEARCH_CAPACITY)
        || !s64_history_prepare_handoff(&editor->history)) return 0;
    memset(state, 0, sizeof(*state));
    state->editor = *editor; metadata(state, document);
    memcpy(state->filename, local->filename, sizeof(state->filename));
    state->clipboard_length = clipboard->length;
    memcpy(state->search, search, sizeof(state->search)); state->search_valid = search_valid;
    state->source_module = S64_MODULE_EDITOR; state->generation = generation;
    return 1;
}

uint8_t s64_application_state_replace(S64ApplicationState* state,
    const S64Buffer* body, const S64Document* document)
{
    uint8_t insert_mode, word_wrap;
    if (!state || !body || !document || !state->generation || state->generation == 0xffffffffUL
        || body->gap_start > body->gap_end || body->gap_end > S64_BUFFER_CAPACITY) return 0;
    insert_mode = state->editor.insert_mode; word_wrap = state->editor.word_wrap;
    memset(&state->editor, 0, sizeof(state->editor));
    state->editor.buffer = *body; state->editor.buffer.cursor = 0;
    state->editor.line = 1; state->editor.column = 1;
    state->editor.insert_mode = insert_mode; state->editor.word_wrap = word_wrap;
    state->editor.dirty = 1;
    /* History control is reset without touching clipboard or its REU bytes. */
    memset(state->filename, 0, sizeof(state->filename));
    metadata(state, document);
    state->document_replaced = 1; state->source_module = S64_MODULE_PUBLISHER;
    ++state->generation;
    return 1;
}
