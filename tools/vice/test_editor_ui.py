#!/usr/bin/env python3
from __future__ import annotations

import argparse
import time

from test_reu_probe import ViceMonitor, wait_for_monitor

DIAG_BASE = 0xC040
SCREEN_BASE = 0x0400
SCREEN_COLUMNS = 40
DIAG_PASS = 0xA5
DIAG_FAIL = 0x5A

STAGES = (0x10, 0x20, 0x30, 0x40)


def screen_row(text: str = "", cursor: int | None = None) -> bytes:
    if len(text) > SCREEN_COLUMNS:
        raise ValueError("fixture row exceeds 40 columns")
    row = bytearray(b" " * SCREEN_COLUMNS)
    row[: len(text)] = text.encode("ascii")
    if cursor is not None:
        row[cursor] ^= 0x80
    return bytes(row)


EXPECTED = {
    0x10: (
        screen_row("0123456789012345678901234567890123456789"),
        screen_row("012345"),
        screen_row("1111111111"),
        screen_row("2222222222", cursor=10),
        screen_row(),
    ),
    0x20: (
        screen_row("0123456789012345678901234567890123456789"),
        screen_row("012345"),
        screen_row("11111911111", cursor=6),
        screen_row("2222222222"),
        screen_row(),
    ),
    0x30: (
        screen_row("0123456789012345678901234567890123456789"),
        screen_row("012345"),
        screen_row("111119"),
        screen_row("11111", cursor=0),
        screen_row("2222222222"),
    ),
    0x40: (
        screen_row("0123456789012345678901234567890123456789"),
        screen_row("012345"),
        screen_row("11111911111", cursor=6),
        screen_row("2222222222"),
        screen_row(),
    ),
}


def read_body(monitor: ViceMonitor) -> tuple[bytes, ...]:
    data = monitor.read_memory(
        SCREEN_BASE + SCREEN_COLUMNS,
        SCREEN_BASE + SCREEN_COLUMNS * 6 - 1,
    )
    return tuple(
        data[offset : offset + SCREEN_COLUMNS]
        for offset in range(0, len(data), SCREEN_COLUMNS)
    )


def explain_row(row: bytes) -> str:
    chars = []
    for value in row:
        base = value & 0x7F
        char = chr(base) if 0x20 <= base <= 0x7E else "."
        chars.append(f"[{char}]" if value & 0x80 else char)
    return "".join(chars).rstrip()


def require_stage(monitor: ViceMonitor, stage: int) -> None:
    actual = read_body(monitor)
    expected = EXPECTED[stage]
    if actual != expected:
        details = []
        for index, (got, want) in enumerate(zip(actual, expected), start=1):
            if got != want:
                details.append(
                    f"row {index}: got={explain_row(got)!r} "
                    f"expected={explain_row(want)!r}"
                )
        raise AssertionError(
            f"UI stage 0x{stage:02x} screen mismatch: " + "; ".join(details)
        )


def run(port: int, timeout: float) -> None:
    monitor = wait_for_monitor("127.0.0.1", port, timeout)
    deadline = time.monotonic() + timeout
    seen: set[int] = set()
    state = bytes((0, 0, 0, 0))
    try:
        while time.monotonic() < deadline:
            state = monitor.read_memory(DIAG_BASE, DIAG_BASE + 3)
            progress, result, ack, detail = state

            if result == DIAG_FAIL:
                raise AssertionError(
                    "UI diagnostic failed "
                    f"(progress=0x{progress:02x}, detail={detail})"
                )

            if progress in STAGES and progress not in seen:
                require_stage(monitor, progress)
                monitor.write_memory(DIAG_BASE + 2, bytes((progress,)))
                seen.add(progress)

            if result == DIAG_PASS:
                missing = [stage for stage in STAGES if stage not in seen]
                if missing:
                    raise AssertionError(
                        "UI diagnostic passed before all screen stages were checked: "
                        + ", ".join(f"0x{x:02x}" for x in missing)
                    )
                print("VICE editor UI screen regression passed")
                return

            monitor.resume()
            time.sleep(0.03)

        progress, result, ack, detail = state
        raise AssertionError(
            "UI diagnostic timed out "
            f"(progress=0x{progress:02x}, result=0x{result:02x}, "
            f"ack=0x{ack:02x}, detail={detail})"
        )
    finally:
        monitor.quit()
        monitor.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=6517)
    parser.add_argument("--timeout", type=float, default=20.0)
    args = parser.parse_args()
    run(args.port, args.timeout)
