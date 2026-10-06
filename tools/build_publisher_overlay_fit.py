#!/usr/bin/env python3
"""Measure Publisher overlays with destination ownership and transition code.

Includes the trusted-entry bootstrap and destination cache/session consumers.
Executable Publisher transitions remain gated.
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
    "src/publisher/plaintext.c", "src/publisher/preview.c",
    "src/platform/managed_publisher_document.c",
    "src/platform/application_state_payload.c",
    "src/platform/application_state_editor_decode.c",
    "src/platform/application_state_wire.c",
    "src/platform/application_state.c", "src/platform/application_state_build.c",
    "src/platform/module_transition.c",
    "src/platform/basic_return.c", "src/platform/basic_return_read.c",
)
NETWORK = (
    "src/platform/network.c", "src/crypto/sha256.c",
    "src/publisher/direct_auth.c", "src/publisher/standalone_direct.c",
    "src/publisher/settings_session.c", "src/publisher/settings_validate.c",
)
RESIDENT_SUPPORT = (
    "src/platform/code_overlay.c", "src/platform/reu_runtime.c",
    "src/platform/module_transfer.c", "src/platform/module_transfer_stub.s",
    "src/platform/module_arrival.c", "src/platform/basic_return_stub.s",
    "src/platform/crc32.c", "src/platform/code_overlay_bootstrap.c",
)
RUNTIME_SUPPORT = (
    "src/platform/code_assets_validate.c", "src/platform/code_assets_session.c",
    "src/platform/code_assets_session_read.c", "src/platform/managed_publisher.c",
    "src/platform/reu_runtime_write.c",
)
BRIDGES = {
    "NETWORK": ("s64_session_journal_encode",),
    "CONTENT": (
        "s64_session_journal_encode", "s64_system_state_set_mode",
        "s64_transfer_capsule_clear", "s64_transfer_capsule_store",
        "s64_transfer_capsule_load", "s64_module_runtime_prepare_transition",
        "s64_module_runtime_resume_interactive",
        "s64_reu_runtime_store_verified",
    ),
    "RUNTIME": ("s64_application_state_load_wire",),
}


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
    runtime = tuple(source for source in shlex.split(values["MANAGED_PUBLISHER_RUNTIME_SOURCES"])
                    if source != "src/platform/reu_runtime.c") + RUNTIME_SUPPORT
    legacy = {"src/platform/chain_editor.s", "src/publisher/handoff.c",
              "src/publisher/settings_bridge.c"}
    sources = list(dict.fromkeys([source for source in shlex.split(values["PUBLISHER_SOURCES"])
                                 if source not in legacy]
                                + list(runtime) + list(CONTENT) + list(NETWORK)
                                + list(RESIDENT_SUPPORT)))
    flags = shlex.split(values["CFLAGS"]) + [
        "-DS64_CODE_ASSET_PUBLISHER=1", "-DS64_MANAGED_OVERLAY=1", "-DS64_REU_BUFFER=1", "-DS64_STANDALONE_ONLY=1", "-DS64_DIRECT_PUBLISHER=1",
    ]
    program = fit.compile_overlay_layout(
        client, client / "build" / "publisher-overlay-fit", sources,
        {"CONTENT": CONTENT, "NETWORK": NETWORK, "RUNTIME": runtime},
        {"CONTENT": 1, "NETWORK": 2, "RUNTIME": 3}, BRIDGES,
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
    print("Publisher bootstrap and S64Q consumers included in measured root/RUNTIME.")


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
