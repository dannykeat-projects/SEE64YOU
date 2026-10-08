#include "publisher/persistent_settings_render.h"

#include <conio.h>
#include <string.h>

#define S64_SETTINGS_SCREEN_COLUMNS 40u
#define S64_SETTINGS_STATUS_ROW 24u

static void put_value(uint8_t row, const char* label, const char* value)
{
    uint8_t column;

    cputsxy(0u, row, label);
    if (!value) return;

    column = (uint8_t)strlen(label);
    gotoxy(column, row);
    while (*value && column < S64_SETTINGS_SCREEN_COLUMNS) {
        cputc((uint8_t)*value++);
        ++column;
    }
}

void s64_persistent_settings_render_screen(
    const S64PersistentSettingsScreen* screen)
{
    uint8_t index;

    if (!screen || !screen->title) return;

    clrscr();
    revers(1u);
    cputsxy(0u, 0u, screen->title);
    revers(0u);

    for (index = 0u; index < screen->line_count; ++index) {
        put_value(
            screen->lines[index].row,
            screen->lines[index].label,
            screen->lines[index].value
        );
    }

    revers(1u);
    if (screen->status) cputsxy(0u, S64_SETTINGS_STATUS_ROW, screen->status);
    revers(0u);
}
