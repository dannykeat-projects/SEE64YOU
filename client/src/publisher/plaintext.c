#include "publisher/plaintext.h"

#include "charset.h"

#include <string.h>

#define S64_PLAINTEXT_CHUNK 64u

typedef struct {
    S64Buffer* buffer;
    uint32_t count;
    S64PlainTextResult result;
    uint8_t write;
    uint8_t pending_cr;
} S64PlainImport;

static uint8_t native_chunk[S64_PLAINTEXT_CHUNK];
static uint8_t ascii_chunk[S64_PLAINTEXT_CHUNK];

static uint8_t filename_valid(const char* filename)
{
    uint16_t length = 0u;

    if (filename == 0 || filename[0] == '\0') return 0u;
    while (filename[length] != '\0') {
        if (filename[length] == '\r'
            || filename[length] == '\n'
            || length + 1u >= S64_PLAINTEXT_FILENAME_CAPACITY) {
            return 0u;
        }
        ++length;
    }
    return 1u;
}

static uint8_t import_emit(S64PlainImport* import, uint8_t ascii)
{
    uint8_t native;

    if (!s64_charset_ascii_to_native(ascii, &native)) {
        import->result = S64_PLAINTEXT_INVALID_TEXT;
        return 0u;
    }
    if (import->count >= S64_BUFFER_CAPACITY) {
        import->result = S64_PLAINTEXT_TOO_LARGE;
        return 0u;
    }
    ++import->count;

    if (import->write && !s64_buffer_insert(import->buffer, native)) {
        import->result = S64_PLAINTEXT_TOO_LARGE;
        return 0u;
    }
    return 1u;
}

static uint8_t import_sink(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64PlainImport* import = (S64PlainImport*)context;
    uint16_t i;
    uint8_t byte;

    for (i = 0u; i < length; ++i) {
        byte = data[i];

        if (import->pending_cr) {
            import->pending_cr = 0u;
            if (!import_emit(import, (uint8_t)'\n')) return 0u;
            if (byte == '\n') continue;
        }

        if (byte == '\r') {
            import->pending_cr = 1u;
        } else if (!import_emit(import, byte)) {
            return 0u;
        }
    }
    return 1u;
}

static S64PlainTextResult import_pass(
    S64Dos* dos,
    S64Buffer* buffer,
    const char* filename,
    uint8_t write
)
{
    S64PlainImport import;
    S64DosResult result;
    S64DosResult close_result;
    uint16_t bytes_read;

    result = s64_dos_open_read(dos, filename);
    if (result != S64_DOS_OK) return S64_PLAINTEXT_IO_ERROR;

    import.buffer = buffer;
    import.count = 0u;
    import.result = S64_PLAINTEXT_OK;
    import.write = write;
    import.pending_cr = 0u;
    if (write) s64_buffer_init(buffer);

    do {
        bytes_read = 0u;
        result = s64_dos_read(
            dos,
            S64_DOS_WRITE_CHUNK,
            import_sink,
            &import,
            &bytes_read
        );
    } while (result == S64_DOS_OK && bytes_read == S64_DOS_WRITE_CHUNK);

    if (result == S64_DOS_OK && import.pending_cr) {
        import.pending_cr = 0u;
        if (!import_emit(&import, (uint8_t)'\n')) {
            result = S64_DOS_CONSUMER_ABORTED;
        }
    }

    close_result = s64_dos_close(dos);
    if (import.result != S64_PLAINTEXT_OK) return import.result;
    if (result != S64_DOS_OK || close_result != S64_DOS_OK) {
        return S64_PLAINTEXT_IO_ERROR;
    }
    return S64_PLAINTEXT_OK;
}

const char* s64_plaintext_result_message(S64PlainTextResult result)
{
    switch (result) {
        case S64_PLAINTEXT_OK: return "OK";
        case S64_PLAINTEXT_INVALID_ARGUMENT: return "Invalid text file name";
        case S64_PLAINTEXT_EXISTS: return "Text file already exists";
        case S64_PLAINTEXT_INVALID_TEXT: return "Unsupported text character";
        case S64_PLAINTEXT_TOO_LARGE: return "Text file too large";
        case S64_PLAINTEXT_IO_ERROR:
        default: return "Text file I/O error";
    }
}

S64PlainTextResult s64_plaintext_export(
    S64Dos* dos,
    const S64Buffer* buffer,
    const char* filename,
    uint8_t overwrite
)
{
    S64DosResult result;
    S64DosResult close_result;
    uint8_t exists;
    unsigned int length;
    unsigned int offset = 0u;
    unsigned int count;
    uint16_t i;
    char temporary[S64_PLAINTEXT_FILENAME_CAPACITY + 4u];

    if (dos == 0 || buffer == 0 || !filename_valid(filename)) {
        return S64_PLAINTEXT_INVALID_ARGUMENT;
    }

    result = s64_dos_file_exists(dos, filename, &exists);
    if (result != S64_DOS_OK) return S64_PLAINTEXT_IO_ERROR;
    if (exists && !overwrite) return S64_PLAINTEXT_EXISTS;

    strcpy(temporary, filename);
    strcat(temporary, ".tmp");
    result = s64_dos_open_write(dos, temporary);
    if (result != S64_DOS_OK) return S64_PLAINTEXT_IO_ERROR;

    length = s64_buffer_length(buffer);
    while (offset < length) {
        count = s64_buffer_read(
            buffer,
            offset,
            native_chunk,
            S64_PLAINTEXT_CHUNK
        );
        if (count == 0u) {
            (void)s64_dos_close(dos);
            (void)s64_dos_delete_file(dos, temporary);
            return S64_PLAINTEXT_IO_ERROR;
        }
        for (i = 0u; i < count; ++i) {
            if (!s64_charset_native_to_ascii(native_chunk[i], &ascii_chunk[i])) {
                (void)s64_dos_close(dos);
                (void)s64_dos_delete_file(dos, temporary);
                return S64_PLAINTEXT_INVALID_TEXT;
            }
        }
        result = s64_dos_write(dos, ascii_chunk, (uint16_t)count);
        if (result != S64_DOS_OK) {
            (void)s64_dos_close(dos);
            (void)s64_dos_delete_file(dos, temporary);
            return S64_PLAINTEXT_IO_ERROR;
        }
        offset += count;
    }

    close_result = s64_dos_close(dos);
    if (close_result != S64_DOS_OK) {
        (void)s64_dos_delete_file(dos, temporary);
        return S64_PLAINTEXT_IO_ERROR;
    }

    if (exists) {
        result = s64_dos_delete_file(dos, filename);
        if (result != S64_DOS_OK) {
            (void)s64_dos_delete_file(dos, temporary);
            return S64_PLAINTEXT_IO_ERROR;
        }
    }
    result = s64_dos_rename_file(dos, temporary, filename);
    if (result != S64_DOS_OK) {
        (void)s64_dos_delete_file(dos, temporary);
        return S64_PLAINTEXT_IO_ERROR;
    }
    return S64_PLAINTEXT_OK;
}

S64PlainTextResult s64_plaintext_import(
    S64Dos* dos,
    S64Buffer* buffer,
    const char* filename
)
{
    S64PlainTextResult result;

    if (dos == 0 || buffer == 0 || !filename_valid(filename)) {
        return S64_PLAINTEXT_INVALID_ARGUMENT;
    }

    result = import_pass(dos, buffer, filename, 0u);
    if (result != S64_PLAINTEXT_OK) return result;

    result = import_pass(dos, buffer, filename, 1u);
    if (result != S64_PLAINTEXT_OK) {
        s64_buffer_init(buffer);
    }
    return result;
}
