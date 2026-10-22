#include <assert.h>
#include <stdio.h>
#include "ui.h"

static const char* input;
static unsigned char consumed;

char cgetc(void)
{
    assert(input[consumed] != '\0');
    return input[consumed++];
}

static void check(const char* keys, unsigned char expected, unsigned char count)
{
    input = keys;
    consumed = 0;
    assert(s64_ui_prompt_exit() == expected);
    assert(consumed == count);
}

int main(void)
{
    check("s", 's', 1);
    check("S", 's', 1);
    check("d", 'd', 1);
    check("D", 'd', 1);
    check("\003", 3, 1);
    check("x\r S", 's', 4);
    check("?\003D", 3, 2);
    puts("exit prompt tests passed");
    return 0;
}
