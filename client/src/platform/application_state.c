#include "platform/application_state_internal.h"
const S64ReuAllocation* s64_application_state_allocation(const S64ReuRuntimeControl* control)
{
    const S64ReuAllocation* found = 0;
    uint8_t i;
    if (!s64_reu_runtime_valid(control)) return 0;
    for (i = 0; i < control->allocation_count; ++i) {
        const S64ReuAllocation* item = &control->allocations[i];
        if (item->flags == S64_APPLICATION_STATE_FLAG) {
            if (found || item->owner != S64_REU_OWNER_RUNTIME
                || item->length != S64_APPLICATION_STATE_SIZE
                || (item->address & 255UL)) return 0;
            found = item;
        }
    }
    return found;
}

uint8_t s64_application_state_active(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, uint8_t* expected)
{
    return s64_reu_runtime_valid(control) && journal
        && journal->state == S64_SESSION_STATE_ACTIVE && journal->sequence
        && journal->session_id == control->session_id
        && journal->package_version == control->app_version
        && s64_session_journal_encode(journal, expected);
}
