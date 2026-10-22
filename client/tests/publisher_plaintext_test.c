#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/reu_layout.h"
#include "publisher/plaintext.h"

static unsigned char reu[2][65536u];
static const unsigned char* input_data;
static uint16_t input_length;
static uint16_t input_offset;
static unsigned char output_data[1024];
static uint16_t output_length;
static unsigned char temp_data[1024];
static uint16_t temp_length;
static uint8_t read_open;
static uint8_t write_open;
static uint8_t output_exists;

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

uint8_t s64_reu_store(
    uint8_t bank,
    uint16_t address,
    const void* source,
    uint16_t length
)
{
    if (bank >= 2u || source == 0
        || (uint32_t)address + length > 65536u) {
        return 0u;
    }
    memcpy(&reu[bank][address], source, length);
    return 1u;
}

uint8_t s64_reu_load(
    uint8_t bank,
    uint16_t address,
    void* destination,
    uint16_t length
)
{
    if (bank >= 2u || destination == 0
        || (uint32_t)address + length > 65536u) {
        return 0u;
    }
    memcpy(destination, &reu[bank][address], length);
    return 1u;
}

S64DosResult s64_dos_file_exists(
    S64Dos* dos,
    const char* filename,
    uint8_t* exists
)
{
    (void)dos;
    if (filename == 0 || exists == 0) return S64_DOS_INVALID_ARGUMENT;
    *exists = strcmp(filename, "OUT.TXT") == 0 && output_exists;
    return S64_DOS_OK;
}

S64DosResult s64_dos_open_read(S64Dos* dos, const char* filename)
{
    (void)dos;
    if (filename == 0 || strcmp(filename, "IN.TXT") != 0) {
        return S64_DOS_NOT_FOUND;
    }
    input_offset = 0u;
    read_open = 1u;
    return S64_DOS_OK;
}

S64DosResult s64_dos_open_write(S64Dos* dos, const char* filename)
{
    (void)dos;
    if (filename == 0 || strcmp(filename, "OUT.TXT.tmp") != 0) {
        return S64_DOS_INVALID_ARGUMENT;
    }
    temp_length = 0u;
    write_open = 1u;
    return S64_DOS_OK;
}

S64DosResult s64_dos_read(
    S64Dos* dos,
    uint16_t requested,
    S64UciDataSink sink,
    void* context,
    uint16_t* bytes_read
)
{
    uint16_t remaining;
    uint16_t count;

    (void)dos;
    if (bytes_read != 0) *bytes_read = 0u;
    if (!read_open || sink == 0) return S64_DOS_NO_FILE_OPEN;

    remaining = (uint16_t)(input_length - input_offset);
    count = remaining < requested ? remaining : requested;
    if (count != 0u
        && !sink(input_data + input_offset, count, context)) {
        return S64_DOS_CONSUMER_ABORTED;
    }
    input_offset = (uint16_t)(input_offset + count);
    if (bytes_read != 0) *bytes_read = count;
    return S64_DOS_OK;
}

S64DosResult s64_dos_write(
    S64Dos* dos,
    const uint8_t* data,
    uint16_t length
)
{
    (void)dos;
    if (!write_open) return S64_DOS_NO_FILE_OPEN;
    if ((uint32_t)temp_length + length > sizeof(temp_data)) {
        return S64_DOS_TARGET_ERROR;
    }
    memcpy(temp_data + temp_length, data, length);
    temp_length = (uint16_t)(temp_length + length);
    return S64_DOS_OK;
}

S64DosResult s64_dos_close(S64Dos* dos)
{
    (void)dos;
    read_open = 0u;
    write_open = 0u;
    return S64_DOS_OK;
}

S64DosResult s64_dos_delete_file(S64Dos* dos, const char* filename)
{
    (void)dos;
    if (strcmp(filename, "OUT.TXT") == 0) {
        output_exists = 0u;
        output_length = 0u;
        return S64_DOS_OK;
    }
    if (strcmp(filename, "OUT.TXT.tmp") == 0) {
        temp_length = 0u;
        return S64_DOS_OK;
    }
    return S64_DOS_NOT_FOUND;
}

S64DosResult s64_dos_rename_file(
    S64Dos* dos,
    const char* filename,
    const char* new_name
)
{
    (void)dos;
    if (strcmp(filename, "OUT.TXT.tmp") != 0
        || strcmp(new_name, "OUT.TXT") != 0) {
        return S64_DOS_INVALID_ARGUMENT;
    }
    memcpy(output_data, temp_data, temp_length);
    output_length = temp_length;
    output_exists = 1u;
    temp_length = 0u;
    return S64_DOS_OK;
}

static void insert_text(S64Buffer* buffer, const char* text)
{
    while (*text != '\0') {
        require(
            s64_buffer_insert(buffer, (unsigned char)*text++),
            "insert fixture"
        );
    }
}

static void require_buffer(S64Buffer* buffer, const char* expected)
{
    unsigned int i;
    unsigned int length = (unsigned int)strlen(expected);

    require(s64_buffer_length(buffer) == length, "buffer length");
    for (i = 0u; i < length; ++i) {
        require(
            s64_buffer_char_at(buffer, i) == (unsigned char)expected[i],
            "buffer contents"
        );
    }
}

int main(void)
{
    S64Dos dos;
    S64Buffer buffer;
    S64PlainTextResult result;
    static const unsigned char import_text[] =
        "First\r\nSecond\rThird\n";
    static const unsigned char invalid_text[] = {
        'B', 'a', 'd', 0x80u, '\n'
    };
    static const char export_text[] =
        "Line 1\nMixed []{}\n";

    memset(&dos, 0, sizeof(dos));
    memset(reu, 0, sizeof(reu));
    output_exists = 0u;
    output_length = 0u;
    temp_length = 0u;

    s64_buffer_init(&buffer);
    insert_text(&buffer, export_text);

    result = s64_plaintext_export(&dos, &buffer, "OUT.TXT", 0u);
    require(result == S64_PLAINTEXT_OK, "text export succeeds");
    require(output_exists, "export target exists");
    require(
        output_length == strlen(export_text)
            && memcmp(output_data, export_text, output_length) == 0,
        "export body is exact ASCII"
    );

    result = s64_plaintext_export(&dos, &buffer, "OUT.TXT", 0u);
    require(result == S64_PLAINTEXT_EXISTS, "overwrite requires consent");
    result = s64_plaintext_export(&dos, &buffer, "OUT.TXT", 1u);
    require(result == S64_PLAINTEXT_OK, "overwrite succeeds");

    input_data = import_text;
    input_length = (uint16_t)(sizeof(import_text) - 1u);
    result = s64_plaintext_import(&dos, &buffer, "IN.TXT");
    require(result == S64_PLAINTEXT_OK, "text import succeeds");
    require_buffer(&buffer, "First\nSecond\nThird\n");

    s64_buffer_init(&buffer);
    insert_text(&buffer, "keep me");
    input_data = invalid_text;
    input_length = (uint16_t)sizeof(invalid_text);
    result = s64_plaintext_import(&dos, &buffer, "IN.TXT");
    require(
        result == S64_PLAINTEXT_INVALID_TEXT,
        "unsupported input rejected"
    );
    require_buffer(&buffer, "keep me");

    require(
        s64_plaintext_import(&dos, &buffer, "")
            == S64_PLAINTEXT_INVALID_ARGUMENT,
        "invalid arguments rejected"
    );

    puts("publisher plain-text tests passed");
    return 0;
}
