#include <conio.h>
#include <string.h>
#include "publisher/standard_display.h"

/* Whole server reply: 80 characters plus the terminator. Rows below split
 * that text into the 39 visible columns of the 40-column grid. */
static char status[81];

/* Rewriting the whole buffer keeps short messages clear of anything an older,
 * longer message left behind, so the screen never shows stale letters. */
void s64_publish_message(const char* text)
{
    memset(status, 0, sizeof(status));
    strncpy(status, text, sizeof(status) - 1u);
    status[sizeof(status) - 1u] = 0;
}

/* One row of the grid, at most `maximum` characters, stopping at the text
 * NUL. This is the shared drawing step: the menu rows and the status rows
 * below go through it. */
void s64_publish_put(uint8_t row, const char* text, uint8_t maximum)
{
    gotoxy(0u, row);
    while (*text && maximum--) cputc(*text++);
}

/* Three fixed 39-character windows: every offset stays inside the stored
 * reply, and a short message stops at its own NUL, so no adjacent memory
 * reaches the screen and the rows after the text stay clear. */
void s64_publish_draw(uint8_t first_row)
{
    s64_publish_put(first_row, status, 39u);
    s64_publish_put((uint8_t)(first_row + 1u), status + 39u, 39u);
    s64_publish_put((uint8_t)(first_row + 2u), status + 78u, 39u);
}
