#ifndef SEE64YOU_MANAGED_EDITOR_SETTINGS_H
#define SEE64YOU_MANAGED_EDITOR_SETTINGS_H

#include <stdint.h>
#include "platform/module_runtime.h"

/* Prepare canonical persistent publishing settings for one managed Publisher
 * transition. Must run in the Editor PUBLICATION/PREP overlay while ACTIVE and
 * IO_SAFE. No Settings-helper execution and no allocator entry are required. */
uint8_t s64_managed_editor_settings_prepare(
    S64ModuleRuntime* runtime,
    uint8_t* scratch
);

#endif
