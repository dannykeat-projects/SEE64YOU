#!/usr/bin/env python3
"""Prove SUI screen-model return followed by resident conio rendering in VICE."""
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
OUT = CLIENT / "build" / "persistent-settings-screen-vice"

MAIN = r"""
#include <stdint.h>
#include <string.h>

#include "platform/code_overlay.h"
#include "platform/reu.h"
#include "platform/reu_runtime.h"
#include "publisher/persistent_settings_render.h"
#include "publisher/persistent_settings_ui.h"
#include "publisher/settings_exchange.h"

extern const uint8_t sui_image[], sui_image_end[];

static S64ReuRuntimeControl control;
static S64ReuAllocation asset;
static S64SettingsExchange settings;
static S64PersistentSettingsScreen screen;

static uint8_t overlay_reader(
    uint8_t bank, uint16_t offset, void* target, uint16_t length)
{
    return s64_reu_load(bank, offset, target, length);
}

static void require(uint8_t passed, uint8_t step)
{
    *(volatile uint8_t*)0xc060 = step;
    if (!passed) {
        *(volatile uint8_t*)0xc062 = step;
        *(volatile uint8_t*)0xc061 = 0x5au;
        for (;;) {}
    }
}

int main(void)
{
    uint8_t slot;

    *(volatile uint8_t*)0xc061 = 0u;
    require(s64_reu_probe() && s64_reu_probe_banks() == 256u, 1u);

    s64_reu_runtime_init(&control, 0x0202u, 0x445566aau);
    require(s64_reu_runtime_allocate(
        &control, S64_REU_OWNER_MODULE,
        (uint16_t)(sui_image_end - sui_image), 256u, &asset), 2u);
    slot = (uint8_t)(control.allocation_count - 1u);
    asset.flags = 0x76u;
    control.allocations[slot].flags = asset.flags;
    require(s64_reu_store(
        (uint8_t)(asset.address >> 16),
        (uint16_t)asset.address,
        sui_image,
        (uint16_t)(sui_image_end - sui_image)), 3u);
    require(s64_code_overlay_configure(
        &control, &asset, 1u,
        (void*)S64_CODE_OVERLAY_ADDRESS,
        S64_CODE_OVERLAY_CAPACITY, overlay_reader), 4u);

    s64_settings_exchange_init(&settings);
    settings.destination_type = (uint8_t)S64_DESTINATION_STANDALONE;
    settings.allow_insecure_http = 1u;
    strcpy(settings.base_url, "http://example.test");

    require(s64_persistent_settings_screen(
        &settings, 0u, "BASIC READY", &screen), 5u);
    require(screen.line_count == 11u
        && screen.title
        && screen.lines[0].row == 3u
        && screen.lines[0].value == settings.base_url, 6u);
    s64_persistent_settings_render_screen(&screen);

    require(s64_persistent_settings_screen(
        &settings, 1u, "ADVANCED READY", &screen), 7u);
    require(screen.line_count == 18u
        && screen.lines[17].row == 23u, 8u);
    s64_persistent_settings_render_screen(&screen);

    *(volatile uint8_t*)0xc060 = 0x80u;
    *(volatile uint8_t*)0xc061 = 0xa5u;
    *(volatile uint8_t*)0xc062 = 0u;
    for (;;) {}
}
"""

def config_text() -> str:
    return """FEATURES { STARTADDRESS: default = $0801; }
SYMBOLS {
    __LOADADDR__: type = import;
    __EXEHDR__: type = import;
    __STACKSIZE__: type = weak, value = $0400;
    __HIMEM__: type = weak, value = $D000;
}
MEMORY {
    ZP: file = "", define = yes, start = $0002, size = $001A;
    LOADADDR: file = %O, start = %S - 2, size = $0002;
    HEADER: file = %O, define = yes, start = %S, size = $000D;
    MAIN: file = %O, define = yes, start = __HEADER_LAST__, size = __HIMEM__ - __HEADER_LAST__;
    BSS: file = "", start = __ONCE_RUN__, size = $8C00 - __ONCE_RUN__;
    SUI: file = "build/persistent-settings-screen-vice/sui.bin", start = $8C00, size = $3C00;
}
SEGMENTS {
    ZEROPAGE: load = ZP, type = zp;
    LOADADDR: load = LOADADDR, type = ro;
    EXEHDR: load = HEADER, type = ro;
    STARTUP: load = MAIN, type = ro;
    LOWCODE: load = MAIN, type = ro, optional = yes;
    CODE: load = MAIN, type = ro;
    RODATA: load = MAIN, type = ro;
    DATA: load = MAIN, type = rw;
    INIT: load = MAIN, type = rw;
    ONCE: load = MAIN, type = ro, define = yes;
    BSS: load = BSS, type = bss, define = yes;
    SUI_CODE: load = SUI, type = ro, define = yes;
    SUI_RODATA: load = SUI, type = ro, define = yes;
}
FEATURES {
    CONDES: type = constructor, label = __CONSTRUCTOR_TABLE__, count = __CONSTRUCTOR_COUNT__, segment = ONCE;
    CONDES: type = destructor, label = __DESTRUCTOR_TABLE__, count = __DESTRUCTOR_COUNT__, segment = RODATA;
    CONDES: type = interruptor, label = __INTERRUPTOR_TABLE__, count = __INTERRUPTOR_COUNT__, segment = RODATA, import = __CALLIRQ__;
}
"""

