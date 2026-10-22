#include "platform/basic_return.h"
#include "platform/reu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t record[S64_BASIC_RETURN_RECORD_SIZE], workspace[S64_BASIC_RETURN_WORKSPACE_SIZE];
static uint8_t saved[S64_BASIC_RETURN_RECORD_SIZE];
static S64ReuRuntimeControl control;
static S64SessionJournal journal;
static uint32_t address;
static unsigned reads, writes, fail_read, fail_write, corrupt_read;

uint8_t s64_reu_load(uint8_t bank, uint16_t offset, void* destination, uint16_t length)
{
    assert((((uint32_t)bank << 16) | offset) == address && length == sizeof(record));
    ++reads;
    if (fail_read) return 0;
    memcpy(destination, record, length);
    if (corrupt_read) ((uint8_t*)destination)[0] ^= 1;
    return 1;
}
uint8_t s64_reu_store(uint8_t bank, uint16_t offset, const void* source, uint16_t length)
{
    assert((((uint32_t)bank << 16) | offset) == address && length == sizeof(record));
    ++writes;
    if (fail_write) return 0;
    memcpy(record, source, length);
    return 1;
}
static void capture(void)
{
    unsigned i;
    memset(workspace, 0, sizeof(workspace));
    workspace[0] = 0x53; workspace[1] = 0x36; workspace[2] = 0x34; workspace[3] = 0x42;
    workspace[4] = 1; workspace[5] = 26; workspace[6] = 0x1d; workspace[7] = 1;
    for (i = 0; i < S64_BASIC_RETURN_RAW_SIZE; ++i) workspace[32 + i] = (uint8_t)i;
    workspace[32] = 0x37; workspace[33] = 0xf0; workspace[34] = 0x24;
}
int main(void)
{
    S64ReuRuntimeControl before;
    S64SessionJournal changed;
    unsigned i;
    s64_reu_runtime_init(&control, 513, 123);
    memset(&journal, 0, sizeof(journal));
    journal.state = S64_SESSION_STATE_ACTIVE; journal.package_version = 513;
    journal.session_id = 123; journal.sequence = 5;
    journal.restore_flags = S64_SESSION_JOURNAL_RESTORE_TURBO;
    assert(s64_basic_return_revoke(&control, workspace)); /* older package, no record */
    assert(s64_basic_return_reserve(&control));
    assert(control.allocation_count == 1);
    address = s64_basic_return_allocation(&control)->address;
    before = control;
    assert(!s64_basic_return_reserve(&control));
    assert(!memcmp(&control, &before, sizeof(control)));
    capture();
    assert(s64_basic_return_publish(&control, &journal, workspace));
    memcpy(saved, workspace, sizeof(saved));
    memset(workspace, 0x77, sizeof(workspace));
    assert(s64_basic_return_read(&control, &journal, workspace));
    assert(!memcmp(workspace, saved, sizeof(saved)));
    changed = journal; changed.state = S64_SESSION_STATE_CLEAN;
    assert(!s64_basic_return_read(&control, &changed, workspace));
    changed = journal; ++changed.sequence;
    assert(!s64_basic_return_read(&control, &changed, workspace));
    changed = journal; ++changed.session_id;
    assert(!s64_basic_return_read(&control, &changed, workspace));
    /* Integrity covers every header, journal and raw-context byte. */
    for (i = 0; i < sizeof(record); ++i) {
        record[i] ^= 1;
        assert(!s64_basic_return_read(&control, &journal, workspace));
        record[i] ^= 1;
    }
    assert(s64_basic_return_read(&control, &journal, workspace));
    memcpy(saved, workspace, sizeof(saved));
    fail_write = 1;
    assert(!s64_basic_return_revoke(&control, workspace)); fail_write = 0;
    fail_read = 1;
    assert(!s64_basic_return_read(&control, &journal, workspace));
    assert(!s64_basic_return_revoke(&control, workspace)); fail_read = 0;
    capture(); assert(s64_basic_return_publish(&control, &journal, workspace));
    corrupt_read = 1;
    assert(!s64_basic_return_revoke(&control, workspace)); corrupt_read = 0;
    capture(); assert(s64_basic_return_publish(&control, &journal, workspace));
    memcpy(saved, workspace, sizeof(saved));
    assert(s64_basic_return_revoke(&control, workspace));
    assert(!memcmp(workspace, saved, sizeof(saved))); /* snapshot survives CLEAN I/O */
    assert(!s64_basic_return_read(&control, &journal, workspace));
    before = control;
    ++control.allocations[0].length; ++control.next_free;
    assert(!s64_basic_return_allocation(&control));
    assert(!s64_basic_return_revoke(&control, workspace));
    control = before;
    capture(); fail_write = 1;
    assert(!s64_basic_return_publish(&control, &journal, workspace)); fail_write = 0;
    corrupt_read = 1;
    assert(!s64_basic_return_publish(&control, &journal, workspace)); corrupt_read = 0;
    workspace[0] = 0;
    assert(!s64_basic_return_publish(&control, &journal, workspace));
    assert(reads && writes);
    puts("BASIC return session/ownership/integrity/revocation tests passed");
    return 0;
}
