#include "storage/local.h"

#include <string.h>

typedef struct {
    S64Dos* dos;
    S64DosResult result;
} S64LocalWriteSink;

static uint8_t filename_copy(char* destination, const char* filename)
{
    uint16_t length = 0u;

    if (destination == 0 || filename == 0 || filename[0] == '\0') return 0u;

    while (filename[length] != '\0') {
        if (length + 1u >= S64_LOCAL_FILENAME_CAPACITY
            || filename[length] == '\r' || filename[length] == '\n') {
            return 0u;
        }
        ++length;
    }

    memcpy(destination, filename, length + 1u);
    return 1u;
}

static void clear_document(S64LocalDocument* local)
{
    S64Buffer* body = local->document->body;
    s64_buffer_init(body);
    s64_document_init(local->document, body);
}

static uint8_t discard_sink(const uint8_t* data, uint16_t length, void* context)
{
    (void)data;
    (void)length;
    (void)context;
    return 1u;
}

static uint8_t write_sink(const uint8_t* data, uint16_t length, void* context)
{
    S64LocalWriteSink* sink = (S64LocalWriteSink*)context;
    sink->result = s64_dos_write(sink->dos, data, length);
    return sink->result == S64_DOS_OK;
}

static uint8_t default_metadata(S64Document* document, const char* filename)
{
    const char* base = filename;
    const char* cursor = filename;
    uint16_t base_length;
    uint16_t i;
    uint16_t slug_length = 0u;
    uint8_t separator = 0u;
    char title[S64_DOCUMENT_TITLE_CAPACITY];
    char slug[S64_DOCUMENT_SLUG_CAPACITY];
    char value;

    while (*cursor != '\0') {
        if (*cursor == '/' || *cursor == '\\' || *cursor == ':') base = cursor + 1;
        ++cursor;
    }

    base_length = (uint16_t)strlen(base);
    if (base_length > 4u && strcmp(base + base_length - 4u, ".s64") == 0) {
        base_length = (uint16_t)(base_length - 4u);
    }
    if (base_length == 0u || base_length >= S64_DOCUMENT_TITLE_CAPACITY) return 0u;

    memcpy(title, base, base_length);
    title[base_length] = '\0';

    for (i = 0u; i < base_length; ++i) {
        value = base[i];
        if (value >= 'A' && value <= 'Z') value = (char)(value - 'A' + 'a');

        if ((value >= 'a' && value <= 'z') || (value >= '0' && value <= '9')) {
            if (separator && slug_length != 0u) {
                if (slug_length + 1u >= S64_DOCUMENT_SLUG_CAPACITY) return 0u;
                slug[slug_length++] = '-';
            }
            separator = 0u;
            if (slug_length + 1u >= S64_DOCUMENT_SLUG_CAPACITY) return 0u;
            slug[slug_length++] = value;
        } else if (slug_length != 0u) {
            separator = 1u;
        }
    }

    if (slug_length == 0u) return 0u;
    slug[slug_length] = '\0';

    if (document->title[0] == '\0' && !s64_document_set_title(document, title)) return 0u;
    if (document->slug[0] == '\0' && !s64_document_set_slug(document, slug)) return 0u;
    return 1u;
}

static S64LocalResult write_document_file(
    S64LocalDocument* local,
    const S64Document* document,
    const char* filename,
    uint8_t overwrite
)
{
    S64DocumentResult document_result;
    S64LocalWriteSink sink;
    S64DosResult result;
    S64DosResult close_result;
    uint8_t exists;
    char temporary[S64_LOCAL_FILENAME_CAPACITY + 4u];

    result = s64_dos_file_exists(local->dos, filename, &exists);
    if (result != S64_DOS_OK) {
        local->dos_result = result;
        return S64_LOCAL_IO_ERROR;
    }
    if (exists && !overwrite) return S64_LOCAL_EXISTS;

    document_result = s64_document_write_wire(document, discard_sink, 0);
    if (document_result != S64_DOCUMENT_OK) {
        return S64_LOCAL_INVALID_DOCUMENT;
    }

    strcpy(temporary, filename);
    strcat(temporary, ".tmp");
    result = s64_dos_open_write(local->dos, temporary);
    if (result != S64_DOS_OK) {
        local->dos_result = result;
        return S64_LOCAL_IO_ERROR;
    }

    sink.dos = local->dos;
    sink.result = S64_DOS_OK;
    document_result = s64_document_write_wire(document, write_sink, &sink);
    close_result = s64_dos_close(local->dos);
    if (document_result != S64_DOCUMENT_OK || close_result != S64_DOS_OK) {
        local->dos_result = document_result != S64_DOCUMENT_OK
            ? sink.result
            : close_result;
        (void)s64_dos_delete_file(local->dos, temporary);
        return document_result == S64_DOCUMENT_INVALID_METADATA
            ? S64_LOCAL_INVALID_DOCUMENT
            : S64_LOCAL_IO_ERROR;
    }

    if (exists) {
        result = s64_dos_delete_file(local->dos, filename);
        if (result != S64_DOS_OK) {
            local->dos_result = result;
            (void)s64_dos_delete_file(local->dos, temporary);
            return S64_LOCAL_IO_ERROR;
        }
    }

    result = s64_dos_rename_file(local->dos, temporary, filename);
    if (result != S64_DOS_OK) {
        local->dos_result = result;
        (void)s64_dos_delete_file(local->dos, temporary);
        return S64_LOCAL_IO_ERROR;
    }

    return S64_LOCAL_OK;
}

