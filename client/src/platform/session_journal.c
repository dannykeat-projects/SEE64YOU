#include "platform/session_journal.h"

#include <string.h>

#define S64_JOURNAL_MAGIC_0 0x53u
#define S64_JOURNAL_MAGIC_1 0x36u
#define S64_JOURNAL_MAGIC_2 0x34u
#define S64_JOURNAL_MAGIC_3 0x4au
#define S64_JOURNAL_CHECKSUM_OFFSET 22u

typedef struct {
    uint8_t bytes[S64_SESSION_JOURNAL_WIRE_SIZE];
    uint8_t offset;
    uint8_t overflow;
} S64JournalRead;

static void write_u16(uint8_t* bytes, uint16_t value)
{
    bytes[0] = (uint8_t)(value & 0xffu);
    bytes[1] = (uint8_t)(value >> 8);
}

static uint16_t read_u16(const uint8_t* bytes)
{
    return (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));
}

static void write_u32(uint8_t* bytes, uint32_t value)
{
    bytes[0] = (uint8_t)(value & 0xffUL);
    bytes[1] = (uint8_t)((value >> 8) & 0xffUL);
    bytes[2] = (uint8_t)((value >> 16) & 0xffUL);
    bytes[3] = (uint8_t)((value >> 24) & 0xffUL);
}

static uint32_t read_u32(const uint8_t* bytes)
{
    return (uint32_t)bytes[0]
        | ((uint32_t)bytes[1] << 8)
        | ((uint32_t)bytes[2] << 16)
        | ((uint32_t)bytes[3] << 24);
}

static uint16_t journal_checksum(const uint8_t* wire)
{
    uint8_t sum1 = 0x53u;
    uint8_t sum2 = 0x4au;
    uint8_t i;

    for (i = 0u; i < S64_JOURNAL_CHECKSUM_OFFSET; ++i) {
        sum1 = (uint8_t)(sum1 + wire[i]);
        sum2 = (uint8_t)(sum2 + sum1);
    }
    return (uint16_t)(((uint16_t)sum2 << 8) | sum1);
}

static uint8_t state_valid(uint8_t state)
{
    return state == S64_SESSION_STATE_ACTIVE
        || state == S64_SESSION_STATE_CLEAN;
}

static uint8_t journal_values_valid(const S64SessionJournal* journal)
{
    if (journal == 0
        || !state_valid(journal->state)
        || journal->session_id == 0UL
        || (journal->restore_flags
            & (uint8_t)~S64_SESSION_JOURNAL_RESTORE_TURBO) != 0u) {
        return 0u;
    }
    return 1u;
}

uint8_t s64_session_journal_mark_clean(S64SessionJournal* journal)
{
    if (!journal_values_valid(journal)) return 0u;
    journal->state = S64_SESSION_STATE_CLEAN;
    return 1u;
}

uint8_t s64_session_journal_is_unclean(const S64SessionJournal* journal)
{
    return journal_values_valid(journal)
        && journal->state == S64_SESSION_STATE_ACTIVE;
}

uint8_t s64_session_journal_restore_snapshot(
    const S64SessionJournal* journal,
    S64SystemSnapshot* snapshot
)
{
    if (!journal_values_valid(journal) || snapshot == 0) return 0u;

    memset(snapshot, 0, sizeof(*snapshot));
    if ((journal->restore_flags & S64_SESSION_JOURNAL_RESTORE_TURBO) != 0u) {
        snapshot->turbo_control = journal->turbo_control;
        snapshot->turbo_control_valid = 1u;
    }
    return 1u;
}

uint8_t s64_session_journal_encode(
    const S64SessionJournal* journal,
    uint8_t wire[S64_SESSION_JOURNAL_WIRE_SIZE]
)
{
    uint16_t checksum;

    if (!journal_values_valid(journal) || wire == 0) return 0u;

    memset(wire, 0, S64_SESSION_JOURNAL_WIRE_SIZE);
    wire[0] = S64_JOURNAL_MAGIC_0;
    wire[1] = S64_JOURNAL_MAGIC_1;
    wire[2] = S64_JOURNAL_MAGIC_2;
    wire[3] = S64_JOURNAL_MAGIC_3;
    wire[4] = S64_SESSION_JOURNAL_FORMAT_VERSION;
    wire[5] = journal->state;
    write_u16(wire + 6u, journal->package_version);
    write_u32(wire + 8u, journal->session_id);
    write_u32(wire + 12u, journal->sequence);
    write_u32(wire + 16u, journal->recovery_ref);
    wire[20] = journal->restore_flags;
    wire[21] = journal->turbo_control;
    checksum = journal_checksum(wire);
    write_u16(wire + S64_JOURNAL_CHECKSUM_OFFSET, checksum);
    return 1u;
}

