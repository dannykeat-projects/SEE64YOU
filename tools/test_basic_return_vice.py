#!/usr/bin/env python3
"""Verify the transient SYS return capsule with a synthetic original context.

This proves restoration mechanics, not real launcher capture/session binding.
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


def main(rom_dir: Path) -> None:
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
extern const unsigned char s64_basic_return_stub_image[], s64_basic_return_stub_end[];
void s64_basic_return_stub_enter(void);
static const unsigned char landing[] = {''' + ','.join(map(str, landing)) + '''};
int main(void) {
    unsigned int i;
    unsigned char* context = (unsigned char*)0x0400;
    context[0] = 0x37; context[1] = 0xf0; context[2] = 0x24;
    for (i=0; i<26; ++i) context[3+i] = (unsigned char)(0x40+i);
    for (i=0; i<256; ++i) context[29+i] = (unsigned char)i;
    context[29+0xf1] = 0xff; context[29+0xf2] = 0xbf;
    memcpy((void*)0xc000, landing, sizeof(landing));
    *(volatile unsigned char*)0xc060 = 0;
    memcpy((void*)0x033c, s64_basic_return_stub_image,
           s64_basic_return_stub_end-s64_basic_return_stub_image);
    s64_basic_return_stub_enter();
    return 1;
}
''')
    prg = OUT / "probe.prg"
    subprocess.run(["cl65", "-t", "c64", "-Os", "-o", str(prg), str(source),
                    str(ROOT / "client/src/platform/basic_return_stub.s")], check=True)
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
                    print("VICE BASIC return passed: original frame, SP, ZP, map, interrupt mask and cleared screen")
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
    main(parser.parse_args().rom_dir)
