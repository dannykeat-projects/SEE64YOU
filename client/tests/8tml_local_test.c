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
static unsigned char fail_rename;
static unsigned char stat_count;
static unsigned char appear_on_second_stat;

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
        if (appear_on_second_stat && ++stat_count == 2u) file_exists = 1u;
        reply_status(
            reply,
            file_exists && parts[1].length == strlen(file_name) + 1u
                && memcmp(parts[1].data, file_name, parts[1].length) == 0
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
        if (fail_rename) {
            reply_status(reply, "87,ACCESS DENIED");
            return S64_UCI_OK;
        }
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

/* Publisher receives source from disk/network rather than creating a header. */
static void prepare_empty_source(S64Buffer* body)
{
#ifdef S64_INDEPENDENT_PUBLISHER
    insert_text(body, "---\n8tml: 1\ntitle: Untitled\n---\n");
#else
    (void)body;
#endif
}

int main(void)
{
    static const char empty[] = "---\n8tml: 1\ntitle: Untitled\n---\n";
    static const char source[] =
        "---\n8tml: 1\n# preserve me\ntitle: Original\nX-future:  mixed : value  \n"
        "style: custom.8ss\n---\n<unknown>Literal</unknown>\n:::future data\nopaque\n:::\n";
    S64Dos dos;
    S64Buffer body;
    S64Document document;
    S64LocalDocument local;
    S64LocalResult result;
    unsigned int saved_length;
    unsigned long saved_checksum;
    unsigned char recovery_exists;
    char boundary[S64_LOCAL_FILENAME_CAPACITY];
    unsigned int stem_length = S64_LOCAL_FILENAME_CAPACITY - sizeof(".8tml");

    s64_dos_init(&dos, mock_execute, S64_DOS_TARGET_PRIMARY);
    s64_buffer_init(&body);
    s64_document_init(&document, &body);
    prepare_empty_source(&body);
    s64_local_init(&local, &dos, &document);
    require_body(&body, empty, "new document contains editable 8TML header");
    file_exists = 0u;
    require(s64_local_save_as(&local, "Empty", 0u) == S64_LOCAL_OK,
        "empty document saves");
    require(file_length == strlen(empty) && memcmp(file_data, empty, file_length) == 0,
        "empty save writes exact canonical source");
    require(strcmp(file_name, "Empty.8tml") == 0, "default extension is 8tml");

    set_file("source.8tml", source);
    require(s64_local_open(&local, "source.8tml", 1u) == S64_LOCAL_OK, "source opens");
    require_body(&body, source, "open preserves metadata and unknown markup");
    require(close_count >= 2u, "disk handles closed");
    require(s64_local_open(&local, "missing.8tml", 1u) == S64_LOCAL_IO_ERROR,
        "missing source reports IO error");
    require(!s64_local_document_replaced(&local), "missing source does not replace document");
    require_body(&body, source, "missing source preserves complete document");
    require(s64_local_save(&local) == S64_LOCAL_OK, "unchanged source saves");
    require(file_length == strlen(source) && memcmp(file_data, source, file_length) == 0,
        "save does not reconstruct metadata or markup");
    require(s64_buffer_move_cursor(&body, s64_buffer_length(&body)), "move to source end");
    insert_text(&body, "Edited body\n");
    require(s64_local_save(&local) == S64_LOCAL_OK, "edited source saves");
    require(s64_local_open(&local, "source.8tml", 1u) == S64_LOCAL_OK, "edited source reopens");
    require_body(&body,
        "---\n8tml: 1\n# preserve me\ntitle: Original\nX-future:  mixed : value  \n"
        "style: custom.8ss\n---\n<unknown>Literal</unknown>\n:::future data\nopaque\n:::\nEdited body\n",
        "edit and reopen preserves all original source");
    saved_length = file_length;
    saved_checksum = file_checksum();
    require(s64_local_save_as(&local, "source.8tml", 0u) == S64_LOCAL_EXISTS,
        "overwrite requires approval");
    require(!temporary_exists && file_length == saved_length && file_checksum() == saved_checksum,
        "overwrite refusal preserves disk");
    file_exists = 0u;
    stat_count = 0u;
    appear_on_second_stat = 1u;
    require(s64_local_save_as(&local, "source.8tml", 0u) == S64_LOCAL_EXISTS,
        "late destination appearance refuses overwrite");
    require(file_exists && !temporary_exists && file_checksum() == saved_checksum,
        "late collision preserves original without temporary output");
    appear_on_second_stat = 0u;
    fail_write = 1u;
    require(s64_local_save(&local) == S64_LOCAL_IO_ERROR, "write failure reported");
    require(!temporary_exists && file_length == saved_length && file_checksum() == saved_checksum,
        "write failure preserves original and removes temp");
    fail_write = 0u;
    fail_rename = 1u;
    require(s64_local_save(&local) == S64_LOCAL_IO_ERROR, "rename failure reported");
    require(temporary_exists && open_mode == 0u, "rename failure retains closed recoverable temp");
    fail_rename = 0u;
    strcpy(file_name, temporary_name);
    memcpy(file_data, temporary_data, temporary_length);
    file_length = temporary_length;
    file_exists = 1u;
    temporary_exists = 0u;
    require(s64_local_open(&local, file_name, 1u) == S64_LOCAL_OK,
        "retained temporary source reopens");
    require(s64_buffer_length(&body) == saved_length, "retained temp preserves full source");

    set_file("invalid.8tml", "---\n8tml: 2\n---\nwrong version\n");
    require(s64_local_open(&local, "invalid.8tml", 1u) == S64_LOCAL_INVALID_DOCUMENT,
        "invalid source rejected");
    require(s64_local_document_replaced(&local) && !s64_local_has_filename(&local),
        "invalid streamed source clears identity");
    require(s64_local_new(&local, 1u) == S64_LOCAL_OK, "new source succeeds");
    prepare_empty_source(&body);
    require_body(&body, empty, "new source restores canonical header");
    require(s64_buffer_move_cursor(&body, s64_buffer_length(&body)), "move past new header");
    insert_text(&body, "Recovery <future>source</future>\n");
    require(s64_local_new(&local, 0u) == S64_LOCAL_DIRTY, "dirty new requires discard");
    require(s64_local_save_recovery(&local) == S64_LOCAL_OK, "recovery saves");
    require(!s64_local_has_filename(&local), "recovery does not claim filename");
    recovery_exists = 0u;
    require(s64_local_recovery_exists(&local, &recovery_exists) == S64_LOCAL_OK && recovery_exists,
        "recovery discoverable");
    require(s64_local_new(&local, 1u) == S64_LOCAL_OK, "clear before recovery");
    require(s64_local_open_recovery(&local) == S64_LOCAL_OK, "recovery opens");
    require_body(&body, "---\n8tml: 1\ntitle: Untitled\n---\nRecovery <future>source</future>\n",
        "recovery preserves complete source");
    require(!s64_local_has_filename(&local), "recovered source requires Save As");
    require(s64_local_delete_recovery(&local) == S64_LOCAL_OK, "recovery deletes");
    require(s64_local_save_as(&local, S64_LOCAL_RECOVERY_FILENAME, 1u) == S64_LOCAL_INVALID_ARGUMENT,
        "reserved recovery filename refused");

    file_exists = 0u;
    require(s64_local_save_as(&local, "UPPER.8TML", 0u) == S64_LOCAL_OK
        && strcmp(file_name, "UPPER.8TML") == 0, "uppercase suffix accepted exactly");
    require(s64_local_new(&local, 1u) == S64_LOCAL_OK, "new before filename tests");
    prepare_empty_source(&body);
    file_exists = 0u;
    require(s64_local_save_as(&local, "wrong.txt", 0u) == S64_LOCAL_INVALID_ARGUMENT,
        "unrelated explicit extension rejected");
    require(!temporary_exists && !file_exists && !s64_local_has_filename(&local),
        "invalid extension causes no writes or identity change");
    memset(boundary, 'x', stem_length);
    boundary[stem_length] = '\0';
    require(s64_local_save_as(&local, boundary, 0u) == S64_LOCAL_OK
        && strlen(file_name) == S64_LOCAL_FILENAME_CAPACITY - 1u,
        "longest extensionless basename fits exact capacity");
    require(s64_local_new(&local, 1u) == S64_LOCAL_OK, "new before overflow");
    prepare_empty_source(&body);
    file_exists = 0u;
    boundary[stem_length] = 'x';
    boundary[stem_length + 1u] = '\0';
    result = s64_local_save_as(&local, boundary, 0u);
    require(result == S64_LOCAL_INVALID_ARGUMENT && !file_exists && !temporary_exists,
        "extension overflow rejected before disk writes");
    require(s64_local_save_as(&local, "archive.v1/New", 0u) == S64_LOCAL_OK
        && strcmp(file_name, "archive.v1/New.8tml") == 0,
        "directory dot does not suppress suffix");
    puts("8TML local document tests passed");
    return 0;
}
