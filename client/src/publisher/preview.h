#ifndef SEE64YOU_PUBLISHER_PREVIEW_H
#define SEE64YOU_PUBLISHER_PREVIEW_H

#include <stdint.h>

#include "buffer.h"

#define S64_PREVIEW_COLUMNS 40u
#define S64_PREVIEW_ROWS 20u

#define S64_PREVIEW_STYLE_NORMAL 0u
#define S64_PREVIEW_STYLE_REVERSE 0x01u

typedef struct {
    unsigned int source_offset;
    unsigned int marker_close;
    unsigned int link_label_end;
    unsigned int link_after;
    uint8_t marker_length;
    uint8_t marker_reverse;
    uint8_t link_active;
    uint8_t at_line_start;
    uint8_t line_reverse;
} S64PreviewState;

typedef void (*S64PreviewEmitFn)(
    uint8_t row,
    uint8_t column,
    uint8_t value,
    uint8_t style,
    void* context
);

void s64_preview_init(S64PreviewState* state);

int s64_preview_render_page(
    const S64Buffer* buffer,
    S64PreviewState* state,
    S64PreviewEmitFn emit,
    void* context,
    uint8_t* at_end
);

#endif
