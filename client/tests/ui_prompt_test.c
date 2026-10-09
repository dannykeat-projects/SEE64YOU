#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ui.h"

static const unsigned char* input;
static unsigned int consumed;

char cgetc(void) { assert(input[consumed]); return (char)input[consumed++]; }
void cputs(const char* text) { (void)text; }
void cputc(char value) { (void)value; }
void gotoxy(unsigned char x, unsigned char y) { (void)x; (void)y; }
unsigned char revers(unsigned char enabled) { return enabled; }

static void check(const char* keys, unsigned char capacity,
                  unsigned char result, const char* expected)
{
    char value[33];
    memset(value, 0x7f, sizeof(value));
    input = (const unsigned char*)keys;
    consumed = 0;
    assert(s64_ui_prompt_text("File: ", value, capacity) == result);
    assert(strcmp(value, expected) == 0);
    assert(consumed == strlen(keys));
    assert((unsigned char)value[capacity] == 0x7f);
}

int main(void)
{
    check("\xc1\xda" "AZ09.S64\r", 32, 1, "\xc1\xda" "AZ09.S64");
    check("\001\200\300\333" "ok\r", 32, 1, "ok");
    check("ab\024c\r", 32, 1, "ac");
    check("\024\r", 32, 1, "");
    check("abcdef\r", 4, 1, "abc");
    check("abc\003", 32, 0, "");
    check("\r", 32, 1, "");
    puts("Editor native text prompt tests passed");
    return 0;
}
