#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "document.h"
#include "platform/dos.h"
#include "storage/local.h"

#define FILE_CAPACITY 10000u

static unsigned char file_data[FILE_CAPACITY];
static unsigned int file_length;
static char file_name[S64_LOCAL_FILENAME_CAPACITY];
static unsigned char file_exists;
static unsigned char temporary_data[FILE_CAPACITY];
static unsigned int temporary_length;
static char temporary_name[S64_LOCAL_FILENAME_CAPACITY + 4u];
static unsigned char temporary_exists;
static unsigned char open_mode;
static unsigned int read_offset;
static unsigned int close_count;
static unsigned char fail_write;

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void reply_status(S64UciReply* reply, const char* status)
{
    size_t length = strlen(status);
    size_t copy = length;

    reply->status_length = (uint16_t)length;
    if (reply->status == NULL || reply->status_capacity == 0u) return;
    if (copy >= reply->status_capacity) copy = reply->status_capacity - 1u;
    memcpy(reply->status, status, copy);
    reply->status[copy] = '\0';
}

static int part_equals(const S64UciCommandPart* part, const char* value)
{
    size_t length = strlen(value);
    return part->length == length && memcmp(part->data, value, length) == 0;
}

static void copy_part(char* destination, size_t capacity, const S64UciCommandPart* part)
{
    require(part->length + 1u <= capacity, "mock filename capacity");
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
    uint16_t requested;
    unsigned int available;
    uint16_t delivered;
    const unsigned char* separator;
    S64UciResult result;

    require(parts[0].data[0] == S64_DOS_TARGET_PRIMARY, "DOS target");

    if (opcode == 0x08u) {
        require(part_count == 2u, "stat parts");
        reply_status(
            reply,
            file_exists && part_equals(&parts[1], file_name)
                ? "00,OK"
                : "88,FILE NOT FOUND"
        );
        return S64_UCI_OK;
    }

    if (opcode == 0x02u) {
        require(part_count == 2u, "open parts");
        if (parts[0].data[2] == 0x01u) {
            if (!file_exists || !part_equals(&parts[1], file_name)) {
                reply_status(reply, "88,FILE NOT FOUND");
                return S64_UCI_OK;
            }
            open_mode = 1u;
            read_offset = 0u;
        } else {
            copy_part(temporary_name, sizeof(temporary_name), &parts[1]);
            temporary_length = 0u;
            temporary_exists = 1u;
            open_mode = 2u;
        }
        reply_status(reply, "00,OK");
        return S64_UCI_OK;
    }

    if (opcode == 0x04u) {
        require(open_mode == 1u, "read file is open");
        requested = (uint16_t)(parts[0].data[2] | ((uint16_t)parts[0].data[3] << 8));
        available = file_length - read_offset;
        delivered = requested < available ? requested : (uint16_t)available;
        if (delivered != 0u && reply->data_sink != NULL) {
            result = reply->data_sink(
                file_data + read_offset,
                delivered,
                reply->data_context
            ) ? S64_UCI_OK : S64_UCI_CONSUMER_ABORTED;
            if (result != S64_UCI_OK) return result;
        }
        read_offset += delivered;
        reply_status(reply, "00,OK");
        return S64_UCI_OK;
    }

    if (opcode == 0x05u) {
        require(open_mode == 2u && part_count == 2u, "write temp is open");
        if (fail_write) {
            reply_status(reply, "87,ACCESS DENIED");
            return S64_UCI_OK;
        }
        require(temporary_length + parts[1].length <= sizeof(temporary_data), "temp capacity");
        memcpy(temporary_data + temporary_length, parts[1].data, parts[1].length);
        temporary_length += parts[1].length;
        reply_status(reply, "00,OK");
        return S64_UCI_OK;
    }

    if (opcode == 0x03u) {
        open_mode = 0u;
        ++close_count;
        reply_status(reply, "00,OK");
        return S64_UCI_OK;
    }

    if (opcode == 0x09u) {
        if (file_exists && part_equals(&parts[1], file_name)) file_exists = 0u;
        if (temporary_exists && part_equals(&parts[1], temporary_name)) temporary_exists = 0u;
        reply_status(reply, "00,OK");
        return S64_UCI_OK;
    }

    if (opcode == 0x0au) {
        separator = parts[2].data;
        require(part_count == 4u && parts[2].length == 1u && separator[0] == 0u, "rename shape");
        require(temporary_exists && part_equals(&parts[1], temporary_name), "rename temp source");
        copy_part(file_name, sizeof(file_name), &parts[3]);
        memcpy(file_data, temporary_data, temporary_length);
        file_length = temporary_length;
        file_exists = 1u;
        temporary_exists = 0u;
        reply_status(reply, "00,OK");
        return S64_UCI_OK;
    }

    reply_status(reply, "99,UNKNOWN");
    return S64_UCI_OK;
}

