#include "platform/basic_return_internal.h"
#include "platform/crc32.h"
#include "platform/reu.h"
#include <string.h>

uint8_t s64_basic_return_read(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, uint8_t* workspace)
{
    const S64ReuAllocation* item = s64_basic_return_allocation(control);
    uint32_t crc;
    uint8_t i;
    if (!workspace || !item || !s64_basic_return_active(control, journal,
            workspace + S64_BASIC_RETURN_RECORD_SIZE)
        || !s64_reu_load((uint8_t)(item->address >> 16), (uint16_t)item->address,
            workspace, S64_BASIC_RETURN_RECORD_SIZE)
        || !s64_basic_return_header_valid(workspace)
        || memcmp(workspace + 8, workspace + S64_BASIC_RETURN_RECORD_SIZE,
            S64_SESSION_JOURNAL_WIRE_SIZE)) return 0;
    crc = s64_crc32_update(0xffffffffUL, workspace, S64_BASIC_RETURN_RECORD_SIZE - 4) ^ 0xffffffffUL;
    for (i = 0; i < 4; ++i) {
        if (workspace[S64_BASIC_RETURN_RECORD_SIZE - 4 + i] != (uint8_t)crc) return 0;
        crc >>= 8;
    }
    return 1;
}

#if defined(__CC65__) && defined(S64_MANAGED_OVERLAY)
extern const uint8_t s64_basic_return_stub_image[], s64_basic_return_stub_end[];
void s64_basic_return_stub_enter(void);
void s64_basic_return_execute(const uint8_t* workspace)
{
    memcpy((void*)0x0400, workspace + 32, S64_BASIC_RETURN_RAW_SIZE);
    memcpy((void*)0x0700, s64_basic_return_stub_image,
        s64_basic_return_stub_end - s64_basic_return_stub_image);
    s64_basic_return_stub_enter();
}
#endif
