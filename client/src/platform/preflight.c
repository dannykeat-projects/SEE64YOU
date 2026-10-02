#include "platform/preflight.h"

#include "platform/dos.h"
#include "platform/reu.h"
#include "platform/uci.h"

#define S64_PREFLIGHT_STORAGE_PATH "/sd/"
#define S64_PREFLIGHT_TEMP_ATTEMPTS 10u

typedef struct {
    uint8_t offset;
    uint8_t mismatch;
} S64StorageReadback;

static const uint8_t storage_marker[] = {
    0x53u, 0x36u, 0x34u, 0x43u, 0x48u, 0x45u, 0x43u, 0x4bu
};

static uint8_t verify_storage_data(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    S64StorageReadback* readback = (S64StorageReadback*)context;
    uint16_t i;

    for (i = 0u; i < length; ++i) {
        if (readback->offset >= (uint8_t)sizeof(storage_marker)
            || data[i] != storage_marker[readback->offset]) {
            readback->mismatch = 1u;
        }
        ++readback->offset;
    }
    return 1u;
}

static uint8_t probe_sd_storage(void)
{
    S64Dos dos;
    S64DosResult result;
    S64StorageReadback readback;
    char filename[] = "S64CHK0.TMP";
    uint16_t bytes_read = 0u;
    uint8_t exists;
    uint8_t attempt;
    uint8_t verified = 0u;

    s64_dos_init(&dos, s64_uci_execute, S64_DOS_TARGET_PRIMARY);
    if (s64_dos_change_dir(&dos, S64_PREFLIGHT_STORAGE_PATH) != S64_DOS_OK) {
        return 0u;
    }

    for (attempt = 0u; attempt < S64_PREFLIGHT_TEMP_ATTEMPTS; ++attempt) {
        filename[6] = (char)('0' + attempt);
        if (s64_dos_file_exists(&dos, filename, &exists) != S64_DOS_OK) {
            return 0u;
        }
        if (!exists) break;
    }
    if (attempt == S64_PREFLIGHT_TEMP_ATTEMPTS) return 0u;

    result = s64_dos_open_write(&dos, filename);
    if (result != S64_DOS_OK) return 0u;

    result = s64_dos_write(&dos, storage_marker, (uint16_t)sizeof(storage_marker));
    (void)s64_dos_close(&dos);

    if (result == S64_DOS_OK
        && s64_dos_open_read(&dos, filename) == S64_DOS_OK) {
        readback.offset = 0u;
        readback.mismatch = 0u;
        result = s64_dos_read(
            &dos,
            (uint16_t)sizeof(storage_marker),
            verify_storage_data,
            &readback,
            &bytes_read
        );
        (void)s64_dos_close(&dos);
        verified = result == S64_DOS_OK
            && bytes_read == (uint16_t)sizeof(storage_marker)
            && readback.offset == (uint8_t)sizeof(storage_marker)
            && !readback.mismatch;
    }

    (void)s64_dos_delete_file(&dos, filename);
    exists = 1u;
    if (s64_dos_file_exists(&dos, filename, &exists) != S64_DOS_OK
        || exists) {
        return 0u;
    }

    return verified;
}

void s64_platform_preflight_run(S64PlatformPreflight* preflight)
{
    S64UciCapabilities capabilities;
    S64PlatformRequirements requirements;

    if (preflight == 0) return;

    preflight->requirement = S64_PLATFORM_MISSING_UCI;
    preflight->storage_ready = 0u;

    s64_uci_probe_capabilities(&capabilities);
    requirements.interface_present = capabilities.interface_present;
    requirements.dos_target = capabilities.dos_target;
    requirements.reu_present = s64_reu_probe();
    requirements.reu_banks = requirements.reu_present
        ? s64_reu_probe_banks()
        : 0u;

    preflight->requirement = s64_platform_check_requirements(&requirements);
    if (preflight->requirement == S64_PLATFORM_READY) {
        preflight->storage_ready = probe_sd_storage();
    }
}