static void set_file(const char* name, const char* contents)
{
    strcpy(file_name, name);
    file_length = (unsigned int)strlen(contents);
    memcpy(file_data, contents, file_length);
    file_exists = 1u;
}

static void insert_text(S64Buffer* buffer, const char* text)
{
    while (*text != '\0') {
        require(s64_buffer_insert(buffer, (unsigned char)*text), "insert fixture text");
        ++text;
    }
}

static void require_body(const S64Buffer* buffer, const char* expected, const char* message)
{
    unsigned int i;
    size_t length = strlen(expected);
    require(s64_buffer_length(buffer) == length, message);
    for (i = 0u; i < length; ++i) {
        require(s64_buffer_char_at(buffer, i) == (unsigned char)expected[i], message);
    }
}

static unsigned long file_checksum(void)
{
    unsigned long checksum = 0u;
    unsigned int i;
    for (i = 0u; i < file_length; ++i) checksum = checksum * 33u + file_data[i];
    return checksum;
}

int main(void)
{
    static const char opened_wire[] =
        "SEE64YOU/1\nType: post\nSlug: opened\nTitle: Opened\n"
        "Status: draft\nDate: \nOrder: 0\n\nOpened body";
    S64Dos dos;
    S64Buffer body;
    S64Document document;
    S64LocalDocument local;
    S64LocalResult result;
    unsigned int saved_length;
    unsigned long saved_checksum;

    s64_dos_init(&dos, mock_execute, S64_DOS_TARGET_PRIMARY);
    s64_buffer_init(&body);
    s64_document_init(&document, &body);
    s64_local_init(&local, &dos, &document);

    set_file("opened.s64", opened_wire);
    result = s64_local_open(&local, "opened.s64", 1u);
    require(result == S64_LOCAL_OK, "open succeeds");
    require(strcmp(local.filename, "opened.s64") == 0, "open remembers filename");
    require(document.type == S64_DOCUMENT_POST, "open metadata");
    require_body(&body, "Opened body", "open streams exact body");
    require(close_count == 1u, "open closes file");

    require(
        s64_local_open(&local, "missing.s64", 1u) == S64_LOCAL_IO_ERROR,
        "missing open maps to file error"
    );
    require(!s64_local_document_replaced(&local), "pre-open failure reports no replacement");
    require_body(&body, "Opened body", "failed pre-open preserves document");

    set_file("broken.s64", "SEE64YOU/1\nType: post\n\npartial");
    result = s64_local_open(&local, "broken.s64", 1u);
    require(result == S64_LOCAL_INVALID_DOCUMENT, "invalid wire rejected");
    require(s64_local_document_replaced(&local), "parsed failure reports cleared document");
    require(s64_buffer_length(&body) == 0u, "failed parsed open clears partial body");
    require(!s64_local_has_filename(&local), "failed parsed open clears identity");

    s64_buffer_init(&body);
    s64_document_init(&document, &body);
    insert_text(&body, "New local body");
    file_exists = 0u;
    result = s64_local_save_as(&local, "My Note.s64", 0u);
    require(result == S64_LOCAL_OK, "save as succeeds");
    require(strcmp(local.filename, "My Note.s64") == 0, "save as remembers filename");
    require(strcmp(document.title, "My Note") == 0, "save as derives title");
    require(strcmp(document.slug, "my-note") == 0, "save as derives slug");
    require(file_exists && file_length >= sizeof("SEE64YOU/1\n") - 1u
        && memcmp(file_data, "SEE64YOU/1\n", sizeof("SEE64YOU/1\n") - 1u) == 0,
        "save writes canonical wire header");
    require(file_length >= strlen("New local body")
        && memcmp(file_data + file_length - strlen("New local body"),
            "New local body", strlen("New local body")) == 0,
        "save writes exact body");

    result = s64_local_save_as(&local, "My Note.s64", 0u);
    require(result == S64_LOCAL_EXISTS, "save as detects overwrite");
    require(!temporary_exists, "overwrite refusal creates no temp file");

    insert_text(&body, " updated");
    result = s64_local_save(&local);
    require(result == S64_LOCAL_OK, "save updates current file");
    require(file_exists && !temporary_exists, "save replaces through temp file");

    saved_length = file_length;
    saved_checksum = file_checksum();
    fail_write = 1u;
    insert_text(&body, " failed");
    result = s64_local_save(&local);
    require(result == S64_LOCAL_IO_ERROR, "write failure reported");
    require(!temporary_exists, "write failure removes temp file");
    require(file_length == saved_length && file_checksum() == saved_checksum,
        "write failure preserves old file");
    fail_write = 0u;

    require(s64_local_new(&local, 0u) == S64_LOCAL_DIRTY, "new requires discard approval");
    require(s64_local_new(&local, 1u) == S64_LOCAL_OK, "approved new succeeds");
    require(s64_buffer_length(&body) == 0u && !s64_local_has_filename(&local),
        "new clears document and identity");

    puts("local document tests passed");
    return 0;
}
