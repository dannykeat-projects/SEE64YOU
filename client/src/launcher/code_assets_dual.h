#ifndef SEE64YOU_LAUNCHER_CODE_ASSETS_DUAL_H
#define SEE64YOU_LAUNCHER_CODE_ASSETS_DUAL_H

#include "platform/code_assets.h"

typedef struct {
    S64ModuleId module;
    uint8_t magic_last;
    uint8_t session_flag;
    uint8_t cache_flag;
    uint8_t overlay_count;
    const char* manifest_filename;
    const char* session_magic;
    const char* filenames[S64_CODE_OVERLAY_MAX_COUNT];
} S64LauncherCodeAssetNamespace;

const S64LauncherCodeAssetNamespace* s64_dual_code_assets_namespace(
    S64ModuleId module
);

uint8_t s64_dual_code_assets_parse(
    const S64LauncherCodeAssetNamespace* namespace,
    S64CodeAssets* assets,
    const uint8_t* wire,
    uint16_t length,
    const S64ModuleDescriptor* module
);

uint8_t s64_dual_code_assets_allocate(
    const S64LauncherCodeAssetNamespace* namespace,
    S64CodeAssets* assets,
    S64ReuRuntimeControl* control
);

uint8_t s64_dual_code_assets_preload(
    const S64LauncherCodeAssetNamespace* namespace,
    S64Dos* dos,
    const S64ReuRuntimeControl* control,
    const S64ModuleDescriptor* module,
    S64CodeAssets* assets,
    S64SystemState* system
);

uint8_t s64_dual_code_assets_clear(
    const S64LauncherCodeAssetNamespace* namespace,
    const S64ReuRuntimeControl* control
);

uint8_t s64_dual_code_assets_store_verified(
    const S64LauncherCodeAssetNamespace* namespace,
    const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal,
    const S64ModuleDescriptor* module,
    const S64CodeAssets* assets
);

#endif
