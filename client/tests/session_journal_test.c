#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/session_journal.h"

typedef struct {
    uint8_t bytes[S64_SESSION_JOURNAL_WIRE_SIZE];
    uint8_t exists;
    uint8_t corrupt_on_read;
} Slot;

static Slot slots[2];
static uint8_t open_slot;
static uint8_t open_mode;
static uint16_t read_offset;

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static int slot_from_part(const S64UciCommandPart* part)
{
    const char* filename;

    if (part == 0) return -1;
    filename = (const char*)part->data;
    if (part->length == strlen(S64_SESSION_JOURNAL_FILENAME_0)
        && memcmp(filename, S64_SESSION_JOURNAL_FILENAME_0, part->length) == 0) {
        return 0;
    }
    if (part->length == strlen(S64_SESSION_JOURNAL_FILENAME_1)
        && memcmp(filename, S64_SESSION_JOURNAL_FILENAME_1, part->length) == 0) {
        return 1;
    }
    if (part->length == strlen(S64_SESSION_JOURNAL_FILENAME_0) + 1u
        && memcmp(filename, S64_SESSION_JOURNAL_FILENAME_0,
            part->length - 1u) == 0) {
        return 0;
    }
    if (part->length == strlen(S64_SESSION_JOURNAL_FILENAME_1) + 1u
        && memcmp(filename, S64_SESSION_JOURNAL_FILENAME_1,
            part->length - 1u) == 0) {
        return 1;
    }
    return -1;
}

static void set_status(S64UciReply* reply, const char* status)
{
    size_t length = strlen(status);
    size_t copy = length;

    reply->status_length = (uint16_t)length;
    if (reply->status == 0 || reply->status_capacity == 0u) return;
    if (copy >= reply->status_capacity) copy = reply->status_capacity - 1u;
    memcpy(reply->status, status, copy);
    reply->status[copy] = '\0';
}

static S64UciResult mock_execute(
    const S64UciCommandPart* parts,
    uint8_t part_count,
    S64UciReply* reply
)
{
    uint8_t opcode;
    int slot;

    if (part_count == 0u || parts[0].length < 2u
        || parts[0].data[0] != S64_DOS_TARGET_PRIMARY) {
        return S64_UCI_PROTOCOL_ERROR;
    }

    opcode = parts[0].data[1];

    if (opcode == 0x11u) {
        set_status(reply,
            part_count == 2u
                && parts[1].length == strlen(S64_SESSION_JOURNAL_STORAGE_PATH)
                && memcmp(parts[1].data, S64_SESSION_JOURNAL_STORAGE_PATH,
                    parts[1].length) == 0
            ? "00,OK" : "87,ACCESS DENIED");
        return S64_UCI_OK;
    }

    if (opcode == 0x08u) {
        slot = part_count == 2u ? slot_from_part(&parts[1]) : -1;
        set_status(reply, slot >= 0 && slots[slot].exists
            ? "00,OK" : "88,FILE NOT FOUND");
        return S64_UCI_OK;
    }

    if (opcode == 0x02u) {
        slot = part_count == 2u ? slot_from_part(&parts[1]) : -1;
        if (slot < 0) return S64_UCI_PROTOCOL_ERROR;

        if (parts[0].data[2] == 0x01u) {
            if (!slots[slot].exists) {
                set_status(reply, "88,FILE NOT FOUND");
                return S64_UCI_OK;
            }
            open_mode = 1u;
            open_slot = (uint8_t)slot;
            read_offset = 0u;
        } else if (parts[0].data[2] == 0x0eu) {
            open_mode = 2u;
            open_slot = (uint8_t)slot;
            slots[slot].exists = 1u;
            memset(slots[slot].bytes, 0, sizeof(slots[slot].bytes));
        } else {
            return S64_UCI_PROTOCOL_ERROR;
        }
        set_status(reply, "00,OK");
        return S64_UCI_OK;
    }

    if (opcode == 0x04u) {
        uint16_t requested;
        uint16_t available;
        uint16_t delivered;
        uint8_t scratch[S64_SESSION_JOURNAL_WIRE_SIZE];

        if (open_mode != 1u || part_count != 1u) {
            return S64_UCI_PROTOCOL_ERROR;
        }
        requested = (uint16_t)(
            parts[0].data[2] | ((uint16_t)parts[0].data[3] << 8)
        );
        available = (uint16_t)(S64_SESSION_JOURNAL_WIRE_SIZE - read_offset);
        delivered = requested < available ? requested : available;
        memcpy(scratch, slots[open_slot].bytes + read_offset, delivered);
        if (slots[open_slot].corrupt_on_read && delivered != 0u) {
            scratch[0] ^= 0xffu;
        }
        if (delivered != 0u && reply->data_sink != 0
            && !reply->data_sink(scratch, delivered, reply->data_context)) {
            return S64_UCI_CONSUMER_ABORTED;
        }
        read_offset = (uint16_t)(read_offset + delivered);
        set_status(reply, "00,OK");
        return S64_UCI_OK;
    }

    if (opcode == 0x05u) {
        if (open_mode != 2u || part_count != 2u
            || parts[1].length != S64_SESSION_JOURNAL_WIRE_SIZE) {
            return S64_UCI_PROTOCOL_ERROR;
        }
        memcpy(slots[open_slot].bytes, parts[1].data, parts[1].length);
        set_status(reply, "00,OK");
        return S64_UCI_OK;
    }

    if (opcode == 0x03u) {
        open_mode = 0u;
        set_status(reply, "00,OK");
        return S64_UCI_OK;
    }

    set_status(reply, "99,UNKNOWN");
    return S64_UCI_OK;
}