def build() -> Path:
    OUT.mkdir(parents=True, exist_ok=True)
    generated = OUT / "generated"
    objects_dir = OUT / "objects"
    generated.mkdir(parents=True, exist_ok=True)
    objects_dir.mkdir(parents=True, exist_ok=True)

    cfg = OUT / "probe.cfg"
    cfg.write_text(config_text(), encoding="ascii", newline="\n")
    flags = [
        "-t", "c64", "-Os", "-I", "src", "-C", str(cfg),
        "-DS64_NATIVE_PETSCII=1", "-DS64_REU_BUFFER=1",
        "-DS64_PERSISTENT_RUNTIME_FIT=1",
    ]

    source_path = "src/publisher/persistent_settings_ui.c"
    names = fit.public_s64_definitions(
        (CLIENT / source_path).read_text(encoding="utf-8"))
    generated_ui = generated / "sui.c"
    generated_ui.write_text(fit.generated_translation_unit(
        "SUI", source_path, names, generated, CLIENT, bridges={}))

    main = generated / "main.c"
    main.write_text(MAIN)

    sources = [
        generated_ui,
        main,
        CLIENT / "src/publisher/persistent_settings_render.c",
        CLIENT / "src/platform/code_overlay.c",
        CLIENT / "src/platform/reu.c",
        CLIENT / "src/platform/reu_dma.s",
        CLIENT / "src/platform/reu_runtime.c",
        CLIENT / "src/platform/reu_runtime_build.c",
        CLIENT / "src/platform/reu_runtime_write.c",
        CLIENT / "src/publisher/settings_exchange.c",
        CLIENT / "src/publisher/settings_validate.c",
    ]

    wrappers = generated / "wrappers.s"
    wrappers.write_text(fit.assembler_wrappers(
        {"SUI": names}, group_ids={"SUI": 1}, bridges={}))
    sources.append(wrappers)

    objects = []
    for index, source in enumerate(sources):
        obj = objects_dir / f"{index:03d}_{source.stem}.o"
        fit.run(["cl65", *flags, "-c", "-o", str(obj), str(source)], cwd=CLIENT)
        objects.append(obj)

    blobs = generated / "blobs.s"
    blob_obj = objects_dir / "blobs.o"
    prg = OUT / "persistent-settings-screen-vice.prg"
    map_path = OUT / "persistent-settings-screen-vice.map"

    for initial in (True, False, False):
        blobs.write_text(
            '.segment "RODATA"\n'
            '.export _sui_image, _sui_image_end\n'
            '_sui_image:\n'
            + ('.byte 0\n' if initial
               else f'.incbin "{OUT.as_posix()}/sui.bin"\n')
            + '_sui_image_end:\n'
        )
        fit.run(["cl65", *flags, "-c", "-o", str(blob_obj), str(blobs)], cwd=CLIENT)
        fit.run(["cl65", *flags, "-m", str(map_path), "-o", str(prg),
                 *(str(obj) for obj in objects), str(blob_obj)], cwd=CLIENT)

    size = (OUT / "sui.bin").stat().st_size
    if not size or size > fit.OVERLAY_LIMIT:
        raise fit.FitError(f"SUI screen asset size invalid: {size}")
    print(f"SUI screen asset: {size} / {fit.OVERLAY_LIMIT}")
    end = fit.bss_end(map_path)
    print(f"Persistent Settings screen VICE probe BSS ends at 0x{end:04X}")
    if end >= fit.RESIDENT_BSS_LIMIT:
        raise fit.FitError("persistent Settings screen probe resident BSS overlaps slot")
    return prg

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-only", action="store_true")
    parser.add_argument("--port", type=int, default=6538)
    parser.add_argument("--vice-bin", default="x64sc")
    parser.add_argument("--rom-dir", type=Path, default=Path("/tmp/see64you-roms"))
    args = parser.parse_args()

    prg = build()
    if args.build_only:
        print(prg)
        return 0

    sys.path.insert(0, str(ROOT / "tools" / "vice"))
    from test_reu_probe import wait_for_monitor

    command = [
        "xvfb-run", "-a", args.vice_bin, "+sound", "-warp",
        "-reu", "-reusize", "16384", "-autostartprgmode", "1",
        "-binarymonitor", "-binarymonitoraddress",
        f"ip4://127.0.0.1:{args.port}",
    ]
    for kind in ("kernal", "basic", "chargen"):
        rom = args.rom_dir / f"{kind}.rom"
        if not rom.is_file():
            raise fit.FitError(f"Missing pinned Open ROM: {rom}")
        command += [f"-{kind}", str(rom)]
    command += ["-autostart", str(prg)]

    with (OUT / "vice.log").open("w") as log:
        process = subprocess.Popen(command, stdout=log, stderr=log)
        monitor = None
        try:
            monitor = wait_for_monitor("127.0.0.1", args.port, 90)
            deadline = time.monotonic() + 90
            state = b""
            while time.monotonic() < deadline:
                state = monitor.read_memory(0xc060, 0xc062)
                if state[1] == 0xa5:
                    print(
                        "VICE persistent Settings screen passed: SUI returned "
                        "screen descriptors and resident conio rendered after return"
                    )
                    return 0
                if state[1] == 0x5a:
                    raise fit.FitError(
                        f"persistent Settings screen failed at step {state[2]}")
                monitor.resume()
                time.sleep(0.03)
            raise fit.FitError(
                f"persistent Settings screen timed out: {state.hex()}")
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