void s64_local_init(S64LocalDocument* local, S64Dos* dos, S64Document* document)
{
    if (local == 0) return;
    local->dos = dos;
    local->document = document;
    local->filename[0] = '\0';
    local->dos_result = S64_DOS_OK;
    local->document_replaced = 0u;
}

const char* s64_local_result_message(const S64LocalDocument* local, S64LocalResult result)
{
    switch (result) {
        case S64_LOCAL_OK: return "OK";
        case S64_LOCAL_INVALID_ARGUMENT: return "Invalid file request";
        case S64_LOCAL_DIRTY: return "Document has unsaved changes";
        case S64_LOCAL_NO_FILENAME: return "Use Save As first";
        case S64_LOCAL_EXISTS: return "File already exists";
        case S64_LOCAL_INVALID_DOCUMENT: return "Document metadata invalid";
        case S64_LOCAL_DOCUMENT_TOO_LARGE: return "Document too large";
        case S64_LOCAL_IO_ERROR:
        default:
            return local == 0 ? "File error" : s64_dos_result_message(local->dos_result);
    }
}

uint8_t s64_local_has_filename(const S64LocalDocument* local)
{
    return local != 0 && local->filename[0] != '\0';
}

uint8_t s64_local_document_replaced(const S64LocalDocument* local)
{
    return local != 0 && local->document_replaced;
}

S64LocalResult s64_local_new(S64LocalDocument* local, uint8_t discard_dirty)
{
    if (local == 0 || local->document == 0 || local->document->body == 0) {
        return S64_LOCAL_INVALID_ARGUMENT;
    }
    if (!discard_dirty) return S64_LOCAL_DIRTY;

    clear_document(local);
    local->filename[0] = '\0';
    local->document_replaced = 1u;
    return S64_LOCAL_OK;
}

S64LocalResult s64_local_open(
    S64LocalDocument* local,
    const char* filename,
    uint8_t discard_dirty
)
{
    S64DocumentWireReader reader;
    S64DosResult dos_result;
    S64DosResult close_result;
    S64DocumentResult document_result;
    uint16_t bytes_read;
    char checked_filename[S64_LOCAL_FILENAME_CAPACITY];

    if (local == 0 || local->dos == 0 || local->document == 0
        || local->document->body == 0
        || !filename_copy(checked_filename, filename)) {
        return S64_LOCAL_INVALID_ARGUMENT;
    }
    if (!discard_dirty) return S64_LOCAL_DIRTY;
    local->document_replaced = 0u;

    dos_result = s64_dos_open_read(local->dos, checked_filename);
    if (dos_result != S64_DOS_OK) {
        local->dos_result = dos_result;
        return S64_LOCAL_IO_ERROR;
    }

    s64_document_wire_reader_init(&reader, local->document);
    local->document_replaced = 1u;
    do {
        bytes_read = 0u;
        dos_result = s64_dos_read(
            local->dos,
            S64_DOS_WRITE_CHUNK,
            s64_document_wire_reader_write,
            &reader,
            &bytes_read
        );
    } while (dos_result == S64_DOS_OK && bytes_read == S64_DOS_WRITE_CHUNK);

    close_result = s64_dos_close(local->dos);
    if (dos_result != S64_DOS_OK || close_result != S64_DOS_OK) {
        local->dos_result = dos_result != S64_DOS_OK ? dos_result : close_result;
        clear_document(local);
        local->filename[0] = '\0';
        return dos_result == S64_DOS_CONSUMER_ABORTED
            && reader.result == S64_DOCUMENT_SINK_ERROR
            ? S64_LOCAL_DOCUMENT_TOO_LARGE
            : (dos_result == S64_DOS_CONSUMER_ABORTED
                ? S64_LOCAL_INVALID_DOCUMENT
                : S64_LOCAL_IO_ERROR);
    }

    document_result = s64_document_wire_reader_finish(&reader);
    if (document_result != S64_DOCUMENT_OK) {
        local->filename[0] = '\0';
        return document_result == S64_DOCUMENT_SINK_ERROR
            ? S64_LOCAL_DOCUMENT_TOO_LARGE
            : S64_LOCAL_INVALID_DOCUMENT;
    }

    strcpy(local->filename, checked_filename);
    return S64_LOCAL_OK;
}

