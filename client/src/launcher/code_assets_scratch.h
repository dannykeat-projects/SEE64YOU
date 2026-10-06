#ifndef SEE64YOU_LAUNCHER_CODE_ASSETS_SCRATCH_H
#define SEE64YOU_LAUNCHER_CODE_ASSETS_SCRATCH_H

#include "launcher/code_assets_dual.h"
#include "platform/basic_return.h"

/*
 * After s64_launcher_basic_return_prepare() succeeds, the original SYS context
 * is verified in REU and this 642-byte $C800 workspace is temporary again.
 * The preload-only diagnostic has no captured return context, so the same area
 * is free there as well. Keep dual-namespace preload scratch out of resident
 * BSS so the overwrite-safe launcher remains below $C800.
 */
#define S64_DUAL_ASSET_RECORD ((uint8_t*)S64_BASIC_RETURN_WORKSPACE)
#define S64_DUAL_ASSET_READBACK     (S64_DUAL_ASSET_RECORD + S64_CODE_ASSET_SESSION_SIZE)
#define S64_DUAL_EDITOR_ASSETS     ((S64CodeAssets*)(S64_DUAL_ASSET_READBACK + S64_CODE_ASSET_SESSION_SIZE))
#define S64_DUAL_PUBLISHER_ASSETS     ((S64CodeAssets*)((uint8_t*)S64_DUAL_EDITOR_ASSETS + sizeof(S64CodeAssets)))
#define S64_DUAL_CONTROL     ((S64ReuRuntimeControl*)((uint8_t*)S64_DUAL_PUBLISHER_ASSETS + sizeof(S64CodeAssets)))
#define S64_DUAL_SCRATCH_USED     (2u * S64_CODE_ASSET_SESSION_SIZE + 2u * sizeof(S64CodeAssets)         + sizeof(S64ReuRuntimeControl))

typedef char S64DualAssetScratchFits[
    S64_BASIC_RETURN_WORKSPACE_SIZE >= S64_DUAL_SCRATCH_USED ? 1 : -1
];

#endif
