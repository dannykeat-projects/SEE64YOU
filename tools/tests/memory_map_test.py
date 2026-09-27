#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "check_memory_headroom.py"


def run_case(end: str, minimum: int) -> subprocess.CompletedProcess[str]:
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "test.map"
        path.write_text(
            "Segment list:\n"
            "-------------\n"
            "Name Start End Size Align\n"
            f"BSS 00B000 00{end} 000001 00001\n",
            encoding="utf-8",
        )
        return subprocess.run(
            [
                sys.executable,
                str(SCRIPT),
                str(path),
                "--label",
                "test",
                "--min-headroom",
                str(minimum),
            ],
            check=False,
            capture_output=True,
            text=True,
        )


def main() -> int:
    passing = run_case("CA54", 256)
    if passing.returncode != 0 or "427 bytes free" not in passing.stdout:
        raise AssertionError(f"passing map rejected: {passing.stdout}{passing.stderr}")

    failing = run_case("CB80", 256)
    if failing.returncode == 0 or "below required" not in failing.stdout:
        raise AssertionError(f"low-headroom map accepted: {failing.stdout}{failing.stderr}")

    print("memory map guard tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
