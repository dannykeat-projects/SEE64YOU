#include "companion_loader.h"

#define S64_COMPANION_STORAGE_PATH "/sd/"
#define S64_COMPANION_MIN_PAYLOAD \
    ((uint16_t)(S64_COMPANION_START_ADDRESS - S64_COMPANION_LOAD_ADDRESS + 1u))
#define S64_COMPANION_MAX_PAYLOAD \
    ((uint16_t)(S64_COMPANION_RESIDENT_BASE - S64_COMPANION_LOAD_ADDRESS))

typedef struct {
    uint8_t bytes[2];
    uint8_t count;
} S64CompanionHeader;

static const char* const filenames[S64_COMPANION_COUNT] = {
    "S64EDIT.PRG",
    "S64PUB.PRG",
    "S64CFG.PRG"
};

static uint8_t capture_header(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64CompanionHeader* header = (S64CompanionHeader*)context;
    uint16_t i;

    if (header == 0 || (uint16_t)header->count + length > 2u) {
        return 0u;
    }
    for (i = 0u; i < length; ++i) {
        header->bytes[header->count++] = data[i];
    }
    return 1u;
}

const char* s64_companion_filename(S64CompanionProgram program)
{
    return (unsigned int)program < S64_COMPANION_COUNT
        ? filenames[(unsigned int)program]
        : 0;
}

S64CompanionResult s64_companion_open_named_preflight(
    S64Dos* dos,
    const char* filename,
    S64CompanionImage* image
)
{
    S64CompanionHeader header;
    S64DosResult result;
    uint32_t file_size;
    uint32_t payload_size;
    uint16_t bytes_read = 0u;
    uint16_t load_address;
    uint8_t opened = 0u;

    if (dos == 0 || filename == 0 || image == 0) {
        return S64_COMPANION_INVALID_ARGUMENT;
    }

    image->load_address = 0u;
    image->payload_size = 0u;
    image->load_end = 0u;
    image->dos_result = S64_DOS_OK;

    result = s64_dos_change_dir(dos, S64_COMPANION_STORAGE_PATH);
    if (result != S64_DOS_OK) goto dos_error;

    result = s64_dos_open_read(dos, filename);
    if (result != S64_DOS_OK) goto dos_error;
    opened = 1u;

    result = s64_dos_file_size(dos, &file_size);
    if (result != S64_DOS_OK) goto dos_error;
    if (file_size < (uint32_t)(2u + S64_COMPANION_MIN_PAYLOAD)
        || file_size > (uint32_t)(2u + S64_COMPANION_MAX_PAYLOAD)) {
        goto invalid_image;
    }

    header.count = 0u;
    result = s64_dos_read(dos, 2u, capture_header, &header, &bytes_read);
    if (result != S64_DOS_OK) goto dos_error;
    if (bytes_read != 2u || header.count != 2u) goto invalid_image;

    load_address = (uint16_t)(
        (uint16_t)header.bytes[0]
        | ((uint16_t)header.bytes[1] << 8)
    );
    if (load_address != S64_COMPANION_LOAD_ADDRESS) goto invalid_image;

    payload_size = file_size - 2u;
    image->load_address = load_address;
    image->payload_size = (uint16_t)payload_size;
    image->load_end = (uint16_t)(load_address + payload_size - 1u);
    return S64_COMPANION_OK;

dos_error:
    image->dos_result = result;
    if (opened) (void)s64_dos_close(dos);
    return S64_COMPANION_DOS_ERROR;

invalid_image:
    if (opened) (void)s64_dos_close(dos);
    return S64_COMPANION_INVALID_IMAGE;
}

S64CompanionResult s64_companion_open_preflight(
    S64Dos* dos,
    S64CompanionProgram program,
    S64CompanionImage* image
)
{
    const char* filename = s64_companion_filename(program);

    if (filename == 0) return S64_COMPANION_INVALID_ARGUMENT;
    return s64_companion_open_named_preflight(dos, filename, image);
}
