#include "platform/basic_return_internal.h"
#include "platform/reu.h"
#include <string.h>

const S64ReuAllocation* s64_basic_return_allocation(const S64ReuRuntimeControl* control)
{
    const S64ReuAllocation* found = 0;
    uint8_t i;
    if (!s64_reu_runtime_valid(control)) return 0;
    for (i = 0; i < control->allocation_count; ++i) {
        const S64ReuAllocation* item = &control->allocations[i];
        if (item->owner == S64_REU_OWNER_RUNTIME && item->flags == S64_BASIC_RETURN_FLAG) {
            if (found || item->length != S64_BASIC_RETURN_RECORD_SIZE || (item->address & 255UL)) return 0;
            found = item;
        }
    }
    return found;
}

uint8_t s64_basic_return_header_valid(const uint8_t* wire)
{
    return wire != 0 && wire[0] == 0x53 && wire[1] == 0x36 && wire[2] == 0x34 && wire[3] == 0x42
        && wire[4] == 1 && wire[5] == 26 && wire[6] == 0x1d && wire[7] == 1
        && (wire[34] & 0x20); /* PHP always captures the fixed processor-status bit. */
}

uint8_t s64_basic_return_active(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, uint8_t* expected)
{
    return journal != 0 && journal->state == S64_SESSION_STATE_ACTIVE && journal->sequence
        && journal->session_id == control->session_id && journal->package_version == control->app_version
        && s64_session_journal_encode(journal, expected);
}

uint8_t s64_basic_return_revoke(const S64ReuRuntimeControl* control, uint8_t* workspace)
{
    const S64ReuAllocation* item;
    uint8_t i;
    uint16_t j;
    uint8_t* scratch;
    if (!workspace || !s64_reu_runtime_valid(control)) return 0;
    for (i = 0; i < control->allocation_count; ++i)
        if (control->allocations[i].owner == S64_REU_OWNER_RUNTIME
            && control->allocations[i].flags == S64_BASIC_RETURN_FLAG) break;
    if (i == control->allocation_count) return 1;
    item = s64_basic_return_allocation(control);
    if (!item) return 0;
    scratch = workspace + S64_BASIC_RETURN_RECORD_SIZE;
    memset(scratch, 0, S64_BASIC_RETURN_RECORD_SIZE);
    if (!s64_reu_store((uint8_t)(item->address >> 16), (uint16_t)item->address,
            scratch, S64_BASIC_RETURN_RECORD_SIZE)
        || !s64_reu_load((uint8_t)(item->address >> 16), (uint16_t)item->address,
            scratch, S64_BASIC_RETURN_RECORD_SIZE)) return 0;
    for (j = 0; j < S64_BASIC_RETURN_RECORD_SIZE; ++j) if (scratch[j]) return 0;
    return 1;
}
