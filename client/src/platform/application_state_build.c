#include "platform/application_state_internal.h"
#include "platform/reu.h"
#include "platform/crc32.h"
#include <limits.h>
#include <string.h>

uint8_t s64_application_state_store(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const S64ApplicationState* state, uint8_t* wire)
{
    const S64ReuAllocation* item = s64_application_state_allocation(control);
    uint8_t readback[32];
    uint16_t offset;
    uint32_t address;
    if (!item || !s64_application_state_encode(control, journal, state, wire)
        || !s64_reu_store((uint8_t)(item->address >> 16), (uint16_t)item->address,
            wire, S64_APPLICATION_STATE_SIZE)) return 0;
    for (offset = 0; offset < S64_APPLICATION_STATE_SIZE; offset += sizeof(readback)) {
        address = item->address + offset;
        if (!s64_reu_load((uint8_t)(address >> 16), (uint16_t)address, readback, sizeof(readback))
            || memcmp(wire + offset, readback, sizeof(readback))) return 0;
    }
    return 1;
}

static void put16(uint8_t* p, uint16_t v)
{ p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void put32(uint8_t* p, uint32_t v)
{ put16(p, (uint16_t)v); put16(p + 2, (uint16_t)(v >> 16)); }

uint8_t s64_application_state_encode(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const S64ApplicationState* s, uint8_t* w)
{
    const S64Editor* e;
    if (!s || !w) return 0;
    e = &s->editor;
    if (s->document.type < S64_DOCUMENT_PAGE || s->document.type > S64_DOCUMENT_POST
        || s->document.status < S64_DOCUMENT_DRAFT || s->document.status > S64_DOCUMENT_PUBLISHED)
        return 0;
#if UINT_MAX > 65535u
    /* Host model positions must not silently narrow into valid cc65 positions. */
    if (e->buffer.gap_start > 65535u || e->buffer.gap_end > 65535u
        || e->buffer.cursor > 65535u || e->view_start > 65535u || e->view_column > 65535u
        || e->line > 65535u || e->column > 65535u || e->position_cursor > 65535u
        || e->selection_anchor > 65535u || e->preferred_column > 65535u) return 0;
#endif
    memset(w, 0, S64_APPLICATION_STATE_SIZE);
    if (!s64_application_state_active(control, journal, w + 8)) return 0;
    /* Explicit ASCII bytes also work under cc65's PETSCII string mapping. */
    w[0] = 0x53; w[1] = 0x36; w[2] = 0x34; w[3] = 0x41;
    w[4] = 1; w[5] = S64_REU_RUNTIME_ABI_VERSION; put16(w + 6, S64_APPLICATION_STATE_SIZE);
    put32(w + 32, s->generation); w[36] = s->source_module; w[37] = s->document_replaced;
    put16(w + 40, e->buffer.gap_start); put16(w + 42, e->buffer.gap_end);
    put16(w + 44, e->buffer.cursor);
    w[46] = e->history.start; w[47] = e->history.count; w[48] = e->history.cursor;
    w[49] = e->history.pending_valid; w[50] = e->history.saved_valid; w[51] = e->history.saved_cursor;
    put16(w + 52, e->view_start); put16(w + 54, e->view_column);
    put16(w + 56, e->line); put16(w + 58, e->column);
    put16(w + 60, e->position_cursor); put16(w + 62, e->selection_anchor);
    put16(w + 64, e->preferred_column);
    w[66] = e->view_cursor_row; w[67] = e->view_cursor_column;
    w[68] = e->selection_valid; w[69] = e->preferred_column_valid; w[70] = e->mark_mode;
    w[71] = e->dirty; w[72] = e->insert_mode; w[73] = e->word_wrap;
    put16(w + 74, s->clipboard_length);
    w[76] = (uint8_t)s->document.type; w[77] = (uint8_t)s->document.status;
    put32(w + 78, (uint32_t)s->document.order);
    memcpy(w + 82, s->document.slug, S64_DOCUMENT_SLUG_CAPACITY);
    memcpy(w + 147, s->document.title, S64_DOCUMENT_TITLE_CAPACITY);
    memcpy(w + 228, s->document.date, S64_DOCUMENT_DATE_CAPACITY);
    memcpy(w + 269, s->filename, S64_LOCAL_FILENAME_CAPACITY);
    memcpy(w + 303, s->search, S64_APPLICATION_SEARCH_CAPACITY); w[336] = s->search_valid;
    if (!s64_application_state_payload_valid(w)) return 0;
    put32(w + 508, s64_crc32_update(0xffffffffUL, w, 508) ^ 0xffffffffUL);
    return 1;
}
