#!/usr/bin/env python3
"""Prove both irreversible transfer entries restore cc65's installed IRQ."""
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "client/build/module-transfer-probe"
sys.path.insert(0, str(ROOT / "tools/vice"))
from test_reu_probe import wait_for_monitor


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    hooks = OUT / "hooks.s"
    hooks.write_text('''.setcpu "6502"
.constructor capture, 31
.interruptor handler, 10
.segment "CODE"
capture:
    lda $0314
    sta $c060
    lda $0315
    sta $c061
    rts
handler:
    clc
    rts
''')
    # Landing observes IRQ before destination startup can install another one.
    landing = [0xad, 0x14, 3, 0x8d, 0x64, 0xc0,
               0xad, 0x15, 3, 0x8d, 0x65, 0xc0,
               0xa9, 0xa5, 0x8d, 0x66, 0xc0, 0x4c, 0x11, 0xc0]
    source = OUT / "main.c"
    source.write_text('''#include <string.h>
extern const unsigned char s64_module_transfer_stub_image[], s64_module_transfer_stub_end[];
void s64_module_transfer_stub_enter(void);
void s64_module_transfer_capsule_enter(void);
static const unsigned char landing[] = {''' + ','.join(map(str, landing)) + '''};
int main(void) {
    volatile unsigned char* reu = (volatile unsigned char*)0xdf00;
    *(volatile unsigned int*)0xc062 = *(volatile unsigned int*)0x0314;
    *(volatile unsigned char*)0xc066 = 0;
    memcpy((void*)0xc000, landing, sizeof(landing));
#ifdef CAPSULE
    reu[2]=0; reu[3]=0xc0; reu[4]=2; reu[5]=0; reu[6]=3;
    reu[7]=sizeof(landing); reu[8]=0; reu[10]=0; reu[1]=0x90;
    *(unsigned long*)0xc832=0x030000;
    *(unsigned int*)0xc826=0xc000;
    *(unsigned int*)0xc828=0xc000;
    *(unsigned int*)0xc82a=sizeof(landing)+2;
    *(unsigned char*)0xc806=2;
    memset((void*)0xc000, 0, sizeof(landing));
    s64_module_transfer_capsule_enter();
#else
    /* Generic entry must tear down even when its staged landing does no DMA. */
    memcpy((void*)0x0700, landing, sizeof(landing));
    s64_module_transfer_stub_enter();
#endif
    return 1;
}
''')
    for capsule in (False, True):
        prg = OUT / "probe.prg"
        subprocess.run(["cl65", "-t", "c64", "-Os", *(["-DCAPSULE"] if capsule else []),
                        "-o", str(prg), str(source), str(hooks),
                        str(ROOT / "client/src/platform/module_transfer_stub.s")], check=True)
        command = ["xvfb-run", "-a", "x64sc", "+sound", "-reu", "-reusize", "16384",
                   "-autostartprgmode", "1", "-binarymonitor",
                   "-binarymonitoraddress", "ip4://127.0.0.1:6529"]
        for kind in ("kernal", "basic", "chargen"):
            command += [f"-{kind}", str(Path('/tmp/see64you-roms') / f'{kind}.rom')]
        with (OUT / "vice.log").open('w') as log:
            process = subprocess.Popen([*command, '-autostart', str(prg)], stdout=log, stderr=log)
            monitor = None
            try:
                monitor = wait_for_monitor('127.0.0.1', 6529, 20)
                deadline = time.monotonic() + 20
                while time.monotonic() < deadline:
                    state = monitor.read_memory(0xc060, 0xc066)
                    if state[6] == 0xa5:
                        assert state[:2] != state[2:4], state.hex()
                        assert state[:2] == state[4:6], state.hex()
                        if capsule:
                            assert monitor.read_memory(0x02a7, 0x02a9) == bytes([0x53, 0x36, 2])
                        print('VICE transfer IRQ teardown passed:', 'capsule DMA' if capsule else 'generic')
                        break
                    monitor.resume()
                    time.sleep(0.03)
                else:
                    raise RuntimeError(f'transfer timed out: {state.hex()}')
            finally:
                if monitor:
                    monitor.quit()
                    monitor.close()
                if process.poll() is None:
                    process.terminate()
                process.wait(timeout=10)


if __name__ == '__main__':
    main()
