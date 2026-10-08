#include "publisher/persistent_settings_render.h"

#include <conio.h>

#define S64_SETTINGS_STATUS_ROW 24u

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
        cputsxy(0u, screen->lines[index].row, screen->lines[index].text);
    }

    revers(1u);
    if (screen->status) cputsxy(0u, S64_SETTINGS_STATUS_ROW, screen->status);
    revers(0u);
}
