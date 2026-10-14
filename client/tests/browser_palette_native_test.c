/* Exercise the actual browser callback under cc65's native character mapping. */
#define main unused_browser_main
#include "../src/browser/main.c"
#undef main
#include <assert.h>
#include <stdio.h>

static const uint8_t header[] = {
    0x53,0x45,0x45,0x36,0x34,0x59,0x4f,0x55,0x2d,
    0x42,0x52,0x4f,0x57,0x53,0x45,0x2f,0x32,0x0a,0x43,0x09
};
static const uint8_t hex[] = {
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
    0x38,0x39,0x41,0x42,0x43,0x44,0x45,0x46
};
static const uint8_t body[] = {0x54,0x09,0x54,0x65,0x73,0x74,0x0a};
static uint8_t wire[24];

int main(void)
{
    uint8_t slot, value, chunk, key;
    uint16_t offset, amount, cases = 0u;
    assert(s64_reu_probe());
    s64_reu_runtime_init(&control, 1u, 1UL);
    assert(s64_reu_runtime_allocate(&control, S64_REU_OWNER_PREVIEW, 65536UL, 256u, &row_store));
    assert(s64_reu_runtime_allocate(&control, S64_REU_OWNER_PREVIEW, 65536UL, 256u, &link_store));
    memcpy(wire, header, sizeof(header));
    wire[23] = 0x0au;
    for (chunk = 1u; chunk <= 24u; chunk += 23u) {
        for (slot = 0u; slot < 3u; ++slot) {
            for (value = 0u; value < 16u; ++value) {
                wire[20] = wire[21] = wire[22] = 0x30u;
                wire[20u + slot] = hex[value];
                s64_browser_parser_init(&parser, record, 0);
                for (offset = 0u; offset < sizeof(wire); offset += amount) {
                    amount = chunk;
                    if (amount > sizeof(wire) - offset) amount = sizeof(wire) - offset;
                    assert(s64_browser_parser_feed(&parser, wire + offset, amount) == S64_BROWSER_PROTOCOL_OK);
                }
                assert(s64_browser_parser_finish(&parser) == S64_BROWSER_PROTOCOL_OK);
                assert(next_background == (slot == 0u ? value : 0u));
                assert(next_text == (slot == 1u ? value : 0u));
                assert(next_link == (slot == 2u ? value : 0u));
                ++cases;
            }
        }
    }
    assert(cases == 96u);
    wire[20] = 0x30u; wire[21] = 0x31u; wire[22] = 0x44u;
    s64_browser_parser_init(&parser, record, 0);
    assert(s64_browser_parser_feed(&parser, wire, sizeof(wire)) == S64_BROWSER_PROTOCOL_OK);
    assert(s64_browser_parser_feed(&parser, body, sizeof(body)) == S64_BROWSER_PROTOCOL_OK);
    assert(s64_browser_parser_finish(&parser) == S64_BROWSER_PROTOCOL_OK);
    assert(rows == 1u && next_background == 0u && next_text == 1u && next_link == 13u);
    wire[22] = 0x47u;
    s64_browser_parser_init(&parser, record, 0);
    assert(s64_browser_parser_feed(&parser, wire, sizeof(wire)) == S64_BROWSER_PROTOCOL_INVALID);
    wire[22] = 0x61u;
    s64_browser_parser_init(&parser, record, 0);
    assert(s64_browser_parser_feed(&parser, wire, sizeof(wire)) == S64_BROWSER_PROTOCOL_INVALID);
    clrscr(); puts("Native palette tests passed"); puts("X exit");
    for (;;) {
        key = (uint8_t)cgetc();
        if (key == 'x') break;
        if (key == 'r') {
            textcolor(COLOR_GREEN);
            assert(unused_browser_main() == 0);
            assert(*(volatile uint8_t*)0x0286u == COLOR_GREEN);
            clrscr(); puts("Native colour restore passed"); puts("X exit");
        }
    }
    return 0;
}
