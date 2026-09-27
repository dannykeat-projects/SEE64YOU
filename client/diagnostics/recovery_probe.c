#include <stdint.h>
#include <string.h>

#include "buffer.h"
#include "document.h"
#include "platform/dos.h"
#include "storage/local.h"

#define DIAG_PROGRESS (*(volatile uint8_t*)0xC030u)
#define DIAG_RESULT   (*(volatile uint8_t*)0xC031u)
#define DIAG_DETAIL   (*(volatile uint8_t*)0xC032u)

#define DIAG_PASS 0xA5u
#define DIAG_FAIL 0x5Au
#define FILE_CAPACITY 768u

static unsigned char file_data[FILE_CAPACITY];
static unsigned int file_length;
static char file_name[S64_LOCAL_FILENAME_CAPACITY];
static unsigned char file_exists;

static unsigned char temp_data[FILE_CAPACITY];
static unsigned int temp_length;
static char temp_name[S64_LOCAL_FILENAME_CAPACITY + 4u];
static unsigned char temp_exists;

static unsigned char open_mode;
static unsigned int read_offset;

static void fail(unsigned char detail)
{
    DIAG_DETAIL = detail;
    DIAG_RESULT = DIAG_FAIL;
    for (;;) {
    }
}

static void reply_status(S64UciReply* reply, const char* status)
{
    uint16_t length = (uint16_t)strlen(status);
    uint16_t copy = length;

    reply->status_length = length;
    if (reply->status == 0 || reply->status_capacity == 0u) {
        return;
    }
    if (copy >= reply->status_capacity) {
        copy = (uint16_t)(reply->status_capacity - 1u);
    }
    memcpy(reply->status, status, copy);
    reply->status[copy] = '\0';
}

static unsigned char part_equals(
    const S64UciCommandPart* part,
    const char* text
)
{
    uint16_t length = (uint16_t)strlen(text);
    return part->length == length
        && memcmp(part->data, text, length) == 0;
}

static void copy_part(
    char* destination,
    unsigned int capacity,
    const S64UciCommandPart* part
)
{
    if ((unsigned int)part->length + 1u > capacity) {
        fail(30u);
    }
    memcpy(destination, part->data, part->length);
    destination[part->length] = '\0';
}

static S64UciResult mock_execute(
    const S64UciCommandPart* parts,
    uint8_t part_count,
    S64UciReply* reply
)
{
    uint8_t opcode = parts[0].data[1];

    if (parts[0].data[0] != S64_DOS_TARGET_PRIMARY) {
        return S64_UCI_TRANSPORT_ERROR;
    }

    if (opcode == 0x08u) {
        reply_status(
            reply,
            file_exists && part_count == 2u && part_equals(&parts[1], file_name)
                ? "00,OK"
                : "88,FILE NOT FOUND"
        );
        return S64_UCI_OK;
    }

    if (opcode == 0x02u) {
        if (part_count != 2u) {
            return S64_UCI_TRANSPORT_ERROR;
        }
        if (parts[0].data[2] == 0x01u) {
            if (!file_exists || !part_equals(&parts[1], file_name)) {
                reply_status(reply, "88,FILE NOT FOUND");
                return S64_UCI_OK;
            }
            open_mode = 1u;
            read_offset = 0u;
        } else {
            copy_part(temp_name, sizeof(temp_name), &parts[1]);
            temp_length = 0u;
            temp_exists = 1u;
            open_mode = 2u;
        }
        reply_status(reply, "00,OK");
        return S64_UCI_OK;
    }

    if (opcode == 0x04u) {
        uint16_t requested;
        unsigned int available;
        uint16_t delivered;

        if (open_mode != 1u || part_count != 1u) {
            return S64_UCI_TRANSPORT_ERROR;
        }
        requested = (uint16_t)(
            parts[0].data[2] | ((uint16_t)parts[0].data[3] << 8)
        );
        available = file_length - read_offset;
        delivered = requested < available ? requested : (uint16_t)available;

        if (delivered != 0u && reply->data_sink != 0) {
            if (!reply->data_sink(
                    file_data + read_offset,
                    delivered,
                    reply->data_context
                )) {
                return S64_UCI_CONSUMER_ABORTED;
            }
        }
        read_offset += delivered;
        reply_status(reply, "00,OK");
        return S64_UCI_OK;
    }

    if (opcode == 0x05u) {
        if (open_mode != 2u || part_count != 2u
            || temp_length + parts[1].length > FILE_CAPACITY) {
            return S64_UCI_TRANSPORT_ERROR;
        }
        memcpy(temp_data + temp_length, parts[1].data, parts[1].length);
        temp_length = (unsigned int)(temp_length + parts[1].length);
        reply_status(reply, "00,OK");
        return S64_UCI_OK;
    }

    if (opcode == 0x03u) {
        open_mode = 0u;
        reply_status(reply, "00,OK");
        return S64_UCI_OK;
    }

    if (opcode == 0x09u) {
        if (part_count != 2u) {
            return S64_UCI_TRANSPORT_ERROR;
        }
        if (file_exists && part_equals(&parts[1], file_name)) {
            file_exists = 0u;
        }
        if (temp_exists && part_equals(&parts[1], temp_name)) {
            temp_exists = 0u;
        }
        reply_status(reply, "00,OK");
        return S64_UCI_OK;
    }

    if (opcode == 0x0Au) {
        if (part_count != 4u
            || parts[2].length != 1u
            || parts[2].data[0] != 0u
            || !temp_exists
            || !part_equals(&parts[1], temp_name)) {
            return S64_UCI_TRANSPORT_ERROR;
        }
        copy_part(file_name, sizeof(file_name), &parts[3]);
        memcpy(file_data, temp_data, temp_length);
        file_length = temp_length;
        file_exists = 1u;
        temp_exists = 0u;
        reply_status(reply, "00,OK");
        return S64_UCI_OK;
    }

    reply_status(reply, "99,UNKNOWN");
    return S64_UCI_OK;
}

