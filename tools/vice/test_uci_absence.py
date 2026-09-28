#!/usr/bin/env python3
from __future__ import annotations

import argparse
import time

from test_reu_probe import wait_for_monitor

DIAG_BASE = 0xC080
DIAG_PASS = 0xA5
DIAG_FAIL = 0x5A


def run(port: int, timeout: float) -> None:
    monitor = wait_for_monitor("127.0.0.1", port, timeout)
    deadline = time.monotonic() + timeout
    state = bytes((0, 0, 0))
    try:
        while time.monotonic() < deadline:
            state = monitor.read_memory(DIAG_BASE, DIAG_BASE + 2)
            progress, result, detail = state
            if result == DIAG_PASS:
                print(
                    "VICE conventional-C64 UCI absence test passed "
                    f"(progress=0x{progress:02x})"
                )
                return
            if result == DIAG_FAIL:
                raise AssertionError(
                    "UCI absence diagnostic false-detected Ultimate capability "
                    f"(detail=0x{detail:02x})"
                )
            monitor.resume()
            time.sleep(0.03)

        progress, result, detail = state
        raise AssertionError(
            "UCI absence diagnostic timed out "
            f"(progress=0x{progress:02x}, result=0x{result:02x}, "
            f"detail=0x{detail:02x})"
        )
    finally:
        monitor.quit()
        monitor.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=6514)
    parser.add_argument("--timeout", type=float, default=20.0)
    args = parser.parse_args()
    run(args.port, args.timeout)
