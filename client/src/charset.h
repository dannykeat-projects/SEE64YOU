#ifndef SEE64YOU_CHARSET_H
#define SEE64YOU_CHARSET_H

#include <stdint.h>

/*
 * Convert the 7-bit source-text subset used by See64You between ASCII on
 * disk/network boundaries and the C64's native PETSCII representation.
 * Returns 0 for bytes outside the supported source-text subset.
 */
uint8_t s64_charset_ascii_to_petscii(uint8_t ascii, uint8_t* petscii);
uint8_t s64_charset_petscii_to_ascii(uint8_t petscii, uint8_t* ascii);

/* Convert between the platform-native text representation and wire ASCII. */
uint8_t s64_charset_native_to_ascii(uint8_t native, uint8_t* ascii);
uint8_t s64_charset_ascii_to_native(uint8_t ascii, uint8_t* native);

#endif
