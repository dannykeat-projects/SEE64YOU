#include <stdint.h>
#include <string.h>
#include "platform/dos.h"
#include "platform/package_paths.h"
#include "platform/uci.h"

/* Read-only synchronous-context probe; credentials remain opaque in RAM.
 * $C040=$A5 success/$5A failure, $C041=failed stage; return to BASIC. */
#define RESULT (*(volatile uint8_t*)0xC040u)
#define DETAIL (*(volatile uint8_t*)0xC041u)
#define BUSY_RESULT (*(volatile uint8_t*)0xC042u)
#define CAPACITY 512u
typedef struct { uint8_t bytes[CAPACITY]; uint16_t count; } Read;
static Read primary_bytes;
static Read secondary_bytes;

static uint8_t collect(const uint8_t* data, uint16_t length, void* context)
{
    Read* read = (Read*)context;
    if (length > CAPACITY - read->count) return 0u;
    memcpy(read->bytes + read->count, data, length);
    read->count = (uint16_t)(read->count + length);
    return 1u;
}

int main(void)
{
    S64Dos primary;
    S64Dos secondary;
    uint16_t count;
    uint8_t primary_open = 0u;
    uint8_t secondary_open = 0u;
    RESULT = 0u;
    DETAIL = 1u;
    if (!s64_uci_probe_target(S64_DOS_TARGET_PRIMARY)
        || !s64_uci_probe_target(S64_DOS_TARGET_SECONDARY)) goto done;
    s64_dos_init(&primary, s64_uci_execute, S64_DOS_TARGET_PRIMARY);
    s64_dos_init(&secondary, s64_uci_execute, S64_DOS_TARGET_SECONDARY);
    DETAIL = 2u;
    if (s64_dos_change_dir(&secondary, S64_PACKAGE_DIRECTORY) != S64_DOS_OK) goto done;
    DETAIL = 3u;
    if (s64_dos_open_read(&secondary, "SEE64YOU.CFG") != S64_DOS_OK) goto done;
    secondary_open = 1u;
    DETAIL = 4u;
    if (s64_dos_read(&secondary, 8u, collect, &secondary_bytes, &count)
        != S64_DOS_OK || count != 8u) goto done;
    DETAIL = 5u;
    /* Firmware may refuse CD while either target has an open handle. Record
     * that capability, but production closes documents before journal I/O. */
    BUSY_RESULT = (uint8_t)s64_dos_change_dir(&primary, S64_SD_ROOT_DIRECTORY);
    DETAIL = 6u;
    if (s64_dos_read(&secondary, CAPACITY - 8u, collect, &secondary_bytes, &count)
        != S64_DOS_OK) goto done;
    DETAIL = 7u;
    if (s64_dos_read(&secondary, 1u, collect, &secondary_bytes, &count)
        != S64_DOS_OK || count != 0u || s64_dos_close(&secondary) != S64_DOS_OK) goto done;
    secondary_open = 0u;
    DETAIL = 8u;
    if (s64_dos_change_dir(&primary, S64_SD_ROOT_DIRECTORY) != S64_DOS_OK) goto done;
    DETAIL = 9u;
    if (s64_dos_open_read(&primary, S64_PACKAGE_DIRECTORY "SEE64YOU.CFG")
        != S64_DOS_OK) goto done;
    primary_open = 1u;
    DETAIL = 10u;
    if (s64_dos_read(&primary, CAPACITY, collect, &primary_bytes, &count)
        != S64_DOS_OK || count < 8u) goto done;
    DETAIL = 11u;
    if (s64_dos_read(&primary, 1u, collect, &primary_bytes, &count)
        != S64_DOS_OK || count != 0u) goto done;
    DETAIL = 12u;
    if (s64_dos_close(&primary) != S64_DOS_OK) goto done;
    primary_open = 0u;
    DETAIL = 13u;
    if (primary_bytes.count != secondary_bytes.count
        || memcmp(primary_bytes.bytes, secondary_bytes.bytes, primary_bytes.count)) goto done;
    DETAIL = 14u;
    /* CWD isolation is checked independently of the earlier open handle. */
    if (s64_dos_open_read(&secondary, "SEE64YOU.CFG") != S64_DOS_OK) goto done;
    secondary_open = 1u;
    DETAIL = 15u;
    if (s64_dos_close(&secondary) != S64_DOS_OK) goto done;
    secondary_open = 0u;
    DETAIL = 0u;
    RESULT = 0xA5u;
done:
    if (primary_open) (void)s64_dos_close(&primary);
    if (secondary_open) (void)s64_dos_close(&secondary);
    memset(&primary_bytes, 0, sizeof(primary_bytes));
    memset(&secondary_bytes, 0, sizeof(secondary_bytes));
    if (RESULT != 0xA5u) RESULT = 0x5Au;
    return RESULT == 0xA5u ? 0 : 1;
}
