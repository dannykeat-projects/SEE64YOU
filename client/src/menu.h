#ifndef SEE64YOU_MENU_H
#define SEE64YOU_MENU_H

#include "input/input.h"

#define S64_MENU_NONE 0xffu
#ifdef S64_INDEPENDENT_EDITOR
#define S64_MENU_GROUPS 4u
#define S64_MENU_BAR " SEE64YOU  FILE EDIT SEARCH FORMAT     "
#else
#define S64_MENU_GROUPS 5u
#define S64_MENU_BAR " SEE64YOU  FILE EDIT SEARCH FORMAT PUB "
#endif

unsigned char s64_menu_group_at(unsigned char column);
unsigned char s64_menu_item_at(unsigned char group, unsigned char column, unsigned char row);
unsigned char s64_menu_item_count(unsigned char group);
S64InputType s64_menu_command(unsigned char group, unsigned char item);
void s64_menu_render(unsigned char group, unsigned char item);

#endif
