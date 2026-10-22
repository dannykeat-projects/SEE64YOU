#!/usr/bin/env python3
"""Measure the real Editor root under the persistent-runtime architecture spike.

This is diagnostic-only. It retains the proven Launcher -> managed Editor
adoption and clean BASIC-return path, but removes normal Editor -> Publisher /
Settings PRG ownership transfer and application-state rehydration machinery.
"""
from __future__ import annotations

import os
import shlex
import sys
from pathlib import Path

import build_editor_overlay_fit as fit

ROOT = Path(__file__).resolve().parents[1]
CLIENT = ROOT / "client"
OUT = CLIENT / "build" / "persistent-editor-fit"

GROUPS: dict[str, tuple[str, ...]] = {
    "MODEL": (
        "src/editor.c",
        "src/editor_format.c",
        "src/ui_exit.c",
    ),
    "FILE": (
        "src/document.c",
        "src/storage/local.c",
        "src/ui.c",
        "src/menu.c",
    ),
    "RUNTIME": (
        "src/platform/module_runtime.c",
        "src/platform/transfer_capsule.c",
        "src/platform/module_session.c",
        "src/platform/module_session_read.c",
        "src/platform/session_lifecycle.c",
        "src/platform/session_journal.c",
        "src/platform/system_state.c",
        "src/platform/turbo.c",
        "src/platform/code_assets_validate.c",
        "src/platform/code_assets_session.c",
        "src/platform/code_assets_session_read.c",
        "src/platform/managed_editor.c",
        "src/platform/basic_return.c",
        "src/platform/basic_return_read.c",
        "src/platform/reu_runtime_write.c",
        "src/platform/managed_editor_exit.c",
        "src/platform/managed_editor_revoke.c",
    ),
    "PUBLICATION": (
        "diagnostics/persistent_runtime_fit_mode.c",
    ),
}
GROUP_IDS = {"MODEL": 1, "FILE": 2, "RUNTIME": 3, "PUBLICATION": 4}
BRIDGES = {
    "FILE": (
        "s64_editor_has_selection",
        "s64_editor_selection_start",
        "s64_editor_selection_end",
    ),
}

# Machinery needed only because Publisher/Settings currently become new PRGs.
REMOVED_TRANSITION_SOURCES = {
    "src/platform/managed_editor_restore.c",
    "src/platform/application_state_editor_decode.c",
    "src/platform/application_state_payload.c",
    "src/platform/managed_editor_state.c",
    "src/platform/application_state.c",
    "src/platform/application_state_wire.c",
    "src/platform/managed_editor_publish.c",
    "src/platform/managed_editor_prepare.c",
    "src/platform/managed_editor_publisher_assets.c",
    "src/platform/managed_editor_settings.c",
    "src/platform/managed_editor_publisher_capsule.c",
    "src/publisher/settings_session_publish.c",
    "src/publisher/settings_session_revoke.c",
    "src/publisher/settings_validate.c",
    "src/platform/dos_file.c",
    "src/platform/dos_reu.c",
    "src/platform/module_integrity.c",
    "src/platform/application_state_editor.c",
    "src/platform/application_state_build.c",
    "src/platform/module_transfer.c",
    "src/platform/module_transfer_stub.s",
}


def build() -> Path:
    db = fit.run(["make", "-pn", "managed-editor-fit"], cwd=CLIENT)
    values = fit.make_variables(db.stdout)
    c64u, runtime = fit.source_lists(db.stdout)
    code_asset_runtime = shlex.split(values.get("CODE_ASSET_RUNTIME_SOURCES", ""))

    sources = list(dict.fromkeys(
        [source for source in c64u + runtime + code_asset_runtime
         if source not in REMOVED_TRANSITION_SOURCES]
        + [
            "src/platform/code_overlay.c",
            "src/platform/managed_editor.c",
            "src/platform/managed_editor_exit.c",
            "src/platform/basic_return.c",
            "src/platform/basic_return_read.c",
            "src/platform/reu_runtime_write.c",
            "src/platform/managed_editor_revoke.c",
            "src/platform/basic_return_stub.s",
            "src/platform/crc32.c",
            "diagnostics/persistent_runtime_fit_mode.c",
        ]
    ))

    flags = shlex.split(values["CFLAGS"])
    if "-DS64_REU_BUFFER=1" not in flags:
        flags.append("-DS64_REU_BUFFER=1")
    flags += ["-DS64_MANAGED_OVERLAY=1", "-DS64_PERSISTENT_RUNTIME_FIT=1"]

    OUT.mkdir(parents=True, exist_ok=True)
    cfg = OUT / "persistent-editor-fit.cfg"
    cfg.write_text(
        (CLIENT / "cfg" / "editor-overlay-fit.cfg").read_text(encoding="utf-8")
        .replace("build/editor-overlay-fit/", "build/persistent-editor-fit/"),
        encoding="utf-8",
    )

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
        image_name="persistent-editor-fit",
    )

    end = fit.bss_end(program.with_suffix(".map"))
    headroom = fit.RESIDENT_BSS_LIMIT - end - 1
    print(f"Persistent Editor resident headroom: {headroom} bytes before $8C00")
    if headroom < 1024:
        raise fit.FitError(
            f"persistent Editor root has only {headroom} bytes before $8C00; "
            "architecture gate requires at least 1024"
        )
    print(
        "Persistent Editor root fit gate passed. Real Publisher/Settings assets "
        "and hardware mode cycles remain unverified."
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
