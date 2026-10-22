#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path


def parse_int(value: str) -> int:
    return int(value, 0)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("prg", type=Path)
    parser.add_argument("--label", default="PRG")
    parser.add_argument("--load-address", type=parse_int, required=True)
    parser.add_argument("--max-end", type=parse_int, required=True)
    args = parser.parse_args()

    data = args.prg.read_bytes()
    if len(data) < 3:
        print(f"ERROR: {args.label} is too short to contain a PRG payload")
        return 1

    load_address = data[0] | (data[1] << 8)
    if load_address != args.load_address:
        print(
            f"ERROR: {args.label} load address is ${load_address:04X}; "
            f"expected ${args.load_address:04X}"
        )
        return 1

    payload_size = len(data) - 2
    load_end = load_address + payload_size - 1
    print(
        f"{args.label}: {payload_size} payload bytes, "
        f"${load_address:04X}-${load_end:04X}"
    )

    if load_end > args.max_end:
        print(
            f"ERROR: {args.label} reaches ${load_end:04X}; "
            f"must end at or below ${args.max_end:04X}"
        )
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
