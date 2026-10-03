#ifndef SEE64YOU_CRC32_H
#define SEE64YOU_CRC32_H
#include <stdint.h>
/* Supply 0xffffffff initially; XOR the final accumulator with 0xffffffff. */
uint32_t s64_crc32_update(uint32_t crc, const uint8_t* bytes, uint16_t length);
#endif
