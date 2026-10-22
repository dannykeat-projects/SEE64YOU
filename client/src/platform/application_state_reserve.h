#ifndef SEE64YOU_PLATFORM_APPLICATION_STATE_RESERVE_H
#define SEE64YOU_PLATFORM_APPLICATION_STATE_RESERVE_H

#include "platform/reu_runtime.h"

#define S64_APPLICATION_STATE_SIZE 512u
#define S64_APPLICATION_STATE_FLAG 6u

/* Allocator-only contract; safe for Launcher code that does not use the
 * REU-backed Editor model. */
uint8_t s64_application_state_reserve(S64ReuRuntimeControl* control);

#endif
