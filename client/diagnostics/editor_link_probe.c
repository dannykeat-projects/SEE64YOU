#include <stdint.h>
#include <string.h>
#include "editor.h"
#define RESULT (*(volatile uint8_t*)0xc061)
#define DETAIL (*(volatile uint8_t*)0xc062)
#define CHECK(test,n) do { if(!(test)) { DETAIL=n; RESULT=0x5a; for(;;){} } } while(0)
static S64Editor editor;
static int text_is(const char* text)
{
    unsigned int i;
    if(s64_buffer_length(&editor.buffer)!=strlen(text)) return 0;
    for(i=0u;text[i];++i)
        if(s64_buffer_char_at(&editor.buffer,i)!=(unsigned char)text[i]) return 0;
    return 1;
}
int main(void)
{
    RESULT=DETAIL=0u;
    s64_editor_init(&editor);
    CHECK(s64_editor_format_link(&editor,"Example","https://example.com/index.8tml"),1);
    CHECK(text_is("[Example](https://example.com/index.8tml)"),2);
    CHECK(!s64_editor_format_link(&editor,"Bad","https://example.com/a\\b"),3);
    CHECK(s64_buffer_move_cursor(&editor.buffer,2u),4);
    CHECK(s64_editor_link_at_cursor(&editor),5);
    CHECK(s64_editor_format_link(&editor,0,"HTTP://example.com/next.8tml"),6);
    CHECK(text_is("[Example](HTTP://example.com/next.8tml)"),7);
    CHECK(!s64_editor_format_link(&editor,"Bad","file://example"),8);
    CHECK(s64_editor_undo(&editor),9);
    CHECK(text_is("[Example](https://example.com/index.8tml)"),10);
    RESULT=0xa5; for(;;){} return 0;
}
