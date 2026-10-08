#include "publisher/persistent_settings_render.h"

#include <conio.h>

#define S64_SETTINGS_SCREEN_COLUMNS 40u
#define S64_SETTINGS_STATUS_ROW 24u

static void clear_row(uint8_t row)
{
    uint8_t column;

    gotoxy(0u, row);
    for (column = 0u; column < S64_SETTINGS_SCREEN_COLUMNS; ++column) {
        cputc(' ');
    }
}

static void put_bounded(const char* text, uint8_t* column)
{
    if (!text) return;
    while (*text && *column < S64_SETTINGS_SCREEN_COLUMNS) {
        cputc((uint8_t)*text++);
        ++(*column);
    }
}

void s64_persistent_settings_render_screen(
    const S64PersistentSettingsScreen* screen)
{
    uint8_t index;
    uint8_t column;

    if (!screen || !screen->title
        || screen->line_count > S64_SETTINGS_SCREEN_MAX_LINES) {
        return;
    }

    clrscr();
    revers(1u);
    gotoxy(0u, 0u);
    cputs(screen->title);
    revers(0u);

    for (index = 0u; index < screen->line_count; ++index) {
        clear_row(screen->lines[index].row);
        gotoxy(0u, screen->lines[index].row);
        column = 0u;
        put_bounded(screen->lines[index].label, &column);
        put_bounded(screen->lines[index].value, &column);
    }

    clear_row(S64_SETTINGS_STATUS_ROW);
    gotoxy(0u, S64_SETTINGS_STATUS_ROW);
    revers(1u);
    column = 0u;
    put_bounded(screen->status, &column);
    revers(0u);
}
