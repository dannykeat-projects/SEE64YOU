#include "platform/code_assets_internal.h"
#include "platform/reu.h"
#include <string.h>

uint8_t s64_code_assets_session_wire[S64_CODE_ASSET_SESSION_SIZE];
uint8_t s64_code_assets_session_readback[S64_CODE_ASSET_SESSION_SIZE];
uint8_t s64_code_assets_session_journal_wire[S64_SESSION_JOURNAL_WIRE_SIZE];
uint16_t s64_code_assets_session_checksum(const uint8_t* bytes)
{
    uint16_t value = 0x64a5u;
    uint8_t i;
    for (i = 0; i < S64_CODE_ASSET_SESSION_SIZE - 2; ++i) {
        value = (uint16_t)((value << 5) | (value >> 11));
        value ^= bytes[i];
    }
    return value ? value : 1;
}

const S64ReuAllocation* s64_code_assets_session_allocation(const S64ReuRuntimeControl* control)
{
    const S64ReuAllocation* found = 0;
    uint8_t i;
    if (!s64_reu_runtime_valid(control)) return 0;
    for (i = 0; i < control->allocation_count; ++i) {
        const S64ReuAllocation* item = &control->allocations[i];
        if (item->owner == S64_REU_OWNER_RUNTIME && item->flags == S64_CODE_ASSET_SESSION_FLAG) {
            if (found || item->length != S64_CODE_ASSET_SESSION_SIZE
                || (item->address & 255UL)) return 0;
            found = item;
        }
    }
    return found;
}

uint8_t s64_code_assets_clear(const S64ReuRuntimeControl* control)
{
    const S64ReuAllocation* allocation = s64_code_assets_session_allocation(control);
    if (!allocation) return 0;
    memset(s64_code_assets_session_wire, 0, sizeof(s64_code_assets_session_wire));
    return s64_reu_store((uint8_t)(allocation->address >> 16), (uint16_t)allocation->address,
            s64_code_assets_session_wire, sizeof(s64_code_assets_session_wire))
        && s64_reu_load((uint8_t)(allocation->address >> 16), (uint16_t)allocation->address,
            s64_code_assets_session_readback, sizeof(s64_code_assets_session_readback))
        && memcmp(s64_code_assets_session_wire, s64_code_assets_session_readback,
            sizeof(s64_code_assets_session_wire)) == 0;
}

uint8_t s64_code_assets_active_session(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal)
{
    return journal != 0 && journal->state == S64_SESSION_STATE_ACTIVE && journal->sequence
        && journal->session_id == control->session_id && journal->package_version == control->app_version
        && s64_session_journal_encode(journal, s64_code_assets_session_journal_wire);
}
