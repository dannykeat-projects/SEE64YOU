#include "platform/application_state_internal.h"
#include "platform/crc32.h"
#include "platform/reu.h"
#include <string.h>

static uint16_t get16(const uint8_t* p)
{ return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
static uint32_t get32(const uint8_t* p)
{ return (uint32_t)get16(p) | ((uint32_t)get16(p + 2) << 16); }
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

uint8_t s64_application_state_wire_valid(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const uint8_t* w)
{
    uint8_t expected[S64_SESSION_JOURNAL_WIRE_SIZE];
    if (!w || !s64_application_state_active(control, journal, expected)
        || w[0] != 0x53 || w[1] != 0x36 || w[2] != 0x34 || w[3] != 0x41
        || w[4] != 1 || w[5] != S64_REU_RUNTIME_ABI_VERSION
        || get16(w + 6) != S64_APPLICATION_STATE_SIZE
        || memcmp(w + 8, expected, sizeof(expected))
        || get32(w + 508) != (s64_crc32_update(0xffffffffUL, w, 508) ^ 0xffffffffUL)) return 0;
    return 1;
}

uint8_t s64_application_state_load_wire(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, uint8_t* wire)
{
    const S64ReuAllocation* item = s64_application_state_allocation(control);
    return item && wire
        && s64_reu_load((uint8_t)(item->address >> 16), (uint16_t)item->address,
            wire, S64_APPLICATION_STATE_SIZE)
        && s64_application_state_wire_valid(control, journal, wire);
}