uint8_t s64_session_journal_decode(
    S64SessionJournal* journal,
    const uint8_t wire[S64_SESSION_JOURNAL_WIRE_SIZE]
)
{
    S64SessionJournal decoded;

    if (journal == 0 || wire == 0
        || wire[0] != S64_JOURNAL_MAGIC_0
        || wire[1] != S64_JOURNAL_MAGIC_1
        || wire[2] != S64_JOURNAL_MAGIC_2
        || wire[3] != S64_JOURNAL_MAGIC_3
        || wire[4] != S64_SESSION_JOURNAL_FORMAT_VERSION
        || read_u16(wire + S64_JOURNAL_CHECKSUM_OFFSET)
            != journal_checksum(wire)) {
        return 0u;
    }

    memset(&decoded, 0, sizeof(decoded));
    decoded.state = wire[5];
    decoded.package_version = read_u16(wire + 6u);
    decoded.session_id = read_u32(wire + 8u);
    decoded.sequence = read_u32(wire + 12u);
    decoded.recovery_ref = read_u32(wire + 16u);
    decoded.restore_flags = wire[20];
    decoded.turbo_control = wire[21];

    if (!journal_values_valid(&decoded)) return 0u;
    memcpy(journal, &decoded, sizeof(*journal));
    return 1u;
}

static uint8_t collect_wire(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64JournalRead* read = (S64JournalRead*)context;

    if ((uint16_t)read->offset + length > S64_SESSION_JOURNAL_WIRE_SIZE) {
        read->overflow = 1u;
        return 0u;
    }
    memcpy(read->bytes + read->offset, data, length);
    read->offset = (uint8_t)(read->offset + length);
    return 1u;
}

static const char* slot_filename(uint8_t slot);

static S64SessionJournalResult select_storage(S64Dos* dos)
{
#ifdef S64_CODE_ASSET_LAUNCHER
    uint8_t exists;
    uint8_t slot;
#endif
    if (dos == 0) return S64_SESSION_JOURNAL_INVALID;
#ifdef S64_CODE_ASSET_LAUNCHER
    /* Settings resolves and backs up legacy ownership before Editor can run. */
    if (s64_dos_change_dir(dos, S64_STORAGE_ROOT_DIRECTORY) != S64_DOS_OK)
        return S64_SESSION_JOURNAL_IO_ERROR;
    for (slot = 0u; slot < 2u; ++slot) {
        if (s64_dos_file_exists(dos, slot_filename(slot), &exists) != S64_DOS_OK)
            return S64_SESSION_JOURNAL_IO_ERROR;
        if (exists) return S64_SESSION_JOURNAL_INVALID;
    }
#endif
    return s64_dos_change_dir(dos, S64_SESSION_JOURNAL_STORAGE_PATH)
        == S64_DOS_OK
        ? S64_SESSION_JOURNAL_OK
        : S64_SESSION_JOURNAL_IO_ERROR;
}

static S64SessionJournalResult load_slot(
    S64Dos* dos,
    const char* filename,
    S64SessionJournal* journal
)
{
    S64DosResult result;
    S64JournalRead read;
    uint8_t exists;
    uint16_t bytes_read = 0u;

    result = s64_dos_file_exists(dos, filename, &exists);
    if (result != S64_DOS_OK) return S64_SESSION_JOURNAL_IO_ERROR;
    if (!exists) return S64_SESSION_JOURNAL_NOT_FOUND;

    result = s64_dos_open_read(dos, filename);
    if (result != S64_DOS_OK) return S64_SESSION_JOURNAL_IO_ERROR;

    memset(&read, 0, sizeof(read));
    result = s64_dos_read(
        dos,
        S64_SESSION_JOURNAL_WIRE_SIZE,
        collect_wire,
        &read,
        &bytes_read
    );
    (void)s64_dos_close(dos);

    if (result != S64_DOS_OK) return S64_SESSION_JOURNAL_IO_ERROR;
    if (read.overflow
        || bytes_read != S64_SESSION_JOURNAL_WIRE_SIZE
        || read.offset != S64_SESSION_JOURNAL_WIRE_SIZE
        || !s64_session_journal_decode(journal, read.bytes)) {
        return S64_SESSION_JOURNAL_INVALID;
    }
    return S64_SESSION_JOURNAL_OK;
}

static const char* slot_filename(uint8_t slot)
{
    return slot == 0u
        ? S64_SESSION_JOURNAL_FILENAME_0
        : S64_SESSION_JOURNAL_FILENAME_1;
}

