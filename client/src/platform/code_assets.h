#ifndef SEE64YOU_CODE_ASSETS_H
#define SEE64YOU_CODE_ASSETS_H
#include "platform/code_overlay.h"
#include "platform/module_manifest.h"
#include "platform/session_journal.h"

#define S64_CODE_ASSET_MANIFEST_SIZE 52u
#define S64_CODE_ASSET_MANIFEST_FILENAME "S64OVL.MNF"
#define S64_CODE_ASSET_SESSION_SIZE 96u
#define S64_CODE_ASSET_SESSION_FLAG 3u
#define S64_CODE_ASSET_CACHE_FLAG 0x21u

typedef struct {
    S64ReuAllocation cache;
    uint32_t integrity;
    uint8_t cache_state;
} S64CodeAsset;
typedef struct {
    uint16_t abi_version;
    uint16_t app_version;
    uint32_t editor_length;
    uint32_t editor_integrity;
    S64CodeAsset assets[S64_CODE_OVERLAY_COUNT];
} S64CodeAssets;

/* Parsing binds raw assets to the exact editor PRG metadata, not filenames
 * supplied by an external manifest. Outputs remain unchanged on failure. */
uint8_t s64_code_assets_parse(S64CodeAssets* assets, const uint8_t* wire,
    uint16_t length, const S64ModuleDescriptor* editor);
/* Reserve three exact MODULE caches and one session record atomically. */
uint8_t s64_code_assets_allocate(S64CodeAssets* assets, S64ReuRuntimeControl* control);
/* Revoke published readiness before changing bytes. Requires verified editor
 * metadata and an owned IO_SAFE scope; all three CRCs must pass together. */
uint8_t s64_code_assets_preload(S64Dos* dos, const S64ReuRuntimeControl* control,
    const S64ModuleDescriptor* editor, S64CodeAssets* assets, S64SystemState* system);
/* Session-record DMA APIs below require an exclusively owned IO_SAFE scope. */
uint8_t s64_code_assets_clear(const S64ReuRuntimeControl* control);
uint8_t s64_code_assets_store_verified(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const S64ModuleDescriptor* editor,
    const S64CodeAssets* assets);
/* Trusted ACTIVE journal/editor only. No SD access or fresh state capture. */
uint8_t s64_code_assets_load(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const S64ModuleDescriptor* editor,
    S64CodeAssets* assets);
/* Consume only a trusted arrival boundary. Failure revokes any old selector
 * configuration; persistent ACTIVE bytes alone never authorize binding. */
uint8_t s64_code_assets_bind(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const S64ModuleDescriptor* editor,
    uint8_t trusted_arrival, void* slot, uint16_t capacity, S64CodeOverlayReader reader);
#endif
