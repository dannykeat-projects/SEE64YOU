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
    S64ModuleId module;
    uint8_t magic_last;
    uint8_t session_flag;
    uint8_t cache_flag;
    uint8_t overlay_count;
    const char* manifest_filename;
    const char* session_magic;
    const char* filenames[S64_CODE_OVERLAY_MAX_COUNT];
} S64CodeAssetNamespace;

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

/* Non-dual module builds expose only their own namespace. Launcher builds set
 * S64_CODE_ASSET_DUAL_NAMESPACE and can resolve both Editor and Publisher. */
const S64CodeAssetNamespace* s64_code_assets_namespace(S64ModuleId module);

/* Namespace-aware producer APIs let one launcher implementation prepare both
 * asset sets without linking two copies of the codec. The namespace count must
 * fit the S64CodeAssets array compiled into the caller. */
uint8_t s64_code_assets_parse_for(const S64CodeAssetNamespace* namespace,
    S64CodeAssets* assets, const uint8_t* wire, uint16_t length,
    const S64ModuleDescriptor* editor);
uint8_t s64_code_assets_allocate_for(const S64CodeAssetNamespace* namespace,
    S64CodeAssets* assets, S64ReuRuntimeControl* control);
uint8_t s64_code_assets_preload_for(const S64CodeAssetNamespace* namespace,
    S64Dos* dos, const S64ReuRuntimeControl* control,
    const S64ModuleDescriptor* editor, S64CodeAssets* assets,
    S64SystemState* system);
uint8_t s64_code_assets_clear_for(const S64CodeAssetNamespace* namespace,
    const S64ReuRuntimeControl* control);
uint8_t s64_code_assets_store_verified_for(
    const S64CodeAssetNamespace* namespace,
    const S64ReuRuntimeControl* control, const S64SessionJournal* journal,
    const S64ModuleDescriptor* editor, const S64CodeAssets* assets);

/* Existing destination/default-namespace API. */
uint8_t s64_code_assets_parse(S64CodeAssets* assets, const uint8_t* wire,
    uint16_t length, const S64ModuleDescriptor* editor);
uint8_t s64_code_assets_allocate(S64CodeAssets* assets, S64ReuRuntimeControl* control);
uint8_t s64_code_assets_preload(S64Dos* dos, const S64ReuRuntimeControl* control,
    const S64ModuleDescriptor* editor, S64CodeAssets* assets, S64SystemState* system);
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
