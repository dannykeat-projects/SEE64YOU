#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path

BSS_RE = re.compile(
    r"^BSS\s+([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)\s+"
)


def parse_int(value: str) -> int:
    return int(value, 0)


def bss_end(path: Path) -> int:
    for line in path.read_text(encoding="utf-8").splitlines():
        match = BSS_RE.match(line)
        if match:
            return int(match.group(2), 16)
    raise ValueError(f"BSS segment not found in {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("map_file", type=Path)
    parser.add_argument("--label", default="C64 build")
    parser.add_argument("--stack-base", type=parse_int, default=0xCC00)
    parser.add_argument("--min-headroom", type=parse_int, required=True)
    args = parser.parse_args()

    end = bss_end(args.map_file)
    headroom = args.stack_base - (end + 1)

    print(
        f"{args.label}: BSS ends at ${end:04X}; "
        f"{headroom} bytes free before ${args.stack_base:04X}"
    )

    if headroom < args.min_headroom:
        print(
            f"ERROR: {args.label} headroom {headroom} is below "
            f"required {args.min_headroom} bytes"
        )
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
