#!/usr/bin/env python3
from __future__ import annotations

import argparse
import time

from test_reu_probe import ViceMonitor, wait_for_monitor

DIAG_BASE = 0xC020
DIAG_PASS = 0xA5
DIAG_FAIL = 0x5A


def run(port: int, timeout: float) -> None:
    monitor = wait_for_monitor("127.0.0.1", port, timeout)
    started = time.monotonic()
    deadline = started + timeout
    state = bytes((0, 0, 0))
    last_progress = None
    last_progress_at = started
    try:
        while time.monotonic() < deadline:
            state = monitor.read_memory(DIAG_BASE, DIAG_BASE + 2)
            progress, result, detail = state

            if progress != last_progress:
                now = time.monotonic()
                if last_progress is not None:
                    print(
                        f"REU editor progress 0x{last_progress:02x}->0x{progress:02x} "
                        f"in {now - last_progress_at:.2f}s"
                    )
                last_progress = progress
                last_progress_at = now
                deadline = now + timeout

            if result == DIAG_PASS:
                print(
                    "VICE REU editor test passed "
                    f"(progress=0x{progress:02x}, detail={detail})"
                )
                return
            if result == DIAG_FAIL:
                raise AssertionError(
                    "REU editor diagnostic failed "
                    f"(progress=0x{progress:02x}, detail={detail})"
                )

            monitor.resume()
            time.sleep(0.05)

        progress, result, detail = state
        raise AssertionError(
            "REU editor diagnostic stalled "
            f"(progress=0x{progress:02x}, result=0x{result:02x}, detail={detail})"
        )
    finally:
        monitor.quit()
        monitor.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=6531)
    parser.add_argument("--timeout", type=float, default=60.0)
    args = parser.parse_args()
    run(args.port, args.timeout)
