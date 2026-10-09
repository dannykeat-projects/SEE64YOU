#ifndef SEE64YOU_BASIC_RETURN_H
#define SEE64YOU_BASIC_RETURN_H
#include "platform/reu_runtime.h"
#include "platform/session_journal.h"

#define S64_BASIC_RETURN_FLAG 4u
#define S64_BASIC_RETURN_RAW_SIZE 285u
#define S64_BASIC_RETURN_RECORD_SIZE 321u
#define S64_BASIC_RETURN_WORKSPACE_SIZE (2u * S64_BASIC_RETURN_RECORD_SIZE)
#ifndef S64_BASIC_RETURN_WORKSPACE
#define S64_BASIC_RETURN_WORKSPACE ((uint8_t*)0xc800u)
#endif
#define S64_BASIC_RETURN_SNAPSHOT (S64_BASIC_RETURN_WORKSPACE + S64_BASIC_RETURN_WORKSPACE_SIZE)
#if S64_BASIC_RETURN_WORKSPACE_SIZE + S64_BASIC_RETURN_RECORD_SIZE > 1024u
#error BASIC return scratch exceeds the existing overlay/stack gap
#endif

/* Workspace and preserved snapshot use 963 bytes of the existing 1 KiB
 * overlay/stack gap. Capsule/session teardown may reuse the workspace, but
 * must not touch the snapshot. Producer
 * must enter through basic_return_capture.s before stock cc65 startup. */
uint8_t s64_basic_return_reserve(S64ReuRuntimeControl* control);
uint8_t s64_basic_return_publish(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, uint8_t* workspace);
const S64ReuAllocation* s64_basic_return_allocation(const S64ReuRuntimeControl* control);
uint8_t s64_basic_return_read(const S64ReuRuntimeControl* control,
    const S64SessionJournal* journal, uint8_t* workspace);
/* Missing allocation is a no-op; malformed ownership or failed readback fails.
 * Uses the second workspace half, preserving a validated first-half snapshot. */
uint8_t s64_basic_return_revoke(const S64ReuRuntimeControl* control, uint8_t* workspace);
/* Only after read validated trusted ACTIVE and revocation/restore/CLEAN passed. */
void s64_basic_return_execute(const uint8_t* workspace);
#endif
