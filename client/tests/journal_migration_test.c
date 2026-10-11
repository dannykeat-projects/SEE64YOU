#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/journal_migration.h"

typedef struct {
    uint8_t bytes[24];
    uint8_t exists;
    uint32_t size;
} Slot;

static Slot slots[2][2];
static uint8_t directory, opened, mode;
static uint16_t offset;
static int fault_dir, fault_slot;
static uint8_t fault_opcode, corrupt_write;
static unsigned writes, restores, verifies;
static uint8_t restore_ok, verify_ok;

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void status(S64UciReply* reply, const char* value)
{
    size_t length = strlen(value);
    reply->status_length = (uint16_t)length;
    if (reply->status && reply->status_capacity) {
        if (length >= reply->status_capacity) length = reply->status_capacity - 1u;
        memcpy(reply->status, value, length);
        reply->status[length] = 0;
    }
}

static int filename(const S64UciCommandPart* part)
{
    static const char* names[2] = { "S64JRN0.DAT", "S64JRN1.DAT" };
    unsigned i;
    for (i = 0; i < 2; ++i) {
        size_t length = strlen(names[i]);
        if ((part->length == length || part->length == length + 1u)
            && !memcmp(part->data, names[i], length)) return (int)i;
    }
    return -1;
}

static S64UciResult execute(const S64UciCommandPart* parts,
    uint8_t count, S64UciReply* reply)
{
    uint8_t opcode;
    int slot = opened;
    Slot* file;
    if (!count || parts[0].length < 2u) return S64_UCI_PROTOCOL_ERROR;
    opcode = parts[0].data[1];
    if (opcode == 0x11u) {
        if (count != 2u) return S64_UCI_PROTOCOL_ERROR;
        if (parts[1].length == 4u && !memcmp(parts[1].data, "/sd/", 4u)) directory = 0;
        else if (parts[1].length == 13u && !memcmp(parts[1].data, "/sd/SEE64YOU/", 13u)) directory = 1;
        else return S64_UCI_PROTOCOL_ERROR;
    }
    if (opcode == 0x08u || opcode == 0x02u) {
        if (count != 2u || (slot = filename(&parts[1])) < 0) return S64_UCI_PROTOCOL_ERROR;
    }
    if (fault_opcode == opcode && (fault_dir < 0 || directory == fault_dir)
        && (fault_slot < 0 || slot == fault_slot)) {
        status(reply, "99,INJECTED IO ERROR");
        return S64_UCI_OK;
    }
    file = &slots[directory][slot];
    switch (opcode) {
    case 0x11u: break;
    case 0x08u:
        status(reply, file->exists ? "00,OK" : "88,FILE NOT FOUND");
        return S64_UCI_OK;
    case 0x02u:
        mode = parts[0].data[2];
        if (mode == 1u && !file->exists) {
            status(reply, "88,FILE NOT FOUND");
            return S64_UCI_OK;
        }
        opened = (uint8_t)slot;
        offset = 0;
        if (mode != 1u) {
            file->exists = 1;
            file->size = 0;
            memset(file->bytes, 0, sizeof(file->bytes));
        }
        break;
    case 0x07u: {
        uint8_t bytes[4];
        unsigned i;
        for (i = 0; i < 4; ++i) bytes[i] = (uint8_t)(file->size >> (i * 8));
        if (reply->data_sink && !reply->data_sink(bytes, 4u, reply->data_context)) return S64_UCI_CONSUMER_ABORTED;
        break;
    }
    case 0x04u: {
        uint16_t requested = (uint16_t)(parts[0].data[2] | ((uint16_t)parts[0].data[3] << 8));
        uint16_t available = file->size > offset ? (uint16_t)(file->size - offset) : 0u;
        uint16_t delivered = requested < available ? requested : available;
        if (mode != 1u || delivered > sizeof(file->bytes)) return S64_UCI_PROTOCOL_ERROR;
        if (delivered && reply->data_sink && !reply->data_sink(file->bytes + offset,
            delivered, reply->data_context)) return S64_UCI_CONSUMER_ABORTED;
        offset = (uint16_t)(offset + delivered);
        break;
    }
    case 0x05u:
        if (count != 2u || parts[1].length != 24u || mode == 1u) return S64_UCI_PROTOCOL_ERROR;
        memcpy(file->bytes, parts[1].data, 24u);
        file->size = 24;
        ++writes;
        if (corrupt_write && (fault_dir < 0 || directory == fault_dir)
            && (fault_slot < 0 || slot == fault_slot)) file->bytes[0] ^= 0xffu;
        break;
    case 0x03u: mode = 0; break;
    default: return S64_UCI_PROTOCOL_ERROR;
    }
    status(reply, "00,OK");
    return S64_UCI_OK;
}

