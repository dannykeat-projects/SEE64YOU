#!/usr/bin/env python3
from __future__ import annotations

import argparse
import socket
import struct
import time
from dataclasses import dataclass

API_VERSION = 0x02
STX = 0x02
CMD_MEMORY_GET = 0x01
CMD_EXIT_MONITOR = 0xAA
CMD_QUIT = 0xBB

DIAG_BASE = 0xC000
DIAG_PASS = 0xA5
DIAG_FAIL = 0x5A

@dataclass(frozen=True)
class Response:
    response_type: int
    error: int
    request_id: int
    body: bytes

class ViceMonitor:
    def __init__(self, host: str, port: int, timeout: float = 5.0) -> None:
        self._socket = socket.create_connection((host, port), timeout=timeout)
        self._socket.settimeout(timeout)
        self._request_id = 1

    def close(self) -> None:
        self._socket.close()

    def _read_exact(self, length: int) -> bytes:
        data = bytearray()
        while len(data) < length:
            chunk = self._socket.recv(length - len(data))
            if not chunk:
                raise ConnectionError("VICE monitor connection closed")
            data.extend(chunk)
        return bytes(data)

    def _read_response(self) -> Response:
        header = self._read_exact(12)
        if header[0] != STX:
            raise RuntimeError("invalid VICE response marker")
        body_length = struct.unpack_from("<I", header, 2)[0]
        return Response(
            response_type=header[6],
            error=header[7],
            request_id=struct.unpack_from("<I", header, 8)[0],
            body=self._read_exact(body_length),
        )

    def command(self, command_type: int, body: bytes = b"") -> Response:
        request_id = self._request_id
        self._request_id += 1
        packet = (
            bytes((STX, API_VERSION))
            + struct.pack("<I", len(body))
            + struct.pack("<I", request_id)
            + bytes((command_type,))
            + body
        )
        self._socket.sendall(packet)
        while True:
            response = self._read_response()
            if response.request_id == request_id:
                if response.error != 0:
                    raise RuntimeError(
                        f"VICE command 0x{command_type:02x} failed: 0x{response.error:02x}"
                    )
                return response

    def read_memory(self, start: int, end: int) -> bytes:
        response = self.command(
            CMD_MEMORY_GET,
            struct.pack("<BHHBH", 0, start, end, 0, 0),
        )
        length = struct.unpack_from("<H", response.body, 0)[0]
        data = response.body[2:]
        if len(data) != length:
            raise RuntimeError("VICE memory response length mismatch")
        return data

    def resume(self) -> None:
        self.command(CMD_EXIT_MONITOR)

    def quit(self) -> None:
        try:
            self.command(CMD_QUIT)
        except (ConnectionError, OSError, TimeoutError):
            pass

def wait_for_monitor(host: str, port: int, timeout: float) -> ViceMonitor:
    deadline = time.monotonic() + timeout
    last_error = None
    while time.monotonic() < deadline:
        try:
            return ViceMonitor(host, port, max(0.1, deadline - time.monotonic()))
        except OSError as error:
            last_error = error
            time.sleep(0.1)
    raise RuntimeError(f"VICE monitor did not open: {last_error}")

def expected_banks(size_kib: int) -> int:
    return size_kib // 64

def run(port: int, size_kib: int, timeout: float) -> None:
    monitor = wait_for_monitor("127.0.0.1", port, timeout)
    deadline = time.monotonic() + timeout
    state = bytes((0, 0, 0, 0))
    try:
        while time.monotonic() < deadline:
            state = monitor.read_memory(DIAG_BASE, DIAG_BASE + 3)
            progress, result, banks_lo, banks_hi = state
            banks = banks_lo | (banks_hi << 8)

            if result == DIAG_PASS:
                expected = expected_banks(size_kib)
                if banks != expected:
                    raise AssertionError(
                        f"REU {size_kib} KiB reported {banks} banks; expected {expected}"
                    )
                print(
                    f"VICE REU {size_kib} KiB passed "
                    f"(banks={banks}, progress=0x{progress:02x})"
                )
                return

            if result == DIAG_FAIL:
                raise AssertionError(
                    f"REU {size_kib} KiB diagnostic failed "
                    f"(progress=0x{progress:02x}, banks={banks})"
                )

            monitor.resume()
            time.sleep(0.05)

        progress, result, banks_lo, banks_hi = state
        banks = banks_lo | (banks_hi << 8)
        raise AssertionError(
            f"REU {size_kib} KiB diagnostic timed out "
            f"(progress=0x{progress:02x}, result=0x{result:02x}, banks={banks})"
        )
    finally:
        monitor.quit()
        monitor.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--size-kib", type=int, required=True)
    parser.add_argument("--timeout", type=float, default=8.0)
    args = parser.parse_args()
    run(args.port, args.size_kib, args.timeout)
