#ifndef SEE64YOU_CODE_ASSETS_H
#define SEE64YOU_CODE_ASSETS_H
#include "platform/code_overlay.h"
#include "platform/module_manifest.h"
#include "platform/session_journal.h"

#define S64_CODE_ASSET_FORMAT_VERSION 2u
#define S64_CODE_ASSET_MANIFEST_SIZE (28u + S64_CODE_OVERLAY_COUNT * 8u)
#define S64_CODE_ASSET_MANIFEST_CRC_OFFSET (S64_CODE_ASSET_MANIFEST_SIZE - 4u)
/* Each module image compiles the same codec against its own namespace.
 * Producer integration must use the matching module build. */
#ifdef S64_CODE_ASSET_PUBLISHER
#define S64_CODE_ASSET_MANIFEST_FILENAME "S64POVL.MNF"
#define S64_CODE_ASSET_MODULE S64_MODULE_PUBLISHER
#define S64_CODE_ASSET_MAGIC_LAST 0x50u
#define S64_CODE_ASSET_SESSION_MAGIC "S64Q"
#define S64_CODE_ASSET_SESSION_FLAG 5u
#define S64_CODE_ASSET_CACHE_FLAG 0x31u
#define S64_CODE_ASSET_FILE_0 "S64PCNT.BIN"
#define S64_CODE_ASSET_FILE_1 "S64PNET.BIN"
#define S64_CODE_ASSET_FILE_2 "S64PRUN.BIN"
#else
#define S64_CODE_ASSET_MANIFEST_FILENAME "S64OVL.MNF"
#define S64_CODE_ASSET_MODULE S64_MODULE_EDITOR
#define S64_CODE_ASSET_MAGIC_LAST 0x4fu
#define S64_CODE_ASSET_SESSION_MAGIC "S64C"
#define S64_CODE_ASSET_SESSION_FLAG 3u
#define S64_CODE_ASSET_CACHE_FLAG 0x21u
#define S64_CODE_ASSET_FILE_0 "S64MODEL.BIN"
#define S64_CODE_ASSET_FILE_1 "S64FILE.BIN"
#define S64_CODE_ASSET_FILE_2 "S64RUN.BIN"
#define S64_CODE_ASSET_FILE_3 "S64PREP.BIN"
#endif
#define S64_CODE_ASSET_SESSION_TRAILER (40u + S64_CODE_OVERLAY_COUNT * 16u)
#define S64_CODE_ASSET_SESSION_SIZE (S64_CODE_ASSET_SESSION_TRAILER + 8u)

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

/* The editor_* fields below contain the selected module root metadata.
 * Parsing binds raw assets to that exact module PRG metadata, not filenames
 * supplied by an external manifest. Outputs remain unchanged on failure. */
uint8_t s64_code_assets_parse(S64CodeAssets* assets, const uint8_t* wire,
    uint16_t length, const S64ModuleDescriptor* editor);
/* Reserve namespace-specific exact MODULE caches and one session record atomically. */
uint8_t s64_code_assets_allocate(S64CodeAssets* assets, S64ReuRuntimeControl* control);
/* Revoke published readiness before changing bytes. Requires verified editor
 * metadata and an owned IO_SAFE scope; all namespace CRCs must pass together. */
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
