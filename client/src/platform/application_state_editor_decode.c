#include "platform/application_state_internal.h"
#include <stddef.h>
static uint16_t get16(const uint8_t* p)
{ return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
void s64_application_state_decode_editor(S64Editor* e, const uint8_t* w)
{
    uint8_t i;
    static const uint8_t words[] = {
        offsetof(S64Editor, buffer.gap_start), offsetof(S64Editor, buffer.gap_end),
        offsetof(S64Editor, buffer.cursor), offsetof(S64Editor, view_start),
        offsetof(S64Editor, view_column), offsetof(S64Editor, line),
        offsetof(S64Editor, column), offsetof(S64Editor, position_cursor),
        offsetof(S64Editor, selection_anchor), offsetof(S64Editor, preferred_column)
    };
    static const uint8_t bytes[] = {
        offsetof(S64Editor, history.start), offsetof(S64Editor, history.count),
        offsetof(S64Editor, history.cursor), offsetof(S64Editor, history.pending_valid),
        offsetof(S64Editor, history.saved_valid), offsetof(S64Editor, history.saved_cursor),
        offsetof(S64Editor, view_cursor_row), offsetof(S64Editor, view_cursor_column),
        offsetof(S64Editor, selection_valid), offsetof(S64Editor, preferred_column_valid),
        offsetof(S64Editor, mark_mode), offsetof(S64Editor, dirty),
        offsetof(S64Editor, insert_mode), offsetof(S64Editor, word_wrap)
    };
    /* Offsets describe this compiler's destination, never the wire layout.
     * Assign unsigned int explicitly because host editor fields are wider. */
    for (i = 0; i < sizeof(words); ++i)
        *(unsigned int*)((uint8_t*)e + words[i]) = get16(w + (i < 3 ? 40 : 46) + 2u * i);
    for (i = 0; i < sizeof(bytes); ++i)
        *((uint8_t*)e + bytes[i]) = w[(i < 6 ? 46 : 60) + i];
}
