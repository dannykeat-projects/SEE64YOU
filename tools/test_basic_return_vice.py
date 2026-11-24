#!/usr/bin/env python3
"""Verify the transient SYS return capsule with a synthetic original context.

This proves restoration mechanics, not real launcher capture/session binding.
The unused record reader is stubbed; the production final-execute path is linked.
"""
from pathlib import Path
import argparse
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "client/build/basic-return-probe"
sys.path.insert(0, str(ROOT / "tools/vice"))
from test_reu_probe import wait_for_monitor


def main(rom_dir: Path, valid_workspace: bool = False) -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    # Landing code records restored SP/flags/ZP/map/stack bytes before parking.
    landing = [0x08, 0x68, 0x8d, 0x67, 0xc0, 0xba, 0x8e, 0x61, 0xc0, 0x78]
    for address, result in ((2, 0xc062), (0x1b, 0xc063), (1, 0xc064),
                            (0x100, 0xc065), (0x1ff, 0xc066)):
        landing += [0xad, address & 255, address >> 8,
                    0x8d, result & 255, result >> 8]
    landing += [0xa9, 0xa5, 0x8d, 0x60, 0xc0]
    parked = 0xc000 + len(landing)
    landing += [0x4c, parked & 255, parked >> 8]
    source = OUT / "main.c"
    source.write_text('''#include <string.h>
#include "platform/basic_return_internal.h"
#include "platform/reu.h"
#include "platform/crc32.h"
const S64ReuAllocation* s64_basic_return_allocation(const S64ReuRuntimeControl* c)
{ (void)c; return 0; }
uint8_t s64_basic_return_active(const S64ReuRuntimeControl* c,
    const S64SessionJournal* j, uint8_t* w)
{ (void)c; (void)j; (void)w; return 0; }
uint8_t s64_basic_return_header_valid(const uint8_t* w)
{ (void)w; return 0; }
uint8_t s64_reu_load(uint8_t b, uint16_t a, void* d, uint16_t n)
{ (void)b; (void)a; (void)d; (void)n; return 0; }
uint32_t s64_crc32_update(uint32_t c, const uint8_t* b, uint16_t n)
{ (void)b; (void)n; return c; }
static const unsigned char landing[] = {''' + ','.join(map(str, landing)) + '''};
int main(void) {
    unsigned int i;
    unsigned char* context = (unsigned char*)0xc820;
    context[0] = 0x37; context[1] = 0xf0; context[2] = 0x24;
    for (i=0; i<26; ++i) context[3+i] = (unsigned char)(0x40+i);
    for (i=0; i<256; ++i) context[29+i] = (unsigned char)i;
    context[29+0xf1] = 0xff; context[29+0xf2] = 0xbf;
    context[285] = 14; context[286] = 6; context[287] = 14;
    *(volatile unsigned char*)0xd020 = 0;
    *(volatile unsigned char*)0xd021 = 0;
    *(volatile unsigned char*)0x0286 = 1;
    memcpy((void*)0xc000, landing, sizeof(landing));
    *(volatile unsigned char*)0xc060 = 0;
    /* Reproduce large-PRG BASIC end beyond the string-memory ceiling. */
    *(unsigned int*)0x2b = 0x0801;
    *(unsigned int*)0x31 = 0xbfbf;
    *(unsigned int*)0x37 = 0xa000;
    s64_basic_return_execute((const unsigned char*)0xc800);
    return 1;
}
''')
    if valid_workspace:
        source.write_text(source.read_text().replace(
            '*(unsigned int*)0x31 = 0xbfbf;',
            '*(unsigned int*)0x2d = 0xa000; *(unsigned int*)0x2f = 0xa000; '
            '*(unsigned int*)0x31 = 0xa000; *(unsigned int*)0x33 = 0xa000;'))
    prg = OUT / "probe.prg"
    subprocess.run(["cl65", "-t", "c64", "-Os", "-DS64_MANAGED_OVERLAY=1",
                    "-I", str(ROOT / "client/src"), "-o", str(prg), str(source),
                    str(ROOT / "client/src/platform/basic_return_read.c"),
                    str(ROOT / "client/src/platform/basic_return_stub.s"),
                    str(ROOT / "client/src/platform/basic_workspace_reset.s")], check=True)
    command = ["xvfb-run", "-a", "x64sc", "+sound", "-autostartprgmode", "1",
               "-binarymonitor", "-binarymonitoraddress", "ip4://127.0.0.1:6528"]
    for kind in ("kernal", "basic", "chargen"):
        rom = rom_dir / f"{kind}.rom"
        if not rom.is_file():
            raise RuntimeError(f"Missing ROM fixture: {rom}")
        command += [f"-{kind}", str(rom)]
    with (OUT / "vice.log").open("w") as log:
        process = subprocess.Popen([*command, "-autostart", str(prg)], stdout=log, stderr=log)
        monitor = None
        try:
            monitor = wait_for_monitor("127.0.0.1", 6528, 20)
            deadline = time.monotonic() + 20
            while time.monotonic() < deadline:
                state = monitor.read_memory(0xc060, 0xc067)
                if state[0] == 0xa5:
                    assert state[1:4] == bytes([0xf2, 0x40, 0x59]), state.hex()
                    assert state[4] & 7 == 7, state.hex()
                    assert state[5:7] == bytes([0, 255]), state.hex()
                    assert state[7] == 0x34, state.hex()  # original flags restored after CHROUT
                    # CHROUT may use stack below SP and change arithmetic flags.
                    # The original frame and untouched high stack must survive.
                    assert monitor.read_memory(0x01f3, 0x01ff) == bytes(range(0xf3, 0x100))
                    assert monitor.read_memory(2, 0x1b) == bytes(range(0x40, 0x5a))
                    assert monitor.read_memory(0x0400, 0x07e7) == b'\x20' * 1000
                    assert [monitor.read_memory(a, a)[0] & 15 for a in
                            (0xd020, 0xd021, 0x0286)] == [14, 6, 14]
                    assert all(v & 15 == 14 for v in monitor.read_memory(0xd800, 0xdbe7))
                    expected_workspace = ([0, 0xa0, 0, 0xa0, 0, 0xa0, 0, 0xa0]
                                          if valid_workspace else [3, 8, 3, 8, 3, 8, 0, 0xa0])
                    assert monitor.read_memory(0x2d, 0x34) == bytes(expected_workspace)
                    print("VICE BASIC return passed: original frame, SP, ZP, map, interrupt mask, colours and cleared screen")
                    return
                monitor.resume()
                time.sleep(0.03)
            raise RuntimeError(f"BASIC return probe timed out: state={state.hex()}, context={monitor.read_memory(0x0400, 0x0407).hex()}, stub={monitor.read_memory(0x033c, 0x0343).hex()}")
        finally:
            if monitor:
                monitor.quit()
                monitor.close()
            if process.poll() is None:
                process.terminate()
            process.wait(timeout=10)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom-dir", type=Path, default=Path("/tmp/see64you-roms"))
    parser.add_argument("--valid-workspace", action="store_true")
    args = parser.parse_args()
    main(args.rom_dir, args.valid_workspace)
