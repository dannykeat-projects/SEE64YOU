#include "publisher/persistent_settings_render.h"

#include <conio.h>

#define S64_SETTINGS_SCREEN_COLUMNS 40u
#define S64_SETTINGS_STATUS_ROW 24u

static void put_line(
    uint8_t row,
    const char* label,
    const char* value)
{
    uint8_t column = 0u;

    gotoxy(0u, row);
    if (label) {
        while (*label && column < S64_SETTINGS_SCREEN_COLUMNS) {
            cputc((uint8_t)*label++);
            ++column;
        }
    }
    if (value) {
        while (*value && column < S64_SETTINGS_SCREEN_COLUMNS) {
            cputc((uint8_t)*value++);
            ++column;
        }
    }
}

void s64_persistent_settings_render_screen(
    const S64PersistentSettingsScreen* screen)
{
    uint8_t index;

    if (!screen || !screen->title
        || screen->line_count > S64_SETTINGS_SCREEN_MAX_LINES) {
        return;
    }

    clrscr();
    revers(1u);
    put_line(0u, screen->title, 0);
    revers(0u);

    for (index = 0u; index < screen->line_count; ++index) {
        put_line(
            screen->lines[index].row,
            screen->lines[index].label,
            screen->lines[index].value
        );
    }

    revers(1u);
    put_line(S64_SETTINGS_STATUS_ROW, screen->status, 0);
    revers(0u);
}
