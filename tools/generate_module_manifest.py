#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import zlib
from dataclasses import dataclass
from pathlib import Path

MAGIC = "S64MODULES1"
MODULES = (
    ("EDITOR", "S64EDIT.PRG"),
    ("PUBLISHER", "S64PUB.PRG"),
    ("SETTINGS", "S64CFG.PRG"),
)

@dataclass(frozen=True)
class PrgMetadata:
    load_address: int
    startup_address: int
    byte_length: int
    crc32: int

def inspect_prg(path: Path) -> PrgMetadata:
    data = path.read_bytes()
    if len(data) < 8:
        raise ValueError(f"{path.name}: PRG too short")
    load_address = data[0] | (data[1] << 8)
    payload = data[2:]

    # cc65 C64 PRGs begin with a BASIC line containing SYS <decimal>.
    # Parse the first BASIC line rather than assuming a fixed machine entry.
    if len(payload) < 5:
        raise ValueError(f"{path.name}: BASIC stub missing")
    next_line = payload[0] | (payload[1] << 8)
    if next_line == 0:
        raise ValueError(f"{path.name}: BASIC stub has no first line")
    try:
        line_end = payload.index(0, 4)
    except ValueError as exc:
        raise ValueError(f"{path.name}: unterminated BASIC stub") from exc
    line = payload[4:line_end]
    try:
        sys_index = line.index(0x9E)
    except ValueError as exc:
        raise ValueError(f"{path.name}: BASIC SYS token missing") from exc
    text = bytes(line[sys_index + 1:]).decode("ascii", errors="ignore")
    match = re.search(r"(\d+)", text)
    if match is None:
        raise ValueError(f"{path.name}: SYS target missing")
    startup_address = int(match.group(1), 10)
    if not 0 < startup_address <= 0xFFFF:
        raise ValueError(f"{path.name}: invalid SYS target {startup_address}")

    return PrgMetadata(
        load_address=load_address,
        startup_address=startup_address,
        byte_length=len(data),
        crc32=zlib.crc32(data) & 0xFFFFFFFF,
    )

def render(directory: Path) -> str:
    lines = [MAGIC]
    for module_id, filename in MODULES:
        metadata = inspect_prg(directory / filename)
        lines.append(
            "|".join((
                module_id,
                filename,
                f"{metadata.load_address:04X}",
                f"{metadata.startup_address:04X}",
                str(metadata.byte_length),
                f"{metadata.crc32:08X}",
            ))
        )
    return "\n".join(lines) + "\n"

def validate(manifest: Path, directory: Path) -> None:
    expected = render(directory)
    actual = manifest.read_text(encoding="ascii")
    if actual != expected:
        raise ValueError(
            "module manifest does not match built PRGs\n"
            f"expected:\n{expected}actual:\n{actual}"
        )

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--validate", type=Path)
    args = parser.parse_args()

    try:
        if args.validate is not None:
            validate(args.validate, args.directory)
            print(f"module manifest validated: {args.validate}")
            return 0
        output = args.output
        if output is None:
            parser.error("--output is required unless --validate is used")
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(render(args.directory), encoding="ascii")
        print(f"module manifest written: {output}")
        return 0
    except (OSError, ValueError) as exc:
        print(f"ERROR: {exc}")
        return 1

if __name__ == "__main__":
    raise SystemExit(main())