static uint8_t restore(void* context, const S64SystemSnapshot* snapshot)
{
    (void)context;
    require(snapshot->turbo_control_valid && snapshot->turbo_control == 0x25u, "restore original turbo");
    ++restores;
    return restore_ok;
}

static uint8_t verify(void* context, const S64SystemSnapshot* snapshot)
{
    (void)context;
    require(snapshot->turbo_control_valid && snapshot->turbo_control == 0x25u, "verify original turbo");
    ++verifies;
    return verify_ok;
}

static void reset(void)
{
    memset(slots, 0, sizeof(slots));
    directory = opened = mode = 0;
    offset = 0;
    writes = restores = verifies = 0;
    fault_opcode = corrupt_write = 0;
    fault_dir = fault_slot = -1;
    restore_ok = verify_ok = 1;
}

static void seed(unsigned dir, unsigned slot, uint8_t state, uint32_t sequence)
{
    S64SessionJournal journal;
    S64SystemSnapshot snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    snapshot.turbo_control_valid = 1;
    snapshot.turbo_control = 0x25u;
    require(s64_session_journal_prepare(&journal, 123u + dir, 1u, 456u, &snapshot), "prepare fixture");
    journal.state = state;
    journal.sequence = sequence;
    require(s64_session_journal_encode(&journal, slots[dir][slot].bytes), "encode fixture");
    slots[dir][slot].exists = 1;
    slots[dir][slot].size = 24;
}

static S64SessionJournal decoded(unsigned dir, unsigned slot)
{
    S64SessionJournal journal;
    require(slots[dir][slot].exists && s64_session_journal_decode(&journal,
        slots[dir][slot].bytes), "valid persisted journal");
    return journal;
}

