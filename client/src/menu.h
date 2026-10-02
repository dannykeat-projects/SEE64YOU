#ifndef SEE64YOU_MENU_H
#define SEE64YOU_MENU_H

#include "input/input.h"

#define S64_MENU_NONE 0xffu
#define S64_MENU_GROUPS 5u

unsigned char s64_menu_group_at(unsigned char column);
unsigned char s64_menu_item_at(unsigned char group, unsigned char column, unsigned char row);
unsigned char s64_menu_item_count(unsigned char group);
S64InputType s64_menu_command(unsigned char group, unsigned char item);
void s64_menu_render(unsigned char group, unsigned char item);

#endif
