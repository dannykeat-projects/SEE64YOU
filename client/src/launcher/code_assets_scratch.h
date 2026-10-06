#ifndef SEE64YOU_LAUNCHER_CODE_ASSETS_SCRATCH_H
#define SEE64YOU_LAUNCHER_CODE_ASSETS_SCRATCH_H

#include "launcher/code_assets_dual.h"

/*
 * Scratch reuses the existing 642-byte $C800-$CA81 BASIC-return workspace.
 * Production and resume-gate callers publish the captured SYS context to REU
 * before asset preparation; the preload-only diagnostic has no capture there.
 *
 * Slots are intentionally generous and fixed so cc65 need not evaluate pointer
 * arithmetic involving sizeof() in constant expressions:
 *   $C800 112-byte manifest/session record
 *   $C870 112-byte readback
 *   $C8E0  96-byte Editor asset descriptor
 *   $C940  96-byte Publisher asset descriptor
 *   $C9A0 192-byte temporary runtime-control plan
 * Highest byte used is $CA5F, below the workspace end $CA81.
 */
#define S64_DUAL_ASSET_RECORD ((uint8_t*)0xc800u)
#define S64_DUAL_ASSET_READBACK ((uint8_t*)0xc870u)
#define S64_DUAL_EDITOR_ASSETS ((S64CodeAssets*)0xc8e0u)
#define S64_DUAL_PUBLISHER_ASSETS ((S64CodeAssets*)0xc940u)
#define S64_DUAL_CONTROL ((S64ReuRuntimeControl*)0xc9a0u)

#endif
