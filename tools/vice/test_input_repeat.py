#!/usr/bin/env python3
from __future__ import annotations

import argparse
import time

from test_reu_probe import ViceMonitor, wait_for_monitor

DIAG_BASE = 0xC060
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
                    "VICE input repeat test passed "
                    f"(progress=0x{progress:02x}, detail={detail})"
                )
                return
            if result == DIAG_FAIL:
                raise AssertionError(
                    "Input repeat diagnostic failed "
                    f"(progress=0x{progress:02x}, detail={detail})"
                )
            monitor.resume()
            time.sleep(0.03)

        progress, result, detail = state
        raise AssertionError(
            "Input repeat diagnostic timed out "
            f"(progress=0x{progress:02x}, result=0x{result:02x}, detail={detail})"
        )
    finally:
        monitor.quit()
        monitor.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=6516)
    parser.add_argument("--timeout", type=float, default=20.0)
    args = parser.parse_args()
    run(args.port, args.timeout)
