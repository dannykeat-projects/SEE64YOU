#include "platform/journal_migration.h"
#include "platform/package_paths.h"

#include <string.h>

typedef struct {
    uint8_t bytes[S64_SESSION_JOURNAL_WIRE_SIZE];
    uint8_t count;
} MigrationRead;

static const char* slot_name(uint8_t slot)
{
    return slot ? S64_SESSION_JOURNAL_FILENAME_1
        : S64_SESSION_JOURNAL_FILENAME_0;
}

static uint8_t collect(const uint8_t* data, uint16_t length, void* context)
{
    MigrationRead* read = (MigrationRead*)context;
    if (length > S64_SESSION_JOURNAL_WIRE_SIZE - read->count) return 0u;
    memcpy(read->bytes + read->count, data, length);
    read->count = (uint8_t)(read->count + length);
    return 1u;
}

static uint8_t equal(const S64SessionJournal* left,
    const S64SessionJournal* right)
{
    return left->state == right->state
        && left->package_version == right->package_version
        && left->session_id == right->session_id
        && left->sequence == right->sequence
        && left->recovery_ref == right->recovery_ref
        && left->restore_flags == right->restore_flags
        && left->turbo_control == right->turbo_control;
}

static S64SessionJournalResult read_slot(S64Dos* dos, uint8_t slot,
    S64SessionJournal* journal)
{
    MigrationRead read;
    S64DosResult result;
    S64DosResult close_result;
    uint32_t size;
    uint16_t count = 0u;
    uint8_t exists;

    if (s64_dos_file_exists(dos, slot_name(slot), &exists) != S64_DOS_OK)
        return S64_SESSION_JOURNAL_IO_ERROR;
    if (!exists) return S64_SESSION_JOURNAL_NOT_FOUND;
    if (s64_dos_open_read(dos, slot_name(slot)) != S64_DOS_OK)
        return S64_SESSION_JOURNAL_IO_ERROR;
    result = s64_dos_file_size(dos, &size);
    if (result != S64_DOS_OK || size != S64_SESSION_JOURNAL_WIRE_SIZE) {
        close_result = s64_dos_close(dos);
        return result != S64_DOS_OK || close_result != S64_DOS_OK
            ? S64_SESSION_JOURNAL_IO_ERROR : S64_SESSION_JOURNAL_INVALID;
    }
    read.count = 0u;
    result = s64_dos_read(dos, S64_SESSION_JOURNAL_WIRE_SIZE,
        collect, &read, &count);
    close_result = s64_dos_close(dos);
    if (result != S64_DOS_OK || close_result != S64_DOS_OK)
        return S64_SESSION_JOURNAL_IO_ERROR;
    if (count != S64_SESSION_JOURNAL_WIRE_SIZE
        || read.count != S64_SESSION_JOURNAL_WIRE_SIZE
        || !s64_session_journal_decode(journal, read.bytes))
        return S64_SESSION_JOURNAL_INVALID;
    return S64_SESSION_JOURNAL_OK;
}

static S64SessionJournalResult read_store(S64Dos* dos, const char* path,
    S64SessionJournal* journal, uint8_t* selected_slot)
{
    S64SessionJournal first;
    S64SessionJournal second;
    S64SessionJournalResult a;
    S64SessionJournalResult b;

    if (s64_dos_change_dir(dos, path) != S64_DOS_OK)
        return S64_SESSION_JOURNAL_IO_ERROR;
    a = read_slot(dos, 0u, &first);
    b = read_slot(dos, 1u, &second);
    if (a == S64_SESSION_JOURNAL_IO_ERROR || b == S64_SESSION_JOURNAL_IO_ERROR)
        return S64_SESSION_JOURNAL_IO_ERROR;
    if (a == S64_SESSION_JOURNAL_OK && b == S64_SESSION_JOURNAL_OK) {
        if (first.sequence == second.sequence && !equal(&first, &second))
            return S64_SESSION_JOURNAL_INVALID;
        memcpy(journal, second.sequence > first.sequence ? &second : &first,
            sizeof(*journal));
        if (selected_slot != 0)
            *selected_slot = second.sequence > first.sequence ? 1u : 0u;
        return S64_SESSION_JOURNAL_OK;
    }
    if (a == S64_SESSION_JOURNAL_OK || b == S64_SESSION_JOURNAL_OK) {
        memcpy(journal, a == S64_SESSION_JOURNAL_OK ? &first : &second,
            sizeof(*journal));
        if (selected_slot != 0)
            *selected_slot = a == S64_SESSION_JOURNAL_OK ? 0u : 1u;
        return S64_SESSION_JOURNAL_OK;
    }
    return a == S64_SESSION_JOURNAL_NOT_FOUND && b == a
        ? S64_SESSION_JOURNAL_NOT_FOUND : S64_SESSION_JOURNAL_INVALID;
}

