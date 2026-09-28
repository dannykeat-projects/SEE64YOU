#include "publisher/preview.h"

#include <string.h>

#define S64_PREVIEW_CACHE_SIZE 64u

static uint8_t cache[S64_PREVIEW_CACHE_SIZE];
static unsigned int cache_start;
static unsigned int cache_count;
static const S64Buffer* cache_buffer;

static uint8_t read_char(
    const S64Buffer* buffer,
    unsigned int position,
    uint8_t* value
)
{
    unsigned int length;

    if (buffer == 0 || value == 0) return 0u;
    length = s64_buffer_length(buffer);
    if (position >= length) return 0u;

    if (cache_buffer != buffer
        || position < cache_start
        || position >= cache_start + cache_count) {
        cache_start = position;
        cache_count = s64_buffer_read(
            buffer,
            position,
            cache,
            S64_PREVIEW_CACHE_SIZE
        );
        cache_buffer = buffer;
        if (cache_count == 0u) return 0u;
    }

    *value = cache[position - cache_start];
    return 1u;
}

static uint8_t matches(
    const S64Buffer* buffer,
    unsigned int position,
    const char* text,
    uint8_t length,
    unsigned int limit
)
{
    uint8_t i;
    uint8_t value;

    if ((unsigned long)position + length > limit) return 0u;
    for (i = 0u; i < length; ++i) {
        if (!read_char(buffer, position + i, &value)
            || value != (uint8_t)text[i]) {
            return 0u;
        }
    }
    return 1u;
}

static unsigned int line_end(
    const S64Buffer* buffer,
    unsigned int start
)
{
    unsigned int length = s64_buffer_length(buffer);
    uint8_t value;

    while (start < length) {
        if (!read_char(buffer, start, &value) || value == '\n' || value == '\r') {
            break;
        }
        ++start;
    }
    return start;
}

static unsigned int find_marker(
    const S64Buffer* buffer,
    unsigned int start,
    unsigned int end,
    const char* marker,
    uint8_t marker_length
)
{
    while ((unsigned long)start + marker_length <= end) {
        if (matches(buffer, start, marker, marker_length, end)) return start;
        ++start;
    }
    return end;
}

static uint8_t link_target_valid(
    const S64Buffer* buffer,
    unsigned int start,
    unsigned int end
)
{
    unsigned int position;
    uint8_t value;

    if (matches(buffer, start, "http://", 7u, end)) {
        position = start + 7u;
    } else if (matches(buffer, start, "https://", 8u, end)) {
        position = start + 8u;
    } else {
        return 0u;
    }
    if (position >= end) return 0u;

    while (position < end) {
        if (!read_char(buffer, position, &value)
            || value <= 0x20u || value > 0x7eu || value == ')') {
            return 0u;
        }
        ++position;
    }
    return 1u;
}

static uint8_t begin_link(
    const S64Buffer* buffer,
    S64PreviewState* state,
    unsigned int end
)
{
    unsigned int label_end;
    unsigned int close;
    uint8_t value;

    if (!read_char(buffer, state->source_offset, &value) || value != '[') {
        return 0u;
    }

    label_end = state->source_offset + 1u;
    while (label_end < end) {
        if (!read_char(buffer, label_end, &value)) return 0u;
        if (value == ']') break;
        ++label_end;
    }
    if (label_end == state->source_offset + 1u
        || label_end + 2u >= end
        || !matches(buffer, label_end, "](", 2u, end)) {
        return 0u;
    }

    close = label_end + 2u;
    while (close < end) {
        if (!read_char(buffer, close, &value)) return 0u;
        if (value == ')') break;
        ++close;
    }
    if (close >= end
        || !link_target_valid(buffer, label_end + 2u, close)) {
        return 0u;
    }

    state->link_active = 1u;
    state->link_label_end = label_end;
    state->link_after = close + 1u;
    ++state->source_offset;
    return 1u;
}

