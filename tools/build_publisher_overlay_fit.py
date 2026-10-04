#!/usr/bin/env python3
"""Measure Publisher overlays with destination ownership and transition code.

This is a link/fit diagnostic, without trusted-entry or cache bindings.
"""
from __future__ import annotations

import argparse
import os
import re
import shlex
import sys
from pathlib import Path

import build_editor_overlay_fit as fit


CONTENT = (
    "src/publisher/destination.c", "src/publisher/state.c",
    "src/publisher/handoff.c", "src/publisher/plaintext.c",
    "src/publisher/preview.c", "src/publisher/settings_bridge.c",
)
NETWORK = (
    "src/platform/network.c", "src/crypto/sha256.c",
    "src/publisher/direct_auth.c", "src/publisher/standalone_direct.c",
)
RESIDENT_SUPPORT = (
    "src/platform/code_overlay.c", "src/platform/module_transition.c",
    "src/platform/module_transfer.c", "src/platform/module_transfer_stub.s",
    "src/platform/module_arrival.c", "src/platform/basic_return.c",
    "src/platform/basic_return_read.c", "src/platform/basic_return_stub.s",
    "src/platform/crc32.c", "diagnostics/managed_publisher_state_fit.c",
)


def compile_publisher_overlay_fit(client: Path, *, make: str = "make",
                                  cl65: str | None = None, od65: str = "od65") -> None:
    database = fit.run([make, "-pn", "managed-publisher-fit"], cwd=client).stdout
    values = dict(re.findall(
        r"^(PUBLISHER_SOURCES|MANAGED_PUBLISHER_RUNTIME_SOURCES|CFLAGS|CL65)\s*:?=\s*(.*)$",
        database, re.MULTILINE,
    ))
    for key in ("PUBLISHER_SOURCES", "MANAGED_PUBLISHER_RUNTIME_SOURCES", "CFLAGS", "CL65"):
        if key not in values:
            raise fit.FitError(f"Missing make variable: {key}")
    runtime = tuple(shlex.split(values["MANAGED_PUBLISHER_RUNTIME_SOURCES"]))
    sources = list(dict.fromkeys(shlex.split(values["PUBLISHER_SOURCES"])
                                + list(runtime) + list(RESIDENT_SUPPORT)))
    flags = shlex.split(values["CFLAGS"]) + [
        "-DS64_CODE_ASSET_PUBLISHER=1", "-DS64_REU_BUFFER=1", "-DS64_STANDALONE_ONLY=1", "-DS64_DIRECT_PUBLISHER=1",
    ]
    program = fit.compile_overlay_layout(
        client, client / "build" / "publisher-overlay-fit", sources,
        {"CONTENT": CONTENT, "NETWORK": NETWORK, "RUNTIME": runtime + ("src/platform/basic_return_read.c",)},
        {"CONTENT": 1, "NETWORK": 2, "RUNTIME": 3}, {},
        client / "cfg" / "publisher-overlay-fit.cfg", flags,
        compiler=cl65 or os.environ.get("CL65") or values["CL65"],
        od65=od65, image_name="publisher-overlay-fit",
    )
    headroom = fit.RESIDENT_BSS_LIMIT - fit.bss_end(program.with_suffix(".map")) - 1
    if headroom < 1024:
        raise fit.FitError(f"Publisher resident headroom is {headroom}; requires 1024 bytes")
    print(f"Publisher resident headroom: {headroom} bytes (minimum 1024)")
    generator = fit.ROOT / "tools" / "generate_code_asset_manifest.py"
    manifest = program.parent / "S64POVL.MNF"
    for extra in ([], ["--validate", str(manifest)]):
        fit.run([sys.executable, str(generator), str(program.parent),
                 "--module", "publisher", "--root", str(program), *extra], cwd=client)
    print("Exact Publisher asset manifest generated and revalidated (diagnostic only).")
    # Compile the producer/consumer specialization without claiming root fit.
    # Trusted bootstrap and runtime-overlay placement are still separate gates.
    codec_dir = program.parent / "asset-codecs"
    codec_dir.mkdir(exist_ok=True)
    for source in ("code_assets", "code_assets_validate", "code_assets_session",
                   "code_assets_session_build", "code_assets_session_read",
                   "code_overlay_bootstrap"):
        fit.run([cl65 or os.environ.get("CL65") or values["CL65"], *flags,
                 "-c", "-o", str(codec_dir / (source + ".o")),
                 str(client / "src" / "platform" / (source + ".c"))], cwd=client)
    print("Publisher asset codecs compile; destination-root integration remains gated.")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client", type=Path, default=fit.CLIENT)
    parser.add_argument("--make", default="make")
    parser.add_argument("--cl65")
    parser.add_argument("--od65", default="od65")
    args = parser.parse_args()
    try:
        compile_publisher_overlay_fit(args.client.resolve(), make=args.make,
                                      cl65=args.cl65, od65=args.od65)
    except fit.FitError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
