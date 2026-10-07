#!/usr/bin/env python3
"""Measure the first real Publisher slice inside the persistent Editor runtime.

Diagnostic only. This keeps the proven Launcher -> Editor adoption/final BASIC
return model, adds the durable Publisher context to the same resident owner, and
links real Publisher state/auth/settings responsibilities as independent
overlays sharing the existing $8C00-$C7FF slot.
"""
from __future__ import annotations

import os
import shlex
import sys
from pathlib import Path

import build_editor_overlay_fit as fit
import build_persistent_editor_fit as editor_fit

ROOT = Path(__file__).resolve().parents[1]
CLIENT = ROOT / "client"
OUT = CLIENT / "build" / "persistent-publisher-slice-fit"

EDITOR_GROUPS = editor_fit.GROUPS
PUBLISHER_GROUPS: dict[str, tuple[str, ...]] = {
    "PCONTENT": (
        "src/publisher/destination.c",
        "src/publisher/state.c",
    ),
    "PNETWORK": (
        "src/platform/network.c",
        "src/crypto/sha256.c",
        "src/publisher/direct_auth.c",
        "src/publisher/standalone_direct.c",
    ),
    "PRUNTIME": (
        "src/publisher/settings_validate.c",
        "src/publisher/settings_exchange.c",
    ),
}
GROUPS = {**EDITOR_GROUPS, **PUBLISHER_GROUPS}
GROUP_IDS = {
    "MODEL": 1,
    "FILE": 2,
    "RUNTIME": 3,
    "PUBLICATION": 4,
    "PCONTENT": 1,
    "PNETWORK": 2,
    "PRUNTIME": 3,
}
BRIDGES = dict(editor_fit.BRIDGES)


def config_text() -> str:
    memories = [
        ("MODEL", "model.bin"),
        ("FILE", "file.bin"),
        ("RUNTIME", "runtime.bin"),
        ("PUBLICATION", "publication.bin"),
        ("PCONTENT", "pcontent.bin"),
        ("PNETWORK", "pnetwork.bin"),
        ("PRUNTIME", "pruntime.bin"),
    ]
    memory_lines = "\n".join(
        f'    {name}: file = "build/persistent-publisher-slice-fit/{filename}", '
        'start = $8C00, size = $3C00;'
        for name, filename in memories
    )
    segment_lines = "\n".join(
        f"    {name}_CODE: load = {name}, type = ro, define = yes;\n"
        f"    {name}_RODATA: load = {name}, type = ro, define = yes;"
        for name, _ in memories
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
{memory_lines}
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
{segment_lines}
}}
FEATURES {{
    CONDES: type = constructor, label = __CONSTRUCTOR_TABLE__, count = __CONSTRUCTOR_COUNT__, segment = ONCE;
    CONDES: type = destructor, label = __DESTRUCTOR_TABLE__, count = __DESTRUCTOR_COUNT__, segment = RODATA;
    CONDES: type = interruptor, label = __INTERRUPTOR_TABLE__, count = __INTERRUPTOR_COUNT__, segment = RODATA, import = __CALLIRQ__;
}}
"""


def build() -> Path:
    db = fit.run(["make", "-pn", "managed-editor-fit"], cwd=CLIENT)
    values = fit.make_variables(db.stdout)
    c64u, runtime = fit.source_lists(db.stdout)
    code_asset_runtime = shlex.split(values.get("CODE_ASSET_RUNTIME_SOURCES", ""))

    sources = list(dict.fromkeys(
        [source for source in c64u + runtime + code_asset_runtime
         if source not in editor_fit.REMOVED_TRANSITION_SOURCES]
        + [
            "src/platform/code_overlay.c",
            "src/platform/code_overlay_namespace.c",
            "src/platform/managed_editor.c",
            "src/platform/managed_editor_exit.c",
            "src/platform/basic_return.c",
            "src/platform/basic_return_read.c",
            "src/platform/reu_runtime_write.c",
            "src/platform/managed_editor_revoke.c",
            "src/platform/basic_return_stub.s",
            "src/platform/crc32.c",
            "src/publisher/persistent_mode.c",
            "diagnostics/persistent_publisher_services.c",
            "diagnostics/persistent_runtime_fit_mode.c",
            *[source for group in PUBLISHER_GROUPS.values() for source in group],
        ]
    ))

    flags = shlex.split(values["CFLAGS"])
    if "-DS64_REU_BUFFER=1" not in flags:
        flags.append("-DS64_REU_BUFFER=1")
    flags += ["-DS64_MANAGED_OVERLAY=1", "-DS64_PERSISTENT_RUNTIME_FIT=1"]

    OUT.mkdir(parents=True, exist_ok=True)
    cfg = OUT / "persistent-publisher-slice-fit.cfg"
    cfg.write_text(config_text(), encoding="ascii", newline="\n")

    program = fit.compile_overlay_layout(
        CLIENT,
        OUT,
        sources,
        GROUPS,
        GROUP_IDS,
        BRIDGES,
        cfg,
        flags,
        compiler=os.environ.get("CL65") or values["CL65"] or "cl65",
        image_name="persistent-publisher-slice-fit",
    )

    end = fit.bss_end(program.with_suffix(".map"))
    headroom = fit.RESIDENT_BSS_LIMIT - end - 1
    print(f"Persistent Editor + Publisher slice headroom: {headroom} bytes before $8C00")
    if headroom < 1024:
        raise fit.FitError(
            f"persistent Editor + Publisher slice has only {headroom} bytes "
            "before $8C00; requires at least 1024"
        )

    for name in PUBLISHER_GROUPS:
        size = (OUT / f"{name.lower()}.bin").stat().st_size
        print(f"Real Publisher {name} slice: {size} / {fit.OVERLAY_LIMIT} bytes")

    print(
        "Persistent Publisher slice fit passed: real remote-state, HMAC/SHA-256 "
        "and settings-record logic coexist with the persistent Editor owner. "
        "Full destination/network/UI migration remains gated."
    )
    return program


def main() -> int:
    try:
        program = build()
    except fit.FitError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    print(program)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
