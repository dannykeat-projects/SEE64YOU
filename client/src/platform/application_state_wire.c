#include "platform/application_state_internal.h"
#include "platform/crc32.h"
#include "platform/reu.h"
#include <string.h>

static uint16_t get16(const uint8_t* p)
{ return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
static uint32_t get32(const uint8_t* p)
{ return (uint32_t)get16(p) | ((uint32_t)get16(p + 2) << 16); }
uint8_t s64_application_state_wire_valid(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, const uint8_t* w)
{
    uint8_t expected[S64_SESSION_JOURNAL_WIRE_SIZE];
    static const uint8_t header[] = {0x53, 0x36, 0x34, 0x41,
        1, S64_REU_RUNTIME_ABI_VERSION, 0, 2};
    if (!w || !s64_application_state_active(control, journal, expected)
        || memcmp(w, header, sizeof(header))
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
