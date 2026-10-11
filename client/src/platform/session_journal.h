#ifndef SEE64YOU_PLATFORM_SESSION_JOURNAL_H
#define SEE64YOU_PLATFORM_SESSION_JOURNAL_H

#include <stdint.h>

#include "platform/dos.h"
#include "platform/system_state.h"

#define S64_SESSION_JOURNAL_FORMAT_VERSION 1u
#define S64_SESSION_JOURNAL_WIRE_SIZE 24u
#define S64_SESSION_JOURNAL_RESTORE_TURBO 0x01u
#define S64_SESSION_JOURNAL_FILENAME_0 "S64JRN0.DAT"
#define S64_SESSION_JOURNAL_FILENAME_1 "S64JRN1.DAT"
#define S64_SESSION_JOURNAL_STORAGE_PATH "/sd/SEE64YOU/"

typedef enum {
    S64_SESSION_STATE_ACTIVE = 1,
    S64_SESSION_STATE_CLEAN = 2
} S64SessionState;

typedef enum {
    S64_SESSION_JOURNAL_OK = 0,
    S64_SESSION_JOURNAL_NOT_FOUND,
    S64_SESSION_JOURNAL_INVALID,
    S64_SESSION_JOURNAL_IO_ERROR,
    S64_SESSION_JOURNAL_VERIFY_ERROR
} S64SessionJournalResult;

typedef struct {
    uint8_t state;
    uint16_t package_version;
    uint32_t session_id;
    uint32_t sequence;
    uint32_t recovery_ref;
    uint8_t restore_flags;
    uint8_t turbo_control;
} S64SessionJournal;

uint8_t s64_session_journal_prepare(
    S64SessionJournal* journal,
    uint32_t session_id,
    uint16_t package_version,
    uint32_t recovery_ref,
    const S64SystemSnapshot* snapshot
);

uint8_t s64_session_journal_mark_clean(S64SessionJournal* journal);
uint8_t s64_session_journal_is_unclean(const S64SessionJournal* journal);

uint8_t s64_session_journal_restore_snapshot(
    const S64SessionJournal* journal,
    S64SystemSnapshot* snapshot
);

uint8_t s64_session_journal_encode(
    const S64SessionJournal* journal,
    uint8_t wire[S64_SESSION_JOURNAL_WIRE_SIZE]
);

uint8_t s64_session_journal_decode(
    S64SessionJournal* journal,
    const uint8_t wire[S64_SESSION_JOURNAL_WIRE_SIZE]
);

S64SessionJournalResult s64_session_journal_store_load(
    S64Dos* dos,
    S64SessionJournal* journal
);

S64SessionJournalResult s64_session_journal_store_save(
    S64Dos* dos,
    S64SessionJournal* journal
);

#endif
