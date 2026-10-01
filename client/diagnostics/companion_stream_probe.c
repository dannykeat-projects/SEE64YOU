#include <conio.h>
#include <stdint.h>
#include <time.h>

#include "platform/companion_loader.h"
#include "platform/dos.h"
#include "platform/uci.h"

static S64Dos dos;
static uint16_t streamed;

static uint8_t count_stream(
    const uint8_t* data,
    uint16_t length,
    void* context
)
{
    (void)data;
    (void)context;
    if ((uint32_t)streamed + length > 65535UL) return 0u;
    streamed = (uint16_t)(streamed + length);
    return 1u;
}

static uint8_t run_one(S64CompanionProgram program)
{
    S64CompanionImage image;
    S64CompanionResult preflight;
    S64DosResult read_result;
    S64DosResult close_result;
    uint16_t bytes_read = 0u;
    clock_t started;
    unsigned long ticks;
    uint8_t okay;

    cprintf("%s\r\n", s64_companion_filename(program));
    preflight = s64_companion_open_preflight(&dos, program, &image);
    if (preflight != S64_COMPANION_OK) {
        cprintf("  PREFLIGHT FAIL %u DOS %u\r\n",
            preflight, image.dos_result);
        return 0u;
    }

    cprintf("  $%04X-$%04X  %u BYTES\r\n",
        image.load_address, image.load_end, image.payload_size);
    streamed = 0u;
    started = clock();
    read_result = s64_dos_read(
        &dos,
        image.payload_size,
        count_stream,
        0,
        &bytes_read
    );
    ticks = (unsigned long)(clock() - started);
    close_result = s64_dos_close(&dos);
    okay = read_result == S64_DOS_OK
        && bytes_read == image.payload_size
        && streamed == image.payload_size;

    cprintf("  READ %u/%u DOS %u CLOSE %u\r\n",
        bytes_read, image.payload_size, read_result, close_result);
    cprintf("  TICKS %lu  %s\r\n", ticks, okay ? "PASS" : "FAIL");
    return okay;
}

int main(void)
{
    uint8_t passed;

    bgcolor(COLOR_BLUE);
    bordercolor(COLOR_BLUE);
    textcolor(COLOR_LIGHTBLUE);
    clrscr();
    cursor(0);
    cputs("SEE64YOU COMPANION DOS STREAM PROBE\r\n\r\n");
    cputs("READ ONLY; DOES NOT EXECUTE PRGS.\r\n");
    cputs("EXPECT CURRENT MATCHING PACKAGE IN /SD/.\r\n\r\n");
    cputs("Y TO RUN; OTHER KEY EXITS.");
    if (cgetc() != 'y') return 0;

    clrscr();
    if (!s64_uci_probe_target(S64_DOS_TARGET_PRIMARY)) {
        cputs("ULTIMATE DOS $01 UNAVAILABLE\r\n");
        cgetc();
        return 1;
    }

    s64_dos_init(&dos, s64_uci_execute, S64_DOS_TARGET_PRIMARY);
    passed = run_one(S64_COMPANION_EDITOR);
    passed = (uint8_t)(run_one(S64_COMPANION_PUBLISHER) && passed);
    passed = (uint8_t)(run_one(S64_COMPANION_SETTINGS) && passed);

    cputs("\r\n");
    cputs(passed ? "ALL STREAMS PASS\r\n" : "STREAM PROBE FAILED\r\n");
    cputs("PRESS KEY TO EXIT");
    cgetc();
    return passed ? 0 : 1;
}
