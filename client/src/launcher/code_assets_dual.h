#ifndef SEE64YOU_LAUNCHER_CODE_ASSETS_DUAL_H
#define SEE64YOU_LAUNCHER_CODE_ASSETS_DUAL_H

#include "platform/code_assets.h"

uint8_t s64_dual_code_assets_count(S64ModuleId module);

uint8_t s64_dual_code_assets_parse(
    S64ModuleId module,
    S64CodeAssets* assets,
    const uint8_t* wire,
    uint16_t length,
    const S64ModuleDescriptor* descriptor
);

uint8_t s64_dual_code_assets_allocate(
    S64ModuleId module,
    S64CodeAssets* assets,
    S64ReuRuntimeControl* control
);

uint8_t s64_dual_code_assets_preload(
    S64ModuleId module,
    S64Dos* dos,
    const S64ReuRuntimeControl* control,
    const S64ModuleDescriptor* descriptor,
    S64CodeAssets* assets,
    S64SystemState* system
);

uint8_t s64_dual_code_assets_clear(
    S64ModuleId module,
    const S64ReuRuntimeControl* control
);

uint8_t s64_dual_code_assets_store_verified(
    S64ModuleId module,
    const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal,
    const S64ModuleDescriptor* descriptor,
    const S64CodeAssets* assets
);

#endif
