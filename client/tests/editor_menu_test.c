#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "menu.h"

int main(void)
{
    unsigned char group;
    unsigned char item;

    assert(s64_menu_group_at(11u) == 0u);
    assert(s64_menu_group_at(16u) == 1u);
    assert(s64_menu_group_at(21u) == 2u);
    assert(s64_menu_group_at(28u) == 3u);
    assert(s64_menu_command(0u, 1u) == S64_INPUT_OPEN);
    assert(s64_menu_command(0u, 2u) == S64_INPUT_SAVE);
    assert(s64_menu_command(3u, 0u) == S64_INPUT_FORMAT_BOLD);
    assert(s64_menu_command(3u, 3u) == S64_INPUT_FORMAT_LINK);
    assert(s64_menu_item_at(0u, 10u, 1u) == 0u);
    assert(s64_menu_command(S64_MENU_GROUPS, 0u) == S64_INPUT_NONE);
    assert(s64_menu_item_count(S64_MENU_GROUPS) == 0u);
#ifdef S64_INDEPENDENT_EDITOR
    assert(S64_MENU_GROUPS == 4u);
    assert(s64_menu_group_at(35u) == S64_MENU_NONE);
    assert(strstr(S64_MENU_BAR, "PUB") == 0);
    for (group = 0u; group < S64_MENU_GROUPS; ++group) {
        for (item = 0u; item < s64_menu_item_count(group); ++item) {
            assert(s64_menu_command(group, item) != S64_INPUT_PUBLISH);
        }
    }
#else
    (void)group;
    (void)item;
    assert(S64_MENU_GROUPS == 5u);
    assert(s64_menu_group_at(35u) == 4u);
    assert(s64_menu_command(4u, 0u) == S64_INPUT_PUBLISH);
#endif
    puts("Editor menu commands and hit areas passed");
    return 0;
}
