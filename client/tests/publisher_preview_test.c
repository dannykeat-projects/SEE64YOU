#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "publisher/preview.h"

static char cells[S64_PREVIEW_ROWS][S64_PREVIEW_COLUMNS];
static uint8_t styles[S64_PREVIEW_ROWS][S64_PREVIEW_COLUMNS];

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void emit(
    uint8_t row,
    uint8_t column,
    uint8_t value,
    uint8_t style,
    void* context
)
{
    (void)context;
    cells[row][column] = (char)value;
    styles[row][column] = style;
}

static void insert_text(S64Buffer* buffer, const char* text)
{
    while (*text != '\0') {
        require(s64_buffer_insert(buffer, (unsigned char)*text++), "insert fixture");
    }
}

static void require_row(uint8_t row, const char* expected, const char* message)
{
    size_t length = strlen(expected);
    require(memcmp(cells[row], expected, length) == 0, message);
    while (length < S64_PREVIEW_COLUMNS) {
        require(cells[row][length] == ' ', message);
        ++length;
    }
}

int main(void)
{
    static const char source[] =
        "# Heading\n"
        "Normal **bold** and *italic*\n"
        "- [OpenAI](https://openai.com)\n"
        "### literal\n"
        "**unterminated";
    S64Buffer buffer;
    S64PreviewState state;
    uint8_t at_end;
    uint8_t i;

    memset(cells, ' ', sizeof(cells));
    memset(styles, 0, sizeof(styles));

    s64_buffer_init(&buffer);
    insert_text(&buffer, source);
    s64_preview_init(&state);

    require(
        s64_preview_render_page(&buffer, &state, emit, 0, &at_end),
        "render preview page"
    );
    require(at_end, "fixture fits one page");
    require_row(0u, "Heading", "heading markers removed");
    require_row(1u, "Normal bold and italic", "inline markers removed");
    require_row(2u, "- OpenAI", "link target hidden");
    require_row(3u, "### literal", "unsupported heading remains literal");
    require_row(4u, "**unterminated", "unterminated marker remains literal");

    for (i = 0u; i < 7u; ++i) {
        require(
            styles[0][i] == S64_PREVIEW_STYLE_REVERSE,
            "heading is visually distinguished"
        );
    }
    require(styles[1][7] == S64_PREVIEW_STYLE_REVERSE, "bold is reversed");
    require(styles[1][8] == S64_PREVIEW_STYLE_REVERSE, "bold span continues");
    require(styles[1][16] == S64_PREVIEW_STYLE_NORMAL, "italic remains normal");
    require(styles[2][2] == S64_PREVIEW_STYLE_REVERSE, "link label is reversed");

    puts("publisher preview tests passed");
    return 0;
}