static S64SessionJournalResult write_next(S64Dos* dos, const char* path,
    S64SessionJournal* journal, uint8_t slot)
{
    uint8_t wire[S64_SESSION_JOURNAL_WIRE_SIZE];
    S64SessionJournal pending;
    S64SessionJournal verify;
    S64DosResult result;
    S64DosResult close_result;

    memcpy(&pending, journal, sizeof(pending));
    /* Wrap would violate highest-sequence selection while preserving the old
     * slot. Block rather than making the old ACTIVE authoritative again. */
    if (pending.sequence == 0xffffffffUL) return S64_SESSION_JOURNAL_INVALID;
    ++pending.sequence;
    if (!s64_session_journal_encode(&pending, wire))
        return S64_SESSION_JOURNAL_INVALID;
    if (s64_dos_change_dir(dos, path) != S64_DOS_OK
        || s64_dos_open_write(dos, slot_name(slot)) != S64_DOS_OK)
        return S64_SESSION_JOURNAL_IO_ERROR;
    result = s64_dos_write(dos, wire, S64_SESSION_JOURNAL_WIRE_SIZE);
    close_result = s64_dos_close(dos);
    if (result != S64_DOS_OK || close_result != S64_DOS_OK)
        return S64_SESSION_JOURNAL_IO_ERROR;
    if (read_slot(dos, slot, &verify) != S64_SESSION_JOURNAL_OK
        || !equal(&pending, &verify)) return S64_SESSION_JOURNAL_VERIFY_ERROR;
    memcpy(journal, &pending, sizeof(*journal));
    return S64_SESSION_JOURNAL_OK;
}

S64SessionJournalResult s64_journal_migration_stage(S64Dos* dos,
    const S64SessionRecoveryBackend* backend)
{
    S64SessionJournal root;
    S64SessionJournal folder;
    S64SystemSnapshot snapshot;
    S64SessionJournalResult root_result;
    S64SessionJournalResult folder_result;
    uint8_t root_slot;

    if (dos == 0) return S64_SESSION_JOURNAL_INVALID;
    folder_result = read_store(dos, S64_PACKAGE_DIRECTORY, &folder, 0);
    root_result = read_store(dos, S64_SD_ROOT_DIRECTORY, &root, &root_slot);
    if (folder_result != S64_SESSION_JOURNAL_OK
        && folder_result != S64_SESSION_JOURNAL_NOT_FOUND) return folder_result;
    if (root_result != S64_SESSION_JOURNAL_OK
        && root_result != S64_SESSION_JOURNAL_NOT_FOUND) return root_result;
    if (root_result == S64_SESSION_JOURNAL_NOT_FOUND)
        return S64_SESSION_JOURNAL_OK;
    if (root.state == S64_SESSION_STATE_ACTIVE) {
        if (folder_result == S64_SESSION_JOURNAL_OK
            && folder.state == S64_SESSION_STATE_ACTIVE)
            return S64_SESSION_JOURNAL_INVALID;
        if (!s64_session_journal_restore_snapshot(&root, &snapshot))
            return S64_SESSION_JOURNAL_INVALID;
        if (snapshot.turbo_control_valid) {
            if (backend == 0 || backend->restore == 0 || backend->verify == 0)
                return S64_SESSION_JOURNAL_INVALID;
            if (!backend->restore(backend->context, &snapshot)
                || !backend->verify(backend->context, &snapshot))
                return S64_SESSION_JOURNAL_VERIFY_ERROR;
        }
        if (!s64_session_journal_mark_clean(&root))
            return S64_SESSION_JOURNAL_INVALID;
        root_result = write_next(dos, S64_SD_ROOT_DIRECTORY, &root,
            (uint8_t)(root_slot ^ 1u));
        if (root_result != S64_SESSION_JOURNAL_OK) return root_result;
    }
    /* Existing folder state has its own sequence and restoration ownership. */
    if (folder_result == S64_SESSION_JOURNAL_OK)
        return S64_SESSION_JOURNAL_OK;
    root.sequence = 0UL;
    return write_next(dos, S64_PACKAGE_DIRECTORY, &root, 1u);
}
