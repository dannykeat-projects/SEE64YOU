#include "platform/application_state_internal.h"
#include <string.h>

static uint16_t get16(const uint8_t* p)
{ return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
static uint32_t get32(const uint8_t* p)
{ return (uint32_t)get16(p) | ((uint32_t)get16(p + 2) << 16); }
/* Wire ranges are checked before touching output: host unsigned int is wider
 * than cc65's, and neither compiler's struct layout is a wire contract. */
uint8_t s64_application_state_payload_valid(const uint8_t* w)
{
    uint16_t start = get16(w + 40), end = get16(w + 42);
    uint16_t length;
    uint16_t i;
    if (start > end || end > S64_BUFFER_CAPACITY) return 0;
    length = S64_BUFFER_CAPACITY - (end - start);
    if (!get32(w + 32) || w[36] < S64_MODULE_EDITOR || w[36] > S64_MODULE_SETTINGS
        || w[37] > 1 || w[38] || w[39]
        || get16(w + 44) > length
        || w[46] >= S64_HISTORY_LEVELS || w[47] > S64_HISTORY_LEVELS
        || w[48] > w[47] || w[49] /* pending operations never cross a transition */
        || w[50] > 1 || w[51] > S64_HISTORY_LEVELS
        || (w[50] && w[51] > w[47])
        || get16(w + 52) > length || get16(w + 54) > S64_BUFFER_CAPACITY
        || get16(w + 56) == 0 || get16(w + 56) > S64_BUFFER_CAPACITY + 1u
        || get16(w + 58) == 0 || get16(w + 58) > S64_BUFFER_CAPACITY + 1u
        || get16(w + 60) > S64_BUFFER_CAPACITY + 1u
        || get16(w + 62) > length || get16(w + 64) > S64_BUFFER_CAPACITY
        || w[66] >= 23 || w[67] >= 40
        || get16(w + 74) > S64_CLIPBOARD_CAPACITY
        || w[76] < S64_DOCUMENT_PAGE || w[76] > S64_DOCUMENT_POST
        || w[77] < S64_DOCUMENT_DRAFT || w[77] > S64_DOCUMENT_PUBLISHED
        || !memchr(w + 82, 0, S64_DOCUMENT_SLUG_CAPACITY)
        || !memchr(w + 147, 0, S64_DOCUMENT_TITLE_CAPACITY)
        || !memchr(w + 228, 0, S64_DOCUMENT_DATE_CAPACITY)
        || !memchr(w + 269, 0, S64_LOCAL_FILENAME_CAPACITY)
        || !memchr(w + 303, 0, S64_APPLICATION_SEARCH_CAPACITY)
        || w[336] > 1 || (w[336] && !w[303])) return 0;
    for (i = 68; i < 74; ++i) if (w[i] > 1) return 0;
    for (i = 337; i < 508; ++i) if (w[i]) return 0;
    return 1;
}

uint8_t s64_application_state_decode_payload(S64ApplicationState* s, const uint8_t* w)
{
    S64Editor* e;
    if (!s || !w || !s64_application_state_payload_valid(w)) return 0;
    memset(s, 0, sizeof(*s));
    e = &s->editor;
    s->generation = get32(w + 32); s->source_module = w[36]; s->document_replaced = w[37];
    e->buffer.gap_start = get16(w + 40); e->buffer.gap_end = get16(w + 42);
    e->buffer.cursor = get16(w + 44);
    e->history.start = w[46]; e->history.count = w[47]; e->history.cursor = w[48];
    e->history.pending_valid = w[49]; e->history.saved_valid = w[50]; e->history.saved_cursor = w[51];
    e->view_start = get16(w + 52); e->view_column = get16(w + 54);
    e->line = get16(w + 56); e->column = get16(w + 58);
    e->position_cursor = get16(w + 60); e->selection_anchor = get16(w + 62);
    e->preferred_column = get16(w + 64);
    e->view_cursor_row = w[66]; e->view_cursor_column = w[67];
    e->selection_valid = w[68]; e->preferred_column_valid = w[69]; e->mark_mode = w[70];
    e->dirty = w[71]; e->insert_mode = w[72]; e->word_wrap = w[73];
    s->clipboard_length = get16(w + 74);
    s->document.type = (S64DocumentType)w[76]; s->document.status = (S64DocumentStatus)w[77];
    s->document.order = (int32_t)get32(w + 78);
    memcpy(s->document.slug, w + 82, S64_DOCUMENT_SLUG_CAPACITY);
    memcpy(s->document.title, w + 147, S64_DOCUMENT_TITLE_CAPACITY);
    memcpy(s->document.date, w + 228, S64_DOCUMENT_DATE_CAPACITY);
    memcpy(s->filename, w + 269, S64_LOCAL_FILENAME_CAPACITY);
    memcpy(s->search, w + 303, S64_APPLICATION_SEARCH_CAPACITY); s->search_valid = w[336];
    return 1;
}
