#ifndef SEE64YOU_STANDARD_DISPLAY_H
#define SEE64YOU_STANDARD_DISPLAY_H

#include <stdint.h>

/* Publisher status text and its rows on the 40x25 grid. A server reply can
 * store 80 characters, which three 39-column rows carry in full; short
 * messages keep their own row and the rows behind them stay clear. No row
 * reads past the stored terminating NUL. */
void s64_publish_put(uint8_t row, const char* text, uint8_t maximum);
void s64_publish_message(const char* text);
void s64_publish_draw(uint8_t first_row);

#endif
