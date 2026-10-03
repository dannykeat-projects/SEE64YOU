#!/usr/bin/env python3
"""Exercise generated overlay veneers and return bridges using VICE."""
from __future__ import annotations

import argparse
import subprocess
import sys
import time
from pathlib import Path

sys.dont_write_bytecode = True
import build_editor_overlay_fit as fit

ROOT = Path(__file__).resolve().parents[1]
CLIENT = ROOT / "client"
OUT = CLIENT / "build" / "code-overlay-probe"

MODEL = """
#include <stdint.h>
#pragma code-name ("MODEL_CODE")
#pragma rodata-name ("MODEL_RODATA")
static const uint32_t mask = 0x12345678UL;
uint32_t __fastcall__ s64_overlay_model_s64_editor_selection_start(uint16_t first, uint32_t last)
{ return (last ^ mask) + first; }
uint16_t s64_overlay_model_s64_editor_selection_end(void) { return 0; }
uint8_t s64_overlay_model_s64_editor_has_selection(void) { return 0; }
"""
FILE = """
#include <stdint.h>
#pragma code-name ("FILE_CODE")
#pragma rodata-name ("FILE_RODATA")
extern uint32_t __fastcall__ s64_overlay_bridge_file_s64_editor_selection_start(uint16_t, uint32_t);
static const uint32_t mask = 0x87654321UL;
static uint16_t calls;
uint32_t __fastcall__ s64_overlay_file_s64_probe_file(uint16_t first, uint32_t last)
{ ++calls; return s64_overlay_bridge_file_s64_editor_selection_start(first, last) ^ mask ^ calls; }
"""
RUNTIME = """
#include <stdint.h>
#pragma code-name ("RUNTIME_CODE")
#pragma rodata-name ("RUNTIME_RODATA")
static const uint32_t mask = 0x11223344UL;
uint32_t __fastcall__ s64_overlay_runtime_s64_probe_runtime(uint32_t value)
{ return value ^ mask; }
"""
MAIN = """
#include <stdint.h>
#include <string.h>
#include "platform/code_overlay.h"
extern const uint8_t model_image[], model_image_end[];
extern const uint8_t file_image[], file_image_end[];
extern const uint8_t runtime_image[], runtime_image_end[];
extern uint32_t __fastcall__ s64_editor_selection_start(uint16_t, uint32_t);
extern uint32_t __fastcall__ s64_probe_file(uint16_t, uint32_t);
extern uint32_t __fastcall__ s64_probe_runtime(uint32_t);
static const uint8_t* images[3];
static S64ReuRuntimeControl control;
static S64ReuAllocation caches[3];
static uint16_t reads;
uint8_t s64_reu_load(uint8_t bank, uint16_t offset, void* target, uint16_t length)
{
    uint8_t i;
    uint32_t address = ((uint32_t)bank << 16) | offset;
    for (i = 0; i < 3; ++i) {
        if (address == caches[i].address && length == caches[i].length) {
            memcpy(target, images[i], length);
            ++reads;
            return 1;
        }
    }
    return 0;
}
static void require(uint8_t passed, uint8_t step)
{
    *(volatile uint8_t*)0xc060 = step;
    if (!passed) {
        *(volatile uint8_t*)0xc062 = step;
        *(volatile uint8_t*)0xc061 = 0x5a;
        for (;;) {}
    }
}
int main(void)
{
    uint8_t i;
    uint16_t before;
    uint32_t input;
    uint32_t expected;
    *(volatile uint8_t*)0xc061 = 0;
    images[0] = model_image; images[1] = file_image; images[2] = runtime_image;
    control.format_version = S64_REU_RUNTIME_FORMAT_VERSION;
    control.abi_version = S64_REU_RUNTIME_ABI_VERSION;
    control.session_id = 1;
    control.allocation_count = 3;
    control.next_free = S64_REU_RUNTIME_HEAP_START;
    caches[0].length = model_image_end - model_image;
    caches[1].length = file_image_end - file_image;
    caches[2].length = runtime_image_end - runtime_image;
    for (i = 0; i < 3; ++i) {
        caches[i].owner = S64_REU_OWNER_MODULE;
        caches[i].address = control.next_free;
        control.next_free += caches[i].length;
        control.allocations[i] = caches[i];
    }
    require(s64_code_overlay_configure(&control, caches,
        (void*)S64_CODE_OVERLAY_ADDRESS, S64_CODE_OVERLAY_CAPACITY, s64_reu_load), 1);
    for (i = 1; i <= 30; ++i) {
        input = 0xabcdef12UL + i;
        expected = (input ^ 0x12345678UL) + (0x3456u + i);
        require(s64_editor_selection_start(0x3456u + i, input) == expected, 2);
        before = reads;
        require(s64_editor_selection_start(0x3456u + i, input) == expected && reads == before, 3);
        require(s64_probe_file(0x3456u + i, input) == (expected ^ 0x87654321UL ^ i), 4);
        require(reads == before + 3u, 5);
        require(s64_probe_runtime(input) == (input ^ 0x11223344UL), 6);
    }
    *(volatile uint8_t*)0xc060 = 0x80;
    *(volatile uint8_t*)0xc061 = 0xa5;
    for (;;) {}
}
"""


