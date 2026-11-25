#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ui.h"

static const char* input;
static unsigned char consumed;

char cgetc(void)
{
    assert(input[consumed] != '\0');
    return input[consumed++];
}

void gotoxy(unsigned char col, unsigned char row) { assert(col == 0u && row == 24u); }
void cputs(const char* prompt) { assert(strlen(prompt) <= 39u); }

static void check_recovery(const char* keys, unsigned char expected)
{
    input = keys;
    consumed = 0;
    assert(s64_ui_recovery_choice() == expected);
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
    check_recovery("Y", 'y');
    check_recovery("d", 'd');
    check_recovery("N", 3u);
    check_recovery("?\003", 3u);
    puts("exit and recovery prompt tests passed");
    return 0;
}
