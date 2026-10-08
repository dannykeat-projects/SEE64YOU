#!/usr/bin/env python3
"""Measure the first two persistent Settings overlays."""
from __future__ import annotations

import re
import shlex
import sys
from pathlib import Path

sys.dont_write_bytecode = True
import build_editor_overlay_fit as fit

ROOT = Path(__file__).resolve().parents[1]
CLIENT = ROOT / "client"
OUT = CLIENT / "build" / "persistent-settings-slice-fit"

GROUPS = {
    "SUI": ("src/publisher/persistent_settings_ui.c",),
    "SSTORE": (
        "src/publisher/settings_store.c",
        "src/publisher/settings_exchange.c",
        "src/publisher/settings_validate.c",
        "src/publisher/destination.c",
        "src/publisher/persistent_settings_control.c",
    ),
}
GROUP_IDS = {"SUI": 1, "SSTORE": 2}

MAIN = r"""
#include <stdint.h>
#include "publisher/settings_exchange.h"
int main(void)
{
    S64SettingsExchange settings;
    s64_settings_exchange_init(&settings);
    return settings.version == S64_SETTINGS_VERSION ? 0 : 1;
}
"""

def config_text() -> str:
    groups = ("SUI", "SSTORE")
    memory = "\n".join(
        f'    {g}: file = "build/persistent-settings-slice-fit/{g.lower()}.bin", '
        'start = $8C00, size = $3C00;'
        for g in groups
    )
    segments = "\n".join(
        f"    {g}_CODE: load = {g}, type = ro, define = yes;\n"
        f"    {g}_RODATA: load = {g}, type = ro, define = yes;"
        for g in groups
    )
    return f"""FEATURES {{ STARTADDRESS: default = $0801; }}
SYMBOLS {{
    __LOADADDR__: type = import;
    __EXEHDR__: type = import;
    __STACKSIZE__: type = weak, value = $0400;
    __HIMEM__: type = weak, value = $D000;
}}
MEMORY {{
    ZP: file = "", define = yes, start = $0002, size = $001A;
    LOADADDR: file = %O, start = %S - 2, size = $0002;
    HEADER: file = %O, define = yes, start = %S, size = $000D;
    MAIN: file = %O, define = yes, start = __HEADER_LAST__, size = __HIMEM__ - __HEADER_LAST__;
    BSS: file = "", start = __ONCE_RUN__, size = $8C00 - __ONCE_RUN__;
{memory}
}}
SEGMENTS {{
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
{segments}
}}
FEATURES {{
    CONDES: type = constructor, label = __CONSTRUCTOR_TABLE__, count = __CONSTRUCTOR_COUNT__, segment = ONCE;
    CONDES: type = destructor, label = __DESTRUCTOR_TABLE__, count = __DESTRUCTOR_COUNT__, segment = RODATA;
    CONDES: type = interruptor, label = __INTERRUPTOR_TABLE__, count = __INTERRUPTOR_COUNT__, segment = RODATA, import = __CALLIRQ__;
}}
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
        "-DS64_NATIVE_PETSCII=1",
        "-DS64_REU_BUFFER=1",
        "-DS64_PERSISTENT_RUNTIME_FIT=1",
        "-DS64_SETTINGS_CONFIG_ONLY=1",
    ]

    grouped_names: dict[str, list[str]] = {}
    sources: list[Path] = []
    for group, paths in GROUPS.items():
        names: list[str] = []
        for path in paths:
            names.extend(fit.public_s64_definitions(
                (CLIENT / path).read_text(encoding="utf-8")))
        grouped_names[group] = list(dict.fromkeys(names))
        for path in paths:
            out = generated / f"{group.lower()}_{Path(path).stem}.c"
            out.write_text(fit.generated_translation_unit(
                group, path, grouped_names[group], generated, CLIENT, bridges={}))
            sources.append(out)

    main = generated / "main.c"
    main.write_text(MAIN)
    sources.append(main)

    sources.extend([
        CLIENT / "src/platform/code_overlay.c",
        CLIENT / "src/platform/reu.c",
        CLIENT / "src/platform/reu_dma.s",
        CLIENT / "src/platform/reu_runtime.c",
        CLIENT / "src/platform/reu_runtime_build.c",
        CLIENT / "src/platform/reu_runtime_write.c",
        CLIENT / "src/platform/dos.c",
        CLIENT / "src/charset.c",
    ])

    wrappers = generated / "wrappers.s"
    wrappers.write_text(
        fit.assembler_wrappers(grouped_names, group_ids=GROUP_IDS, bridges={}))
    sources.append(wrappers)

    objects: list[Path] = []
    for index, source in enumerate(sources):
        obj = objects_dir / f"{index:03d}_{source.stem}.o"
        fit.run(["cl65", *flags, "-c", "-o", str(obj), str(source)], cwd=CLIENT)
        objects.append(obj)

    prg = OUT / "persistent-settings-slice-fit.prg"
    map_path = OUT / "persistent-settings-slice-fit.map"
    fit.run(
        ["cl65", *flags, "-m", str(map_path), "-o", str(prg),
         *(str(obj) for obj in objects)],
        cwd=CLIENT,
    )

    settings_main_obj = objects_dir / "settings_main_persistent_compile.o"
    fit.run(
        ["cl65", *flags, "-c", "-o", str(settings_main_obj),
         "src/publisher/settings_main.c"],
        cwd=CLIENT,
    )
    print("Persistent settings_main.c compile passed")
    dump = fit.run(["od65", "--dump-segments", str(settings_main_obj)], cwd=CLIENT).stdout
    segment_sizes: dict[str, int] = {}
    current = None
    for line in dump.splitlines():
        match = re.match(r'\s*Name:\s*"([^"]+)"', line)
        if match:
            current = match.group(1)
            continue
        match = re.match(r'\s*Size:\s*(0x[0-9A-Fa-f]+|[0-9]+)', line)
        if match and current:
            segment_sizes[current] = int(match.group(1), 0)
            current = None
    interesting = ("CODE", "RODATA", "DATA", "BSS")
    if segment_sizes:
        print("Persistent settings_main.c object segments: " + ", ".join(
            f"{name}={segment_sizes.get(name, 0)}" for name in interesting))
    else:
        print("Persistent settings_main.c object segment sizes unavailable")

    for group in ("SUI", "SSTORE"):
        size = (OUT / f"{group.lower()}.bin").stat().st_size
        if not size or size > fit.OVERLAY_LIMIT:
            raise fit.FitError(f"{group} asset size invalid: {size}")
        print(f"{group} overlay: {size} / {fit.OVERLAY_LIMIT}")

    end = fit.bss_end(map_path)
    print(f"Persistent Settings slice BSS ends at 0x{end:04X}")
    if end >= fit.RESIDENT_BSS_LIMIT:
        raise fit.FitError("persistent Settings fit resident BSS overlaps slot")
    return prg

if __name__ == "__main__":
    try:
        build()
    except fit.FitError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
