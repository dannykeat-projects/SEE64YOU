#ifndef SEE64YOU_CODE_ASSETS_INTERNAL_H
#define SEE64YOU_CODE_ASSETS_INTERNAL_H
#include "platform/code_assets.h"
uint8_t s64_code_assets_editor_valid(const S64ModuleDescriptor* editor);
uint8_t s64_code_assets_valid(const S64CodeAssets* assets,
    const S64ReuRuntimeControl* control, const S64ModuleDescriptor* editor,
    uint8_t require_ready);
const S64ReuAllocation* s64_code_assets_session_allocation(const S64ReuRuntimeControl* control);
#endif
