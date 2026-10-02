#include <conio.h>
#include <string.h>

#include "menu.h"

#define S64_MENU_WIDTH 20u

typedef struct {
    const char* label;
    S64InputType command;
} S64MenuItem;

typedef struct {
    const char* title;
    unsigned char header_col;
    unsigned char popup_col;
    const S64MenuItem* items;
    unsigned char count;
} S64MenuGroup;

static const S64MenuItem file_items[] = {
    { "New Ctrl+N", S64_INPUT_NEW },
    { "Open Ctrl+O", S64_INPUT_OPEN },
    { "Save Ctrl+S", S64_INPUT_SAVE },
    { "Save As", S64_INPUT_SAVE_AS },
    { "Exit RUN/STOP", S64_INPUT_EXIT }
};

static const S64MenuItem edit_items[] = {
    { "Undo Ctrl+Z", S64_INPUT_UNDO },
    { "Redo Ctrl+Shift+Z", S64_INPUT_REDO },
    { "Select All Ctrl+A", S64_INPUT_SELECT_ALL },
    { "Copy F3", S64_INPUT_COPY },
    { "Cut F5", S64_INPUT_CUT },
    { "Paste F7", S64_INPUT_PASTE },
    { "Mark F1", S64_INPUT_TOGGLE_MARK }
};

static const S64MenuItem search_items[] = {
    { "Find Ctrl+F", S64_INPUT_FIND },
    { "Find Next F2", S64_INPUT_FIND_NEXT },
    { "Find Previous F4", S64_INPUT_FIND_PREVIOUS },
    { "Go To Line Ctrl+G", S64_INPUT_GOTO_LINE },
    { "Replace Ctrl+H", S64_INPUT_REPLACE }
};

static const S64MenuItem format_items[] = {
    { "Bold Ctrl+B", S64_INPUT_FORMAT_BOLD },
    { "Italic Ctrl+I", S64_INPUT_FORMAT_ITALIC },
    { "List Ctrl+L", S64_INPUT_FORMAT_LIST },
    { "Word Wrap Ctrl+W", S64_INPUT_TOGGLE_WRAP }
};

static const S64MenuItem pub_items[] = {
    { "Publisher Ctrl+P", S64_INPUT_PUBLISH }
};

static const S64MenuGroup groups[] = {
    { "FILE",   11u, 10u, file_items,   sizeof(file_items) / sizeof(file_items[0]) },
    { "EDIT",   16u, 15u, edit_items,   sizeof(edit_items) / sizeof(edit_items[0]) },
    { "SEARCH", 21u, 20u, search_items, sizeof(search_items) / sizeof(search_items[0]) },
    { "FORMAT", 28u, 20u, format_items, sizeof(format_items) / sizeof(format_items[0]) },
    { "PUB",    35u, 20u, pub_items,    sizeof(pub_items) / sizeof(pub_items[0]) }
};

unsigned char s64_menu_group_at(unsigned char column)
{
    unsigned char group;

    for (group = 0u; group < S64_MENU_GROUPS; ++group) {
        unsigned char length = (unsigned char)strlen(groups[group].title);
        if (column >= groups[group].header_col
            && column < (unsigned char)(groups[group].header_col + length)) {
            return group;
        }
    }
    return S64_MENU_NONE;
}

unsigned char s64_menu_item_count(unsigned char group)
{
    return group < S64_MENU_GROUPS ? groups[group].count : 0u;
}

unsigned char s64_menu_item_at(
    unsigned char group,
    unsigned char column,
    unsigned char row
)
{
    unsigned char item;

    if (group >= S64_MENU_GROUPS || row == 0u
        || column < groups[group].popup_col
        || column >= (unsigned char)(groups[group].popup_col + S64_MENU_WIDTH)) {
        return S64_MENU_NONE;
    }
    item = (unsigned char)(row - 1u);
    return item < groups[group].count ? item : S64_MENU_NONE;
}

S64InputType s64_menu_command(unsigned char group, unsigned char item)
{
    if (group >= S64_MENU_GROUPS || item >= groups[group].count) {
        return S64_INPUT_NONE;
    }
    return groups[group].items[item].command;
}

void s64_menu_render(unsigned char group, unsigned char item)
{
    unsigned char row;
    unsigned char col;
    unsigned char length;
    const S64MenuGroup* menu;
    const char* label;

    if (group >= S64_MENU_GROUPS) {
        return;
    }

    menu = &groups[group];

    revers(0);
    gotoxy(menu->header_col, 0u);
    cputs(menu->title);
    revers(1);

    for (row = 0u; row < menu->count; ++row) {
        gotoxy(menu->popup_col, (unsigned char)(1u + row));
        revers(row == item);
        label = menu->items[row].label;
        length = (unsigned char)strlen(label);
        cputs(label);
        for (col = length; col < S64_MENU_WIDTH; ++col) {
            cputc(' ');
        }
    }
    revers(0);
}