int main(void)
{
    S64Dos dos;
    S64SessionRecoveryBackend backend = { restore, verify, 0 };
    S64SessionJournal journal;
    Slot original;
    unsigned opcode;
    static const uint8_t fault_ops[] = { 0x11u, 0x08u, 0x04u, 0x07u, 0x03u };
    static const uint8_t write_fault_ops[] = { 0x05u, 0x03u, 0x04u };
    s64_dos_init(&dos, execute, S64_DOS_TARGET_PRIMARY);
    reset();
    require(s64_journal_migration_stage(0, &backend) == S64_SESSION_JOURNAL_INVALID,
        "null DOS rejected");
    require(s64_journal_migration_stage(&dos, &backend) == S64_SESSION_JOURNAL_OK, "empty stores succeed");
    require(!writes && !restores && !verifies, "empty stores do not write or restore");

    reset(); seed(0, 0, S64_SESSION_STATE_CLEAN, 7);
    original = slots[0][0];
    require(s64_journal_migration_stage(&dos, &backend) == S64_SESSION_JOURNAL_OK, "copy root CLEAN");
    require(!memcmp(&original, &slots[0][0], sizeof(original)), "root CLEAN unchanged");
    journal = decoded(1, 1);
    require(journal.state == S64_SESSION_STATE_CLEAN && journal.sequence == 1u, "folder CLEAN starts own sequence");
    require(!restores && !verifies, "CLEAN does not restore");

    reset(); seed(0, 0, S64_SESSION_STATE_ACTIVE, 7);
    original = slots[0][0];
    require(s64_journal_migration_stage(&dos, &backend) == S64_SESSION_JOURNAL_OK, "recover root ACTIVE");
    require(restores == 1u && verifies == 1u, "ACTIVE requires restore and verify");
    require(!memcmp(&original, &slots[0][0], sizeof(original)), "old ACTIVE retained byte exact");
    journal = decoded(0, 1);
    require(journal.state == S64_SESSION_STATE_CLEAN && journal.sequence == 8u
        && journal.session_id == 123u && journal.recovery_ref == 456u, "root alternate matching CLEAN");
    require(decoded(1, 1).state == S64_SESSION_STATE_CLEAN, "folder seeded only after root CLEAN");

    reset(); seed(0, 0, S64_SESSION_STATE_ACTIVE, 7); seed(1, 0, S64_SESSION_STATE_ACTIVE, 9);
    require(s64_journal_migration_stage(&dos, &backend) != S64_SESSION_JOURNAL_OK, "concurrent ACTIVE blocked");
    require(!writes && !restores, "collision untouched");

    reset(); seed(0, 0, S64_SESSION_STATE_CLEAN, 7); seed(1, 0, S64_SESSION_STATE_ACTIVE, 9);
    original = slots[1][0];
    require(s64_journal_migration_stage(&dos, &backend) == S64_SESSION_JOURNAL_OK, "folder ACTIVE owned by normal recovery");
    require(!writes && !restores && !memcmp(&original, &slots[1][0], sizeof(original)), "folder ACTIVE untouched");

    reset(); seed(0, 0, S64_SESSION_STATE_CLEAN, 7); seed(1, 1, S64_SESSION_STATE_CLEAN, 9);
    original = slots[1][1];
    require(s64_journal_migration_stage(&dos, &backend) == S64_SESSION_JOURNAL_OK, "existing folder CLEAN preserved");
    require(!writes && !memcmp(&original, &slots[1][1], sizeof(original)), "folder CLEAN unchanged");

    reset(); seed(0, 0, S64_SESSION_STATE_CLEAN, 7); seed(0, 1, S64_SESSION_STATE_CLEAN, 8);
    slots[0][1].bytes[0] ^= 0xffu;
    require(s64_journal_migration_stage(&dos, &backend) == S64_SESSION_JOURNAL_OK, "valid peer survives torn slot");

    reset(); seed(0, 0, S64_SESSION_STATE_CLEAN, 7); slots[0][0].bytes[0] ^= 0xffu;
    require(s64_journal_migration_stage(&dos, &backend) != S64_SESSION_JOURNAL_OK && !writes, "whole invalid root blocks");
    reset(); seed(1, 0, S64_SESSION_STATE_CLEAN, 7); slots[1][0].size = 23;
    require(s64_journal_migration_stage(&dos, &backend) != S64_SESSION_JOURNAL_OK && !writes, "wrong length blocks");
    reset(); seed(1, 0, S64_SESSION_STATE_CLEAN, 7); slots[1][0].size = 25;
    require(s64_journal_migration_stage(&dos, &backend) != S64_SESSION_JOURNAL_OK && !writes, "trailing bytes block");
    reset(); seed(0, 0, S64_SESSION_STATE_ACTIVE, 7); seed(0, 1, S64_SESSION_STATE_CLEAN, 7);
    require(s64_journal_migration_stage(&dos, &backend) != S64_SESSION_JOURNAL_OK && !writes && !restores,
        "equal sequence conflicting state blocks");
    reset(); seed(0, 0, S64_SESSION_STATE_ACTIVE, 0xffffffffUL);
    require(s64_journal_migration_stage(&dos, &backend) != S64_SESSION_JOURNAL_OK && !writes,
        "sequence wrap refuses CLEAN update");
    reset(); seed(0, 0, S64_SESSION_STATE_ACTIVE, 7); seed(1, 1, S64_SESSION_STATE_CLEAN, 9);
    original = slots[1][1];
    require(s64_journal_migration_stage(&dos, &backend) == S64_SESSION_JOURNAL_OK,
        "root ACTIVE resolves with existing folder CLEAN");
    require(restores == 1u && verifies == 1u && writes == 1u
        && !memcmp(&original, &slots[1][1], sizeof(original)), "folder CLEAN preserved during root recovery");

    for (opcode = 0; opcode < sizeof(fault_ops); ++opcode) {
        reset(); seed(0, 0, S64_SESSION_STATE_ACTIVE, 7);
        fault_opcode = fault_ops[opcode]; fault_dir = 0;
        require(s64_journal_migration_stage(&dos, &backend) != S64_SESSION_JOURNAL_OK, "read/path/stat/close ambiguity blocks");
        require(!writes && !restores, "ambiguous read no side effects");
    }

    for (opcode = 0; opcode < sizeof(fault_ops); ++opcode) {
        reset(); seed(0, 0, S64_SESSION_STATE_ACTIVE, 7);
        seed(1, 0, S64_SESSION_STATE_CLEAN, 9);
        fault_opcode = fault_ops[opcode]; fault_dir = 1;
        require(s64_journal_migration_stage(&dos, &backend)
            != S64_SESSION_JOURNAL_OK, "folder ambiguity cannot be hidden by root");
        require(!writes && !restores, "ambiguous folder blocks root restoration");
    }
    reset(); seed(0, 0, S64_SESSION_STATE_ACTIVE, 7);
    seed(1, 0, S64_SESSION_STATE_CLEAN, 9);
    slots[1][0].bytes[0] ^= 0xffu;
    require(s64_journal_migration_stage(&dos, &backend)
        == S64_SESSION_JOURNAL_INVALID && !writes && !restores,
        "whole invalid folder blocks root restoration");

    reset(); seed(0, 0, S64_SESSION_STATE_ACTIVE, 7); restore_ok = 0;
    require(s64_journal_migration_stage(&dos, &backend) != S64_SESSION_JOURNAL_OK && !writes, "failed restore blocks CLEAN");
    reset(); seed(0, 0, S64_SESSION_STATE_ACTIVE, 7); verify_ok = 0;
    require(s64_journal_migration_stage(&dos, &backend) != S64_SESSION_JOURNAL_OK && !writes, "failed verify blocks CLEAN");
    reset(); seed(0, 0, S64_SESSION_STATE_ACTIVE, 7); backend.verify = 0;
    require(s64_journal_migration_stage(&dos, &backend) != S64_SESSION_JOURNAL_OK && !writes, "mandatory verifier absent blocks");
    backend.verify = verify;
    reset(); seed(0, 0, S64_SESSION_STATE_ACTIVE, 7);
    require(s64_journal_migration_stage(&dos, 0) != S64_SESSION_JOURNAL_OK && !writes, "absent backend blocks");

    for (opcode = 0; opcode < sizeof(write_fault_ops); ++opcode) {
        reset(); seed(0, 0, S64_SESSION_STATE_ACTIVE, 7);
        original = slots[0][0]; fault_opcode = write_fault_ops[opcode]; fault_dir = 0; fault_slot = 1;
        require(s64_journal_migration_stage(&dos, &backend) != S64_SESSION_JOURNAL_OK,
            "root CLEAN write/close/readback fails closed");
        require(!slots[1][0].exists && !slots[1][1].exists
            && !memcmp(&original, &slots[0][0], sizeof(original)), "write failure keeps ACTIVE and no folder");
    }
    reset(); seed(0, 0, S64_SESSION_STATE_ACTIVE, 7);
    corrupt_write = 1; fault_dir = 0; fault_slot = 1;
    require(s64_journal_migration_stage(&dos, &backend) != S64_SESSION_JOURNAL_OK && !slots[1][0].exists, "root CLEAN corrupt readback blocks folder");

    reset(); seed(0, 0, S64_SESSION_STATE_ACTIVE, 7);
    fault_opcode = 0x02u; fault_dir = 1;
    require(s64_journal_migration_stage(&dos, &backend) != S64_SESSION_JOURNAL_OK, "failed folder write reports failure");
    require(decoded(0, 1).state == S64_SESSION_STATE_CLEAN, "durable root CLEAN survives folder failure");
    fault_opcode = 0;
    require(s64_journal_migration_stage(&dos, &backend) == S64_SESSION_JOURNAL_OK, "retry after folder write interruption");
    require(restores == 1u && verifies == 1u, "retry does not repeat restoration");
    reset(); seed(0, 0, S64_SESSION_STATE_ACTIVE, 7);
    original = slots[0][0];
    fault_opcode = 0x05u; fault_dir = 1;
    require(s64_journal_migration_stage(&dos, &backend) != S64_SESSION_JOURNAL_OK,
        "partial folder write reports failure");
    fault_opcode = 0;
    require(!memcmp(&original, &slots[0][0], sizeof(original))
        && decoded(0, 1).state == S64_SESSION_STATE_CLEAN
        && slots[1][1].exists && slots[1][1].size == 0,
        "failed destination and original journals retained for diagnosis");
    require(s64_journal_migration_stage(&dos, &backend) != S64_SESSION_JOURNAL_OK,
        "invalid partially written folder requires explicit recovery");
    puts("journal migration tests passed");
    return 0;
}
