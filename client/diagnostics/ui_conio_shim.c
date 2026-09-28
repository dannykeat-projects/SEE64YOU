#include <conio.h>
#include <stdint.h>

#define SCREEN_COLUMNS 40u
#define SCREEN_ROWS 25u
#define SCREEN_RAM ((volatile uint8_t*)0x0400u)
#define COLOR_RAM ((volatile uint8_t*)0xD800u)
#define BORDER_REG (*(volatile uint8_t*)0xD020u)
#define BG_REG (*(volatile uint8_t*)0xD021u)

static uint8_t shim_x;
static uint8_t shim_y;
static uint8_t shim_reverse;
static uint8_t shim_text_color = COLOR_LIGHTBLUE;
static uint8_t shim_cursor;

static uint8_t screen_code(uint8_t value)
{
    if (value >= 0x80u) {
        value &= 0x7fu;
        if (value == 0x7fu) {
            return 0x5eu;
        }
        return (uint8_t)(value | 0x40u);
    }

    if (value >= 0x60u) {
        value &= 0xdfu;
    }
    if (value >= 0x20u) {
        value &= 0x3fu;
    }
    return value;
}

unsigned char __fastcall__ textcolor(unsigned char color)
{
    unsigned char old = shim_text_color;
    shim_text_color = color;
    return old;
}

unsigned char __fastcall__ bgcolor(unsigned char color)
{
    unsigned char old = BG_REG;
    BG_REG = color;
    return old;
}

unsigned char __fastcall__ bordercolor(unsigned char color)
{
    unsigned char old = BORDER_REG;
    BORDER_REG = color;
    return old;
}

unsigned char __fastcall__ cursor(unsigned char onoff)
{
    unsigned char old = shim_cursor;
    shim_cursor = onoff ? 1u : 0u;
    return old;
}

unsigned char __fastcall__ revers(unsigned char onoff)
{
    unsigned char old = shim_reverse;
    shim_reverse = onoff ? 0x80u : 0u;
    return old ? 1u : 0u;
}

void __fastcall__ gotoxy(unsigned char x, unsigned char y)
{
    shim_x = x;
    shim_y = y;
}

void clrscr(void)
{
    unsigned int i;

    for (i = 0u; i < SCREEN_COLUMNS * SCREEN_ROWS; ++i) {
        SCREEN_RAM[i] = 0x20u;
        COLOR_RAM[i] = shim_text_color;
    }
    shim_x = 0u;
    shim_y = 0u;
    shim_reverse = 0u;
}

void __fastcall__ cputc(char c)
{
    uint8_t value = (uint8_t)c;
    unsigned int offset;

    if (value == '\n') {
        shim_x = 0u;
        if (shim_y + 1u < SCREEN_ROWS) {
            ++shim_y;
        }
        return;
    }
    if (value == '\r') {
        if (shim_y + 1u < SCREEN_ROWS) {
            ++shim_y;
        }
        return;
    }

    if (shim_x >= SCREEN_COLUMNS || shim_y >= SCREEN_ROWS) {
        return;
    }

    offset = (unsigned int)shim_y * SCREEN_COLUMNS + shim_x;
    SCREEN_RAM[offset] = (uint8_t)(screen_code(value) | shim_reverse);
    COLOR_RAM[offset] = shim_text_color;

    ++shim_x;
    if (shim_x >= SCREEN_COLUMNS) {
        shim_x = 0u;
        if (shim_y + 1u < SCREEN_ROWS) {
            ++shim_y;
        }
    }
}

void __fastcall__ cputs(const char* text)
{
    while (*text != '\0') {
        cputc(*text++);
    }
}
