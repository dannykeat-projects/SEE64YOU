#ifndef SEE64YOU_CODE_ASSETS_INTERNAL_H
#define SEE64YOU_CODE_ASSETS_INTERNAL_H
#include "platform/code_assets.h"

uint8_t s64_code_assets_editor_valid_for(const S64CodeAssetNamespace* namespace,
    const S64ModuleDescriptor* editor);
uint8_t s64_code_assets_valid_for(const S64CodeAssetNamespace* namespace,
    const S64CodeAssets* assets, const S64ReuRuntimeControl* control,
    const S64ModuleDescriptor* editor, uint8_t require_ready);
uint8_t s64_code_assets_editor_valid(const S64ModuleDescriptor* editor);
uint8_t s64_code_assets_valid(const S64CodeAssets* assets,
    const S64ReuRuntimeControl* control, const S64ModuleDescriptor* editor,
    uint8_t require_ready);

const S64ReuAllocation* s64_code_assets_session_allocation_for(
    const S64CodeAssetNamespace* namespace,
    const S64ReuRuntimeControl* control);
const S64ReuAllocation* s64_code_assets_session_allocation(
    const S64ReuRuntimeControl* control);

/* Shared non-reentrant session-record scratch/helpers for producer and consumer. */
extern uint8_t s64_code_assets_session_wire[S64_CODE_ASSET_SESSION_SIZE];
uint8_t s64_code_assets_session_verify_for(
    const S64CodeAssetNamespace* namespace,
    const S64ReuAllocation* allocation);
uint8_t s64_code_assets_session_verify(const S64ReuAllocation* allocation);
extern uint8_t s64_code_assets_session_journal_wire[S64_SESSION_JOURNAL_WIRE_SIZE];
uint8_t s64_code_assets_active_session(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal);
uint16_t s64_code_assets_session_checksum_for(
    const S64CodeAssetNamespace* namespace, const uint8_t* bytes);
uint16_t s64_code_assets_session_checksum(const uint8_t* bytes);
#endif
