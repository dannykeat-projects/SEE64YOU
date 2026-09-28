#!/usr/bin/env python3
from __future__ import annotations

import argparse
import time

from test_reu_probe import ViceMonitor, wait_for_monitor

SCREEN_BASE = 0x0400
SCREEN_COLUMNS = 40
SCREEN_ROWS = 25
KEY_BUFFER = 0x0277
KEY_COUNT = 0x00C6
CH_STOP = 0x03


def screen_char(value: int) -> str:
    value &= 0x7F
    if value == 0:
        return "@"
    if 1 <= value <= 26:
        return chr(ord("A") + value - 1)
    if 0x20 <= value <= 0x3F:
        return chr(value)
    if 0x41 <= value <= 0x5A:
        return chr(value)
    return " "


def screen_text(monitor: ViceMonitor) -> list[str]:
    raw = monitor.read_memory(
        SCREEN_BASE,
        SCREEN_BASE + SCREEN_COLUMNS * SCREEN_ROWS - 1,
    )
    rows = []
    for offset in range(0, len(raw), SCREEN_COLUMNS):
        rows.append("".join(screen_char(v) for v in raw[offset:offset + SCREEN_COLUMNS]))
    return rows


def wait_for_text(
    monitor: ViceMonitor,
    needle: str,
    timeout: float,
    *,
    row: int | None = None,
) -> list[str]:
    deadline = time.monotonic() + timeout
    last_rows: list[str] = []
    while time.monotonic() < deadline:
        last_rows = screen_text(monitor)
        haystack = last_rows[row] if row is not None else "\n".join(last_rows)
        if needle in haystack:
            return last_rows
        monitor.resume()
        time.sleep(0.05)

    rendered = "\n".join(f"{i:02d}: {line}" for i, line in enumerate(last_rows))
    raise AssertionError(
        f"Timed out waiting for {needle!r} on screen"
        + (f" row {row}" if row is not None else "")
        + f":\n{rendered}"
    )


def inject_stop(monitor: ViceMonitor) -> None:
    monitor.write_memory(KEY_BUFFER, bytes((CH_STOP,)))
    monitor.write_memory(KEY_COUNT, b"\x01")
    monitor.resume()


def run(port: int, timeout: float) -> None:
    monitor = wait_for_monitor("127.0.0.1", port, timeout)
    try:
        wait_for_text(monitor, "SEE64YOU PUBLISHER", timeout, row=0)
        wait_for_text(
            monitor,
            "SETTINGS STORAGE UNAVAILABLE",
            timeout,
            row=24,
        )
        print("VICE chain: publisher/settings round trip passed")

        inject_stop(monitor)
        wait_for_text(monitor, "SEE64YOU  FILE EDIT SEARCH FORMAT PUB", timeout, row=0)
        print("VICE chain: publisher/editor return passed")
    finally:
        monitor.quit()
        monitor.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=6514)
    parser.add_argument("--timeout", type=float, default=35.0)
    args = parser.parse_args()
    run(args.port, args.timeout)