static uint8_t begin_marker(
    const S64Buffer* buffer,
    S64PreviewState* state,
    unsigned int end
)
{
    unsigned int close;
    uint8_t length;
    const char* marker;

    if (matches(buffer, state->source_offset, "***", 3u, end)) {
        marker = "***";
        length = 3u;
        state->marker_reverse = 1u;
    } else if (matches(buffer, state->source_offset, "**", 2u, end)) {
        marker = "**";
        length = 2u;
        state->marker_reverse = 1u;
    } else if (matches(buffer, state->source_offset, "*", 1u, end)) {
        marker = "*";
        length = 1u;
        state->marker_reverse = 0u;
    } else {
        return 0u;
    }

    close = find_marker(
        buffer,
        state->source_offset + length,
        end,
        marker,
        length
    );
    if (close >= end) return 0u;

    state->marker_close = close;
    state->marker_length = length;
    state->source_offset += length;
    return 1u;
}

void s64_preview_init(S64PreviewState* state)
{
    if (state == 0) return;

    memset(state, 0, sizeof(*state));
    state->at_line_start = 1u;
}

int s64_preview_render_page(
    const S64Buffer* buffer,
    S64PreviewState* state,
    S64PreviewEmitFn emit,
    void* context,
    uint8_t* at_end
)
{
    unsigned int length;
    unsigned int end;
    uint8_t row = 0u;
    uint8_t column = 0u;
    uint8_t value;
    uint8_t style;

    if (at_end != 0) *at_end = 0u;
    if (buffer == 0 || state == 0 || emit == 0) return 0;

    length = s64_buffer_length(buffer);
    if (state->source_offset > length) return 0;

    cache_buffer = 0;
    cache_count = 0u;

    while (row < S64_PREVIEW_ROWS && state->source_offset < length) {
        end = line_end(buffer, state->source_offset);

        if (state->at_line_start) {
            state->line_reverse = 0u;
            if (matches(buffer, state->source_offset, "## ", 3u, end)) {
                state->source_offset += 3u;
                state->line_reverse = 1u;
            } else if (matches(buffer, state->source_offset, "# ", 2u, end)) {
                state->source_offset += 2u;
                state->line_reverse = 1u;
            }
            state->at_line_start = 0u;
            end = line_end(buffer, state->source_offset);
        }

        if (state->source_offset >= end) {
            if (state->source_offset < length
                && read_char(buffer, state->source_offset, &value)
                && (value == '\n' || value == '\r')) {
                ++state->source_offset;
                if (value == '\r'
                    && state->source_offset < length
                    && read_char(buffer, state->source_offset, &value)
                    && value == '\n') {
                    ++state->source_offset;
                }
            }
            state->at_line_start = 1u;
            state->line_reverse = 0u;
            state->marker_length = 0u;
            state->link_active = 0u;
            ++row;
            column = 0u;
            continue;
        }

        if (state->link_active
            && state->source_offset == state->link_label_end) {
            state->source_offset = state->link_after;
            state->link_active = 0u;
            continue;
        }

        if (state->marker_length != 0u
            && state->source_offset == state->marker_close) {
            state->source_offset += state->marker_length;
            state->marker_length = 0u;
            state->marker_reverse = 0u;
            continue;
        }

        if (!state->link_active
            && state->marker_length == 0u
            && begin_link(buffer, state, end)) {
            continue;
        }

        if (state->marker_length == 0u
            && begin_marker(buffer, state, end)) {
            continue;
        }

        if (!read_char(buffer, state->source_offset, &value)) return 0;
        style = (state->line_reverse
                || state->marker_reverse
                || state->link_active)
            ? S64_PREVIEW_STYLE_REVERSE
            : S64_PREVIEW_STYLE_NORMAL;
        emit(row, column, value, style, context);
        ++state->source_offset;
        ++column;

        if (column >= S64_PREVIEW_COLUMNS) {
            column = 0u;
            ++row;
        }
    }

    if (state->source_offset >= length && at_end != 0) {
        *at_end = 1u;
    }
    return 1;
}