def build() -> Path:
    OUT.mkdir(parents=True, exist_ok=True)
    cfg = OUT / "probe.cfg"
    cfg.write_text((CLIENT / "cfg/editor-overlay-fit.cfg").read_text().replace(
        "build/editor-overlay-fit/", "build/code-overlay-probe/"))
    wrappers = OUT / "wrappers.s"
    wrappers.write_text(fit.assembler_wrappers({
        "MODEL": list(fit.BRIDGES["FILE"]),
        "FILE": ["s64_probe_file"], "RUNTIME": ["s64_probe_runtime"],
    }))
    sources = []
    for name, content in (("model", MODEL), ("file", FILE), ("runtime", RUNTIME), ("main", MAIN)):
        source = OUT / f"{name}.c"
        source.write_text(content)
        sources.append(source)
    flags = ["-t", "c64", "-Os", "-I", "src", "-C", str(cfg)]
    blobs = OUT / "blobs.s"
    prg = OUT / "code-overlay-probe.prg"
    common_sources = [*sources, wrappers, CLIENT / "src/platform/code_overlay.c",
                      CLIENT / "src/platform/reu_runtime.c"]
    objects = []
    for source in common_sources:
        obj = OUT / f"{source.stem}.o"
        fit.run(["cl65", *flags, "-c", "-o", str(obj), str(source)], cwd=CLIENT)
        objects.append(str(obj))
    # Full blobs move resident BSS. Relink once more so embedded code contains
    # addresses from the final resident layout rather than the dummy layout.
    for initial in (True, False, False):
        lines = ['.segment "RODATA"']
        for group in ("model", "file", "runtime"):
            lines += [f".export _{group}_image, _{group}_image_end", f"_{group}_image:",
                      ".byte 0" if initial else f'.incbin "{OUT.as_posix()}/{group}.bin"',
                      f"_{group}_image_end:"]
        blobs.write_text("\n".join(lines) + "\n")
        blob_object = OUT / "blobs.o"
        fit.run(["cl65", *flags, "-c", "-o", str(blob_object), str(blobs)], cwd=CLIENT)
        fit.run(["cl65", *flags, "-o", str(prg), *objects, str(blob_object)], cwd=CLIENT)
    return prg


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-only", action="store_true")
    parser.add_argument("--port", type=int, default=6527)
    parser.add_argument("--vice-bin", default="x64sc")
    parser.add_argument("--rom-dir", type=Path, default=Path("/tmp/see64you-roms"))
    args = parser.parse_args()
    prg = build()
    if args.build_only:
        print(prg)
        return 0
    sys.path.insert(0, str(ROOT / "tools/vice"))
    from test_reu_probe import wait_for_monitor
    command = ["xvfb-run", "-a", args.vice_bin, "+sound", "-autostartprgmode", "1",
               "-binarymonitor", "-binarymonitoraddress", f"ip4://127.0.0.1:{args.port}"]
    for kind in ("kernal", "basic", "chargen"):
        if not (args.rom_dir / f"{kind}.rom").is_file():
            raise fit.FitError(f"Missing {kind} ROM in {args.rom_dir}; use the pinned Open ROMs from tools/test_reu_vice.sh")
        command += [f"-{kind}", str(args.rom_dir / f"{kind}.rom")]
    command += ["-autostart", str(prg)]
    with (OUT / "vice.log").open("w") as log:
        process = subprocess.Popen(command, stdout=log, stderr=log)
        monitor = None
        try:
            monitor = wait_for_monitor("127.0.0.1", args.port, 20)
            deadline = time.monotonic() + 20
            while time.monotonic() < deadline:
                state = monitor.read_memory(0xc060, 0xc062)
                if state[1] == 0xa5:
                    print("VICE overlay ABI passed: 30 swaps, 32-bit arguments/results, caller restoration, persistent data, cache hits")
                    return 0
                if state[1] == 0x5a:
                    raise fit.FitError(f"Overlay ABI probe failed at step {state[2]}")
                monitor.resume()
                time.sleep(0.03)
            raise fit.FitError(f"Overlay ABI probe timed out: {state.hex()}")
        finally:
            if monitor:
                monitor.quit()
                monitor.close()
            if process.poll() is None:
                process.terminate()
            process.wait(timeout=10)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except fit.FitError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