int main(void)
{
    S64SessionJournal journal;
    S64SessionJournal decoded;
    S64SessionJournal loaded;
    S64SystemSnapshot snapshot;
    S64SystemSnapshot restored;
    S64Dos dos;
    uint8_t wire[S64_SESSION_JOURNAL_WIRE_SIZE];
    uint8_t corrupt[S64_SESSION_JOURNAL_WIRE_SIZE];

    memset(slots, 0, sizeof(slots));
    memset(&snapshot, 0, sizeof(snapshot));
    snapshot.turbo_control = 0x25u;
    snapshot.turbo_control_valid = 1u;

    require(
        s64_session_journal_prepare(
            &journal, 0x12345678UL, 0x0201u, 0xabcdef01UL, &snapshot
        ),
        "prepare active journal"
    );
    require(s64_session_journal_is_unclean(&journal), "new journal is active");
    require(journal.sequence == 0UL, "sequence assigned only on durable save");
    require(
        (journal.restore_flags & S64_SESSION_JOURNAL_RESTORE_TURBO) != 0u,
        "captured turbo explicitly marked restorable"
    );

    require(s64_session_journal_encode(&journal, wire), "encode journal");
    require(s64_session_journal_decode(&decoded, wire), "decode journal");
    require(decoded.session_id == 0x12345678UL, "session id round trips");
    require(decoded.package_version == 0x0201u, "package version round trips");
    require(decoded.recovery_ref == 0xabcdef01UL, "recovery ref round trips");
    require(
        s64_session_journal_restore_snapshot(&decoded, &restored),
        "restore snapshot extracted"
    );
    require(restored.turbo_control_valid && restored.turbo_control == 0x25u,
        "only captured turbo state restored");

    memcpy(corrupt, wire, sizeof(corrupt));
    corrupt[9] ^= 0x40u;
    require(!s64_session_journal_decode(&decoded, corrupt),
        "checksum rejects corruption");

    memcpy(corrupt, wire, sizeof(corrupt));
    corrupt[20] |= 0x80u;
    /* Repair checksum indirectly by decoding must still fail unknown flags. */
    {
        S64SessionJournal bad = journal;
        bad.restore_flags = 0x80u;
        require(!s64_session_journal_encode(&bad, corrupt),
            "unknown restore flags cannot be encoded");
    }

    s64_dos_init(&dos, mock_execute, S64_DOS_TARGET_PRIMARY);
    require(
        s64_session_journal_store_load(&dos, &loaded)
            == S64_SESSION_JOURNAL_NOT_FOUND,
        "empty two-slot store reports not found"
    );

    require(
        s64_session_journal_store_save(&dos, &journal)
            == S64_SESSION_JOURNAL_OK,
        "first active journal save succeeds"
    );
    require(journal.sequence == 1UL && slots[1].exists,
        "first save uses sequence one and alternate slot");
    require(
        s64_session_journal_store_load(&dos, &loaded)
            == S64_SESSION_JOURNAL_OK,
        "saved active journal loads"
    );
    require(loaded.sequence == 1UL && s64_session_journal_is_unclean(&loaded),
        "loaded active record selected");

    require(s64_session_journal_mark_clean(&journal), "mark clean");
    require(
        s64_session_journal_store_save(&dos, &journal)
            == S64_SESSION_JOURNAL_OK,
        "clean record save succeeds"
    );
    require(journal.sequence == 2UL && slots[0].exists,
        "clean update alternates slot");
    require(
        s64_session_journal_store_load(&dos, &loaded)
            == S64_SESSION_JOURNAL_OK,
        "latest clean record loads"
    );
    require(loaded.sequence == 2UL && !s64_session_journal_is_unclean(&loaded)
        && loaded.state == S64_SESSION_STATE_CLEAN,
        "newest clean record supersedes older active slot");

    /*
     * Simulate damage/power loss in the newest slot. The prior valid ACTIVE
     * record remains readable, yielding a safe false-positive recovery rather
     * than losing the captured state.
     */
    slots[0].bytes[3] ^= 0x80u;
    require(
        s64_session_journal_store_load(&dos, &loaded)
            == S64_SESSION_JOURNAL_OK,
        "corrupt newest slot falls back to older valid slot"
    );
    require(loaded.sequence == 1UL && s64_session_journal_is_unclean(&loaded),
        "fallback preserves restorable active record");

    slots[0].bytes[3] ^= 0x80u;
    slots[1].corrupt_on_read = 1u;
    require(
        s64_session_journal_store_load(&dos, &loaded)
            == S64_SESSION_JOURNAL_OK,
        "one unreadable/corrupt slot does not defeat valid peer"
    );
    require(loaded.sequence == 2UL && loaded.state == S64_SESSION_STATE_CLEAN,
        "valid peer remains authoritative");
    slots[1].corrupt_on_read = 0u;

    {
        S64SessionJournal no_snapshot;
        S64SystemSnapshot empty;
        require(
            s64_session_journal_prepare(
                &no_snapshot, 7UL, 1u, 0UL, 0
            ),
            "journal may contain no restorable settings"
        );
        require(
            s64_session_journal_restore_snapshot(&no_snapshot, &empty),
            "empty restorable snapshot decodes safely"
        );
        require(!empty.turbo_control_valid,
            "uncaptured turbo is never invented as restorable");
    }

    puts("session journal tests passed");
    return 0;
}