S64LocalResult s64_local_save(S64LocalDocument* local)
{
    if (!s64_local_has_filename(local)) return S64_LOCAL_NO_FILENAME;
    return s64_local_save_as(local, local->filename, 1u);
}

S64LocalResult s64_local_save_as(
    S64LocalDocument* local,
    const char* filename,
    uint8_t overwrite
)
{
    S64LocalResult result;
    S64DosResult dos_result;
    uint8_t exists;
    char checked_filename[S64_LOCAL_FILENAME_CAPACITY];

    if (local == 0 || local->dos == 0 || local->document == 0
        || local->document->body == 0
        || !filename_copy(checked_filename, filename)
        || strcmp(checked_filename, S64_LOCAL_RECOVERY_FILENAME) == 0) {
        return S64_LOCAL_INVALID_ARGUMENT;
    }

    dos_result = s64_dos_file_exists(local->dos, checked_filename, &exists);
    if (dos_result != S64_DOS_OK) {
        local->dos_result = dos_result;
        return S64_LOCAL_IO_ERROR;
    }
    if (exists && !overwrite) {
        return S64_LOCAL_EXISTS;
    }

    if (!default_metadata(local->document, checked_filename)) {
        return S64_LOCAL_INVALID_DOCUMENT;
    }

    result = write_document_file(
        local,
        local->document,
        checked_filename,
        1u
    );
    if (result == S64_LOCAL_OK) {
        strcpy(local->filename, checked_filename);
    }
    return result;
}

S64LocalResult s64_local_recovery_exists(
    S64LocalDocument* local,
    uint8_t* exists
)
{
    S64DosResult result;

    if (local == 0 || local->dos == 0 || exists == 0) {
        return S64_LOCAL_INVALID_ARGUMENT;
    }

    result = s64_dos_file_exists(
        local->dos,
        S64_LOCAL_RECOVERY_FILENAME,
        exists
    );
    if (result != S64_DOS_OK) {
        local->dos_result = result;
        return S64_LOCAL_IO_ERROR;
    }
    return S64_LOCAL_OK;
}

S64LocalResult s64_local_save_recovery(S64LocalDocument* local)
{
    S64Document recovery;

    if (local == 0 || local->dos == 0 || local->document == 0
        || local->document->body == 0) {
        return S64_LOCAL_INVALID_ARGUMENT;
    }

    recovery = *local->document;
    if (!s64_document_slug_valid(recovery.slug)) {
        strcpy(recovery.slug, "recovered-draft");
    }
    if (recovery.title[0] == '\0') {
        strcpy(recovery.title, "Recovered Draft");
    }

    return write_document_file(
        local,
        &recovery,
        S64_LOCAL_RECOVERY_FILENAME,
        1u
    );
}

S64LocalResult s64_local_open_recovery(S64LocalDocument* local)
{
    S64LocalResult result;

    result = s64_local_open(
        local,
        S64_LOCAL_RECOVERY_FILENAME,
        1u
    );
    if (result == S64_LOCAL_OK) {
        local->filename[0] = '\0';
    }
    return result;
}

S64LocalResult s64_local_delete_recovery(S64LocalDocument* local)
{
    S64DosResult result;
    uint8_t exists;

    if (local == 0 || local->dos == 0) {
        return S64_LOCAL_INVALID_ARGUMENT;
    }

    result = s64_dos_file_exists(
        local->dos,
        S64_LOCAL_RECOVERY_FILENAME,
        &exists
    );
    if (result != S64_DOS_OK) {
        local->dos_result = result;
        return S64_LOCAL_IO_ERROR;
    }
    if (!exists) {
        return S64_LOCAL_OK;
    }

    result = s64_dos_delete_file(local->dos, S64_LOCAL_RECOVERY_FILENAME);
    if (result != S64_DOS_OK) {
        local->dos_result = result;
        return S64_LOCAL_IO_ERROR;
    }
    return S64_LOCAL_OK;
}
