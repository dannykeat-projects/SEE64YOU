#ifndef SEE64YOU_PUBLISHER_LOCAL_PICKER_H
#define SEE64YOU_PUBLISHER_LOCAL_PICKER_H
#include <stdint.h>
#include "platform/dos.h"

/* 0 cancelled, 1 selected, 2 request manual filename, 3 DOS/error.
   DOS current directory must already be the configured package folder. */
uint8_t s64_local_pick(S64Dos* dos, char* filename, uint8_t capacity);
#endif
