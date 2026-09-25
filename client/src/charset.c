#include "charset.h"

uint8_t s64_charset_ascii_to_petscii(uint8_t ascii, uint8_t* petscii)
{
    uint8_t value;

    if (petscii == 0) return 0u;

    if (ascii == 0x00u || ascii == 0x09u) {
        value = ascii;
    } else if (ascii == 0x0Au) {
        value = 0x0Du;
    } else if (ascii == 0x0Du) {
        value = 0x0Au;
    } else if (ascii >= 0x20u && ascii <= 0x40u) {
        value = ascii;
    } else if (ascii >= 0x41u && ascii <= 0x5Au) {
        value = (uint8_t)(ascii + 0x80u);
    } else if (ascii == 0x5Bu || ascii == 0x5Du || ascii == 0x5Eu) {
        value = ascii;
    } else if (ascii == 0x5Cu) {
        value = 0xBFu;
    } else if (ascii == 0x5Fu) {
        value = 0xA4u;
    } else if (ascii == 0x60u) {
        value = 0xADu;
    } else if (ascii >= 0x61u && ascii <= 0x7Au) {
        value = (uint8_t)(ascii - 0x20u);
    } else if (ascii == 0x7Bu) {
        value = 0xB3u;
    } else if (ascii == 0x7Cu) {
        value = 0xDDu;
    } else if (ascii == 0x7Du) {
        value = 0xABu;
    } else if (ascii == 0x7Eu) {
        value = 0xB1u;
    } else {
        return 0u;
    }

    *petscii = value;
    return 1u;
}

uint8_t s64_charset_petscii_to_ascii(uint8_t petscii, uint8_t* ascii)
{
    uint8_t value;

    if (ascii == 0) return 0u;

    if (petscii == 0x00u || petscii == 0x09u) {
        value = petscii;
    } else if (petscii == 0x0Du) {
        value = 0x0Au;
    } else if (petscii == 0x0Au) {
        value = 0x0Du;
    } else if (petscii >= 0x20u && petscii <= 0x40u) {
        value = petscii;
    } else if (petscii >= 0x41u && petscii <= 0x5Au) {
        value = (uint8_t)(petscii + 0x20u);
    } else if (petscii == 0x5Bu || petscii == 0x5Du || petscii == 0x5Eu) {
        value = petscii;
    } else if (petscii >= 0xC1u && petscii <= 0xDAu) {
        value = (uint8_t)(petscii - 0x80u);
    } else if (petscii == 0xBFu) {
        value = 0x5Cu;
    } else if (petscii == 0xA4u) {
        value = 0x5Fu;
    } else if (petscii == 0xADu) {
        value = 0x60u;
    } else if (petscii == 0xB3u) {
        value = 0x7Bu;
    } else if (petscii == 0xDDu) {
        value = 0x7Cu;
    } else if (petscii == 0xABu) {
        value = 0x7Du;
    } else if (petscii == 0xB1u) {
        value = 0x7Eu;
    } else {
        return 0u;
    }

    *ascii = value;
    return 1u;
}

uint8_t s64_charset_native_to_ascii(uint8_t native, uint8_t* ascii)
{
#ifdef S64_NATIVE_PETSCII
    return s64_charset_petscii_to_ascii(native, ascii);
#else
    uint8_t unused;

    if (ascii == 0 || !s64_charset_ascii_to_petscii(native, &unused)) return 0u;
    *ascii = native;
    return 1u;
#endif
}

uint8_t s64_charset_ascii_to_native(uint8_t ascii, uint8_t* native)
{
#ifdef S64_NATIVE_PETSCII
    return s64_charset_ascii_to_petscii(ascii, native);
#else
    uint8_t unused;

    if (native == 0 || !s64_charset_ascii_to_petscii(ascii, &unused)) return 0u;
    *native = ascii;
    return 1u;
#endif
}
