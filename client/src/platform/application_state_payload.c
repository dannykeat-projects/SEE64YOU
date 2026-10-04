#include "platform/application_state_internal.h"
#include <string.h>
#include <stddef.h>

#ifdef __CC65__
/* Small scalar readers and range tables remain callable/readable while either
 * content overlay is selected. This preserves the fixed execution-slot limit. */
#pragma code-name (push, "CODE")
#pragma rodata-name (push, "RODATA")
#endif
static uint16_t get16(const uint8_t* p)
{ return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
static uint32_t get32(const uint8_t* p)
{ return (uint32_t)get16(p) | ((uint32_t)get16(p + 2) << 16); }
#ifdef __CC65__
#pragma code-name (pop)
#endif
/* Wire ranges are checked before touching output: host unsigned int is wider
 * than cc65's, and neither compiler's struct layout is a wire contract. */
uint8_t s64_application_state_payload_valid(const uint8_t* w)
{
    uint16_t start = get16(w + 40), end = get16(w + 42);
    uint16_t length;
    uint16_t i;
    static const uint8_t positions[] = {44, 52, 62};
    static const uint8_t bounded[] = {54, 56, 58, 60, 64, 74};
    static const uint16_t minima[] = {0, 1, 1, 0, 0, 0};
    static const uint16_t maxima[] = {S64_BUFFER_CAPACITY,
        S64_BUFFER_CAPACITY + 1u, S64_BUFFER_CAPACITY + 1u,
        S64_BUFFER_CAPACITY + 1u, S64_BUFFER_CAPACITY, S64_CLIPBOARD_CAPACITY};
    static const uint16_t byte_offsets[] = {36, 37, 46, 47, 50, 51, 66, 67, 76, 77, 336};
    static const uint8_t byte_minima[] = {S64_MODULE_EDITOR, 0, 0, 0, 0, 0, 0, 0,
        S64_DOCUMENT_PAGE, S64_DOCUMENT_DRAFT, 0};
    static const uint8_t byte_maxima[] = {S64_MODULE_SETTINGS, 1, S64_HISTORY_LEVELS - 1,
        S64_HISTORY_LEVELS, 1, S64_HISTORY_LEVELS, 22, 39,
        S64_DOCUMENT_POST, S64_DOCUMENT_PUBLISHED, 1};
    static const uint16_t strings[] = {82, 147, 228, 269, 303};
    static const uint8_t capacities[] = {S64_DOCUMENT_SLUG_CAPACITY, S64_DOCUMENT_TITLE_CAPACITY,
        S64_DOCUMENT_DATE_CAPACITY, S64_LOCAL_FILENAME_CAPACITY, S64_APPLICATION_SEARCH_CAPACITY};
    if (start > end || end > S64_BUFFER_CAPACITY) return 0;
    length = S64_BUFFER_CAPACITY - (end - start);
    for (i = 0; i < sizeof(positions); ++i)
        if (get16(w + positions[i]) > length) return 0;
    for (i = 0; i < sizeof(bounded); ++i) {
        uint16_t value = get16(w + bounded[i]);
        if (value < minima[i] || value > maxima[i]) return 0;
    }
    for (i = 0; i < sizeof(byte_minima); ++i) {
        uint8_t value = w[byte_offsets[i]];
        if (value < byte_minima[i] || value > byte_maxima[i]) return 0;
    }
    for (i = 0; i < sizeof(capacities); ++i)
        if (!memchr(w + strings[i], 0, capacities[i])) return 0;
    if (!get32(w + 32) || w[38] || w[39]
        || w[48] > w[47] || w[49] /* pending operations never cross a transition */
        || (w[50] && w[51] > w[47])
        || (w[336] && !w[303])) return 0;
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
    s64_application_state_decode_editor(e, w);
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
#ifdef __CC65__
#pragma rodata-name (pop)
#endif
