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
SCREEN_START = 0x0400
SCREEN_SIZE = 40 * 25

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

def screen_codes(text: str) -> bytes:
    out = bytearray()
    for ch in text:
        if "A" <= ch <= "Z":
            out.append(ord(ch) - ord("A") + 1)
        else:
            out.append(ord(ch))
    return bytes(out)

def decode_screen(screen: bytes) -> str:
    rows = []
    for row in range(25):
        chars = []
        for value in screen[row * 40 : (row + 1) * 40]:
            if 1 <= value <= 26:
                chars.append(chr(ord("A") + value - 1))
            elif 32 <= value <= 126:
                chars.append(chr(value))
            else:
                chars.append(".")
        rows.append("".join(chars).rstrip())
    return "\n".join(rows)

def run(port: int, size_kib: int, timeout: float) -> None:
    monitor = wait_for_monitor("127.0.0.1", port, timeout)
    expected_size = screen_codes(f"SIZE: {size_kib} KIB")
    expected_pass = screen_codes("PASS")
    deadline = time.monotonic() + timeout
    screen = b""
    try:
        while time.monotonic() < deadline:
            screen = monitor.read_memory(SCREEN_START, SCREEN_START + SCREEN_SIZE - 1)
            if expected_size in screen and expected_pass in screen:
                print(f"VICE REU {size_kib} KiB passed")
                return
            monitor.resume()
            time.sleep(0.05)
        raise AssertionError(
            f"diagnostic never reported {size_kib} KiB PASS\n"
            f"screen:\n{decode_screen(screen)}"
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