S64SessionJournalResult s64_session_journal_store_load(
    S64Dos* dos,
    S64SessionJournal* journal
)
{
    S64SessionJournal first;
    S64SessionJournal second;
    S64SessionJournalResult first_result;
    S64SessionJournalResult second_result;

    if (dos == 0 || journal == 0) return S64_SESSION_JOURNAL_INVALID;
    first_result = select_storage(dos);
    if (first_result != S64_SESSION_JOURNAL_OK) return first_result;

    first_result = load_slot(dos, S64_SESSION_JOURNAL_FILENAME_0, &first);
    second_result = load_slot(dos, S64_SESSION_JOURNAL_FILENAME_1, &second);

    if (first_result == S64_SESSION_JOURNAL_OK
        && second_result == S64_SESSION_JOURNAL_OK) {
        if (second.sequence > first.sequence) {
            memcpy(journal, &second, sizeof(*journal));
        } else {
            memcpy(journal, &first, sizeof(*journal));
        }
        return S64_SESSION_JOURNAL_OK;
    }
    if (first_result == S64_SESSION_JOURNAL_OK) {
        memcpy(journal, &first, sizeof(*journal));
        return S64_SESSION_JOURNAL_OK;
    }
    if (second_result == S64_SESSION_JOURNAL_OK) {
        memcpy(journal, &second, sizeof(*journal));
        return S64_SESSION_JOURNAL_OK;
    }

    if (first_result == S64_SESSION_JOURNAL_NOT_FOUND
        && second_result == S64_SESSION_JOURNAL_NOT_FOUND) {
        return S64_SESSION_JOURNAL_NOT_FOUND;
    }
    if (first_result == S64_SESSION_JOURNAL_IO_ERROR
        || second_result == S64_SESSION_JOURNAL_IO_ERROR) {
        return S64_SESSION_JOURNAL_IO_ERROR;
    }
    return S64_SESSION_JOURNAL_INVALID;
}

static uint8_t journal_equal(
    const S64SessionJournal* left,
    const S64SessionJournal* right
)
{
    return left->state == right->state
        && left->package_version == right->package_version
        && left->session_id == right->session_id
        && left->sequence == right->sequence
        && left->recovery_ref == right->recovery_ref
        && left->restore_flags == right->restore_flags
        && left->turbo_control == right->turbo_control;
}

S64SessionJournalResult s64_session_journal_store_save(
    S64Dos* dos,
    S64SessionJournal* journal
)
{
    S64SessionJournal current;
    S64SessionJournal verify;
    S64SessionJournal pending;
    S64SessionJournalResult load_result;
    S64DosResult result;
    uint8_t wire[S64_SESSION_JOURNAL_WIRE_SIZE];
    uint32_t next_sequence = 1UL;
    uint8_t slot;

    if (dos == 0 || !journal_values_valid(journal)) {
        return S64_SESSION_JOURNAL_INVALID;
    }
    if (select_storage(dos) != S64_SESSION_JOURNAL_OK) {
        return S64_SESSION_JOURNAL_IO_ERROR;
    }

    load_result = s64_session_journal_store_load(dos, &current);
    if (load_result == S64_SESSION_JOURNAL_OK) {
        next_sequence = current.sequence == 0xffffffffUL
            ? 1UL
            : current.sequence + 1UL;
    } else if (load_result != S64_SESSION_JOURNAL_NOT_FOUND
        && load_result != S64_SESSION_JOURNAL_INVALID) {
        return load_result;
    }

    memcpy(&pending, journal, sizeof(pending));
    pending.sequence = next_sequence;
    if (!s64_session_journal_encode(&pending, wire)) {
        return S64_SESSION_JOURNAL_INVALID;
    }

    slot = (uint8_t)(next_sequence & 1UL);
    result = s64_dos_open_write(dos, slot_filename(slot));
    if (result != S64_DOS_OK) return S64_SESSION_JOURNAL_IO_ERROR;

    result = s64_dos_write(dos, wire, S64_SESSION_JOURNAL_WIRE_SIZE);
    (void)s64_dos_close(dos);
    if (result != S64_DOS_OK) return S64_SESSION_JOURNAL_IO_ERROR;

    if (load_slot(dos, slot_filename(slot), &verify)
            != S64_SESSION_JOURNAL_OK
        || !journal_equal(&pending, &verify)) {
        return S64_SESSION_JOURNAL_VERIFY_ERROR;
    }

    memcpy(journal, &pending, sizeof(*journal));
    return S64_SESSION_JOURNAL_OK;
}
