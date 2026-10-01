#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "check_prg_extent.py"


def run_case(
    load_address: int,
    payload_size: int,
    max_end: int,
) -> subprocess.CompletedProcess[str]:
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "test.prg"
        path.write_bytes(
            bytes((load_address & 0xFF, (load_address >> 8) & 0xFF))
            + bytes(payload_size)
        )
        return subprocess.run(
            [
                sys.executable,
                str(SCRIPT),
                str(path),
                "--label",
                "test",
                "--load-address",
                "0x0801",
                "--max-end",
                hex(max_end),
            ],
            check=False,
            capture_output=True,
            text=True,
        )


def main() -> int:
    passing = run_case(0x0801, 0xBFFF, 0xC7FF)
    if passing.returncode != 0 or "$0801-$C7FF" not in passing.stdout:
        raise AssertionError(f"valid PRG rejected: {passing.stdout}{passing.stderr}")

    overlap = run_case(0x0801, 0xC000, 0xC7FF)
    if overlap.returncode == 0 or "must end at or below $C7FF" not in overlap.stdout:
        raise AssertionError(f"resident overlap accepted: {overlap.stdout}{overlap.stderr}")

    wrong_load = run_case(0x1000, 32, 0xC7FF)
    if wrong_load.returncode == 0 or "expected $0801" not in wrong_load.stdout:
        raise AssertionError(f"wrong load address accepted: {wrong_load.stdout}{wrong_load.stderr}")

    with tempfile.TemporaryDirectory() as directory:
        short = Path(directory) / "short.prg"
        short.write_bytes(b"\x01\x08")
        result = subprocess.run(
            [
                sys.executable,
                str(SCRIPT),
                str(short),
                "--label",
                "test",
                "--load-address",
                "0x0801",
                "--max-end",
                "0xC7FF",
            ],
            check=False,
            capture_output=True,
            text=True,
        )
        if result.returncode == 0 or "too short" not in result.stdout:
            raise AssertionError(f"empty payload accepted: {result.stdout}{result.stderr}")

    print("PRG extent guard tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