int main(void)
{
    static const unsigned char fixture[] = {
        'R','e','c','o','v','e','r','y',' ',
        'b','o','d','y','\n','L','i','n','e',' ','2'
    };
    S64Dos dos;
    S64Buffer body;
    S64Document document;
    S64LocalDocument local;
    S64LocalResult result;
    uint8_t exists;
    unsigned int i;

    DIAG_PROGRESS = 0x10u;
    DIAG_RESULT = 0u;
    DIAG_DETAIL = 0u;

    s64_dos_init(&dos, mock_execute, S64_DOS_TARGET_PRIMARY);
    s64_buffer_init(&body);
    s64_document_init(&document, &body);
    s64_local_init(&local, &dos, &document);

    for (i = 0u; i < sizeof(fixture); ++i) {
        if (!s64_buffer_insert(&body, fixture[i])) {
            fail(1u);
        }
    }

    DIAG_PROGRESS = 0x20u;
    result = s64_local_save_recovery(&local);
    if (result != S64_LOCAL_OK
        || !file_exists
        || strcmp(file_name, S64_LOCAL_RECOVERY_FILENAME) != 0
        || s64_local_has_filename(&local)) {
        fail(2u);
    }

    exists = 0u;
    if (s64_local_recovery_exists(&local, &exists) != S64_LOCAL_OK
        || !exists) {
        fail(3u);
    }

    DIAG_PROGRESS = 0x30u;
    if (s64_local_new(&local, 1u) != S64_LOCAL_OK
        || s64_buffer_length(&body) != 0u) {
        fail(4u);
    }

    result = s64_local_open_recovery(&local);
    if (result != S64_LOCAL_OK
        || s64_local_has_filename(&local)
        || s64_buffer_length(&body) != sizeof(fixture)) {
        fail(5u);
    }

    for (i = 0u; i < sizeof(fixture); ++i) {
        if (s64_buffer_char_at(&body, i) != fixture[i]) {
            fail(6u);
        }
    }

    DIAG_PROGRESS = 0x40u;
    if (s64_local_delete_recovery(&local) != S64_LOCAL_OK) {
        fail(7u);
    }
    exists = 1u;
    if (s64_local_recovery_exists(&local, &exists) != S64_LOCAL_OK
        || exists) {
        fail(8u);
    }

    DIAG_PROGRESS = 0x50u;
    DIAG_RESULT = DIAG_PASS;
    for (;;) {
    }
}
