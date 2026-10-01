#include <conio.h>
#include <stdint.h>

#include "platform/dos.h"
#include "platform/requirements.h"
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

    /*
     * The selected name was confirmed absent in this single-task preflight
     * context, so the normal overwrite/create open cannot replace user data.
     */
    result = s64_dos_open_write(&dos, filename);
    if (result != S64_DOS_OK) return 0u;

    result = s64_dos_write(&dos, storage_marker, (uint16_t)sizeof(storage_marker));
    (void)s64_dos_close(&dos);
    if (result == S64_DOS_OK && s64_dos_open_read(&dos, filename) == S64_DOS_OK) {
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

    /*
     * Firmware may report non-fatal delete status; final absence is the
     * authoritative cleanup check.
     */
    (void)s64_dos_delete_file(&dos, filename);
    exists = 1u;
    if (s64_dos_file_exists(&dos, filename, &exists) != S64_DOS_OK || exists) {
        return 0u;
    }
    return verified;
}

static S64PlatformRequirement check_platform(void)
{
    S64UciCapabilities capabilities;
    S64PlatformRequirements requirements;

    s64_uci_probe_capabilities(&capabilities);
    requirements.interface_present = capabilities.interface_present;
    requirements.dos_target = capabilities.dos_target;
    requirements.reu_present = s64_reu_probe();
    requirements.reu_banks = requirements.reu_present
        ? s64_reu_probe_banks()
        : 0u;

    return s64_platform_check_requirements(&requirements);
}

static void show_requirement(S64PlatformRequirement requirement)
{
    clrscr();
    cputs("SEE64YOU PLATFORM CHECK\r\n\r\n");

    switch (requirement) {
        case S64_PLATFORM_MISSING_UCI:
            cputs("COMMAND INTERFACE REQUIRED\r\n\r\n");
            cputs("OPEN ULTIMATE MENU\r\n");
            cputs("F2 SETTINGS\r\n");
            cputs("C64 AND CARTRIDGE SETTINGS\r\n");
            cputs("COMMAND INTERFACE: ENABLED\r\n");
            break;
        case S64_PLATFORM_MISSING_DOS:
            cputs("ULTIMATE-DOS REQUIRED\r\n\r\n");
            cputs("COMMAND INTERFACE IS READY\r\n");
            cputs("CHECK ULTIMATE FIRMWARE\r\n");
            break;
        case S64_PLATFORM_MISSING_REU:
            cputs("16 MB REU REQUIRED\r\n\r\n");
            cputs("OPEN ULTIMATE MENU\r\n");
            cputs("F2 SETTINGS\r\n");
            cputs("C64 AND CARTRIDGE SETTINGS\r\n");
            cputs("RAM EXPANSION UNIT: ENABLED\r\n");
            cputs("REU SIZE: 16 MB\r\n");
            break;
        case S64_PLATFORM_WRONG_REU_SIZE:
            cputs("REU SIZE MUST BE 16 MB\r\n\r\n");
            cputs("OPEN ULTIMATE MENU\r\n");
            cputs("F2 SETTINGS\r\n");
            cputs("C64 AND CARTRIDGE SETTINGS\r\n");
            cputs("REU SIZE: 16 MB\r\n");
            break;
        default:
            break;
    }

    cputs("\r\nR RECHECK    X EXIT");
}

static void show_storage_requirement(void)
{
    clrscr();
    cputs("SEE64YOU PLATFORM CHECK\r\n\r\n");
    cputs("SD STORAGE NOT READY\r\n\r\n");
    cputs("CHECK SD CARD IS INSERTED\r\n");
    cputs("AND WRITABLE, THEN RECHECK\r\n");
    cputs("\r\nR RECHECK    X EXIT");
}

int main(void)
{
    S64PlatformRequirement requirement;
    unsigned char key;

    for (;;) {
        requirement = check_platform();
        if (requirement == S64_PLATFORM_READY && probe_sd_storage()) {
            clrscr();
            cputs("SEE64YOU PLATFORM CHECK\r\n\r\n");
            cputs("COMMAND INTERFACE READY\r\n");
            cputs("ULTIMATE-DOS       READY\r\n");
            cputs("MEMORY 16 MB       READY\r\n");
            cputs("STORAGE            READY\r\n");
            cputs("\r\nPLATFORM READY\r\n");
            return 0;
        }

        if (requirement == S64_PLATFORM_READY) {
            show_storage_requirement();
        } else {
            show_requirement(requirement);
        }

        for (;;) {
            key = (unsigned char)cgetc();
            if (key == 'R' || key == 'r') break;
            if (key == 'X' || key == 'x') return 1;
        }
    }
}
