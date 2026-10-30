#!/usr/bin/env python3
"""Disposable in-memory API 1 test fixture, never a production server.

Uses only the public vector credential test-1 / bytes(range(32)). Start fresh
for each C64U probe; no credentials or application source files are loaded.
The optional receipt contains hashes/operations, never keys or signatures.
"""
from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import re
import secrets
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qsl, unquote, urlsplit

KEY = bytes(range(32))
ID = "test-1"
NAME = re.compile(r"[A-Za-z0-9_.-]+(?:/[A-Za-z0-9_.-]+)*\Z")
MAX_SIZE = 61440


def valid_name(value: str) -> bool:
    return (0 < len(value) <= 64 and NAME.fullmatch(value) is not None
            and all(part not in (".", "..") for part in value.split("/")))


class State:
    def __init__(self, public_url: str, receipt: Path | None):
        self.public_url = public_url.rstrip("/")
        self.receipt = receipt
        self.lock = threading.RLock()
        self.challenges: dict[str, float] = {}
        self.files: dict[str, bytes] = {}
        self.events: list[dict] = []
        self.puts = 0

    def record(self, event: dict) -> None:
        with self.lock:
            self.events.append(event)
            if self.receipt:
                data = {
                    "fixture": "8tml-publish/1-disposable",
                    "independent_interoperability": False,
                    "events": self.events,
                    "successful_puts": self.puts,
                    "files": {name: {"size": len(source),
                                     "sha256": hashlib.sha256(source).hexdigest()}
                              for name, source in self.files.items()},
                }
                temporary = self.receipt.with_suffix(self.receipt.suffix + ".tmp")
                temporary.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
                temporary.replace(self.receipt)


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.0"
    server_version = "Disposable8TMLFixture/1"

    @property
    def state(self) -> State:
        return self.server.state

    def log_message(self, format, *args):
        pass  # Never log credentials, signed requests or source bytes.

    def response(self, status: int, fields: list[tuple[str, str]], event: dict):
        body = ("".join(f"{key}: {value}\n" for key, value in fields)).encode("ascii")
        self.state.record(dict(event, http_status=status))
        self.send_response(status)
        self.send_header("Content-Type", "text/plain; charset=us-ascii")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Connection", "close")
        self.end_headers()
        self.wfile.write(body)
        self.close_connection = True

    def error(self, code: str, message: str, event: dict, status: int = 400):
        self.response(status, [("status", "error"), ("error", code),
                               ("message", message)], dict(event, error=code))

    def do_GET(self):
        self.handle_api(False)

    def do_POST(self):
        self.handle_api(True)

    def handle_api(self, post: bool):
        self.connection.settimeout(30)
        parsed = urlsplit(self.path)
        if not post and parsed.path.startswith("/files/"):
            name = unquote(parsed.path[7:])
            with self.state.lock:
                data = self.state.files.get(name)
            if data is None:
                self.error("missing", "No file", {"op": "read", "name": name}, 404)
                return
            self.state.record({"op": "read", "name": name, "http_status": 200})
            self.send_response(200)
            self.send_header("Content-Type", "text/8tml")
            self.send_header("Content-Length", str(len(data)))
            self.send_header("Connection", "close")
            self.end_headers()
            self.wfile.write(data)
            return
        fields = parse_qsl(parsed.query, keep_blank_values=True)
        query = dict(fields)
        op = query.get("op", "")
        event = {"op": op}
        if parsed.path != "/publish" or len(query) != len(fields):
            self.error("request", "Invalid endpoint or duplicate parameter", event)
            return
        if not post and op == "status":
            self.response(200, [("status", "ok"), ("api", "1"),
                                ("auth", "hmac"), ("max-size", str(MAX_SIZE))], event)
            return
        if not post or op not in ("challenge", "list", "put"):
            self.error("request", "Unsupported operation", event)
            return
        # Consume a supplied challenge before any signed-request validation.
        with self.state.lock:
            issued = self.state.challenges.pop(query.get("c", ""), None)
        lengths = self.headers.get_all("Content-Length", [])
        if len(lengths) != 1 or not re.fullmatch(r"0|[1-9][0-9]*", lengths[0]):
            self.error("request", "One canonical Content-Length required", event)
            return
        length = int(lengths[0])
        if length > MAX_SIZE + 64:
            self.error("size", "Request too large", event, 413)
            return
        if query.get("id") != ID:
            self.error("auth", "Unknown fixture key id", event, 403)
            return
        if op == "challenge":
            if length:
                self.error("request", "Challenge body must be empty", event)
                return
            with self.state.lock:
                now = time.monotonic()
                self.state.challenges = {c: t for c, t in self.state.challenges.items()
                                         if now - t < 120}
                if len(self.state.challenges) >= 16:
                    self.error("quota", "Too many fixture challenges", event, 429)
                    return
                challenge = secrets.token_hex(16)
                self.state.challenges[challenge] = now
            self.response(200, [("status", "ok"), ("challenge", challenge)], event)
            return
        if issued is None or time.monotonic() - issued > 120:
            self.error("auth", "Missing expired or consumed challenge", event, 403)
            return
        size_text = query.get("size", "")
        if not re.fullmatch(r"0|[1-9][0-9]*", size_text):
            self.error("size", "Canonical size required", event)
            return
        size = int(size_text)
        name = query.get("name", "")
        if size > MAX_SIZE or length != size + 64:
            self.error("size", "Size or Content-Length mismatch", event, 413)
            return
        if (op == "list" and (name or size)) or (op == "put" and not valid_name(name)):
            self.error("name", "Invalid remote name", event)
            return
        try:
            body = self.rfile.read(length)
        except (TimeoutError, OSError):
            self.state.record(dict(event, error="partial-body", received=False))
            return
        if len(body) != length:
            self.error("request", "Incomplete body", event)
            return
        prefix = f"8tml-publish/1\n{op}\n{ID}\n{query['c']}\n{name}\n{size}\n".encode("ascii")
        expected = hmac.new(KEY, prefix + body[:size], hashlib.sha256).hexdigest().encode("ascii")
        if not hmac.compare_digest(expected, body[size:]):
            self.error("auth", "Invalid signature", event, 403)
            return
        event.update(authenticated=True, request_bytes=length)
        if op == "list":
            with self.state.lock:
                files = [("file", f"{len(source)} {path}")
                         for path, source in sorted(self.state.files.items())]
            self.response(200, [("status", "ok")] + files, event)
            return
        name = name.lower()
        event["name"] = name
        if name == "fixture-denied.8tml":
            self.error("quota", "Intentional disposable fixture rejection", event, 403)
            return
        with self.state.lock:
            if name not in self.state.files and len(self.state.files) >= 8:
                self.error("quota", "Fixture file limit", event, 403)
                return
            replaced = name in self.state.files
            self.state.files[name] = body[:size]
            self.state.puts += 1
        event.update(replaced=replaced, size=size,
                     sha256=hashlib.sha256(body[:size]).hexdigest())
        self.response(200, [("status", "ok"), ("name", name), ("size", str(size)),
                            ("url", self.state.public_url + "/files/" + name)], event)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8767)
    parser.add_argument("--public-url", default=None)
    parser.add_argument("--receipt", type=Path)
    args = parser.parse_args()
    server = ThreadingHTTPServer((args.host, args.port), Handler)
    public_url = args.public_url or f"http://127.0.0.1:{server.server_port}"
    server.state = State(public_url, args.receipt)
    print(f"Disposable 8TML fixture ready on {args.host}:{server.server_port}", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
