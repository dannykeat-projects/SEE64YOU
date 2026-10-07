#ifndef SEE64YOU_CODE_ASSETS_INTERNAL_H
#define SEE64YOU_CODE_ASSETS_INTERNAL_H
#include "platform/code_assets.h"
uint8_t s64_code_assets_editor_valid(const S64ModuleDescriptor* editor);
uint8_t s64_code_assets_valid(const S64CodeAssets* assets,
    const S64ReuRuntimeControl* control, const S64ModuleDescriptor* editor,
    uint8_t require_ready);
const S64ReuAllocation* s64_code_assets_session_allocation(const S64ReuRuntimeControl* control);

/* Shared non-reentrant session-record scratch/helpers for producer and consumer. */
#ifdef S64_PERSISTENT_RUNTIME_FIT
#include "platform/persistent_fit_scratch.h"
#define s64_code_assets_session_wire \
    (*(uint8_t (*)[S64_CODE_ASSET_SESSION_SIZE])S64_PERSISTENT_FIT_SCRATCH_BASE)
#define s64_code_assets_session_journal_wire \
    (*(uint8_t (*)[S64_SESSION_JOURNAL_WIRE_SIZE]) \
        (S64_PERSISTENT_FIT_SCRATCH_BASE + S64_CODE_ASSET_SESSION_SIZE))
typedef char S64PersistentFitAssetScratchFits[
    S64_CODE_ASSET_SESSION_SIZE + S64_SESSION_JOURNAL_WIRE_SIZE
        <= S64_PERSISTENT_FIT_SCRATCH_CAPACITY ? 1 : -1
];
#else
extern uint8_t s64_code_assets_session_wire[S64_CODE_ASSET_SESSION_SIZE];
extern uint8_t s64_code_assets_session_journal_wire[S64_SESSION_JOURNAL_WIRE_SIZE];
#endif
uint8_t s64_code_assets_session_verify(const S64ReuAllocation* allocation);
uint8_t s64_code_assets_active_session(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal);
uint16_t s64_code_assets_session_checksum(const uint8_t* bytes);
#endif
