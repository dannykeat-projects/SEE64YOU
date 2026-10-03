#ifndef SEE64YOU_MODULE_INTEGRITY_H
#define SEE64YOU_MODULE_INTEGRITY_H
#include "platform/system_state.h"
/* Caller owns IO_SAFE. A mode failure still requires owner cleanup/restore. */
uint8_t s64_reu_crc32_matches(uint32_t address, uint32_t length,
    uint32_t expected, S64SystemState* system);
#endif
