#!/usr/bin/env python3
"""See64You trusted-LAN HTTPS relay.

The C64U's current UCI HTTP target does not provide verified TLS. This relay
accepts authenticated HTTP requests from a controlled LAN and forwards them to
one fixed HTTPS upstream using the host Python TLS stack.
"""

from __future__ import annotations

import argparse
import hmac
import ipaddress
import json
import secrets
import ssl
import sys
import urllib.error
import urllib.parse
import urllib.request
from dataclasses import dataclass
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Iterable, Union

RELAY_HEADER = "X-See64You-Relay-Key"
FORWARDED_REQUEST_HEADERS = ("Authorization", "Content-Type", "Accept")
FORWARDED_RESPONSE_HEADERS = ("Content-Type", "Cache-Control")
SUPPORTED_METHODS = {"GET", "POST", "PUT", "PATCH", "DELETE"}


@dataclass(frozen=True)
class RelayConfig:
    listen_host: str
    listen_port: int
    upstream_base: str
    upstream_scheme: str
    upstream_netloc: str
    upstream_base_path: str
    relay_key: str
    allowed_networks: tuple[Union[ipaddress.IPv4Network, ipaddress.IPv6Network], ...]
    allowed_path_prefixes: tuple[str, ...]
    max_request_bytes: int
    max_response_bytes: int
    timeout_seconds: float


class RelayConfigError(ValueError):
    pass


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):  # noqa: N802
        return None


def _positive_int(value: object, name: str, minimum: int, maximum: int) -> int:
    if not isinstance(value, int) or isinstance(value, bool):
        raise RelayConfigError(f"{name} must be an integer")
    if value < minimum or value > maximum:
        raise RelayConfigError(f"{name} must be between {minimum} and {maximum}")
    return value


def _positive_number(value: object, name: str, minimum: float, maximum: float) -> float:
    if not isinstance(value, (int, float)) or isinstance(value, bool):
        raise RelayConfigError(f"{name} must be a number")
    result = float(value)
    if result < minimum or result > maximum:
        raise RelayConfigError(f"{name} must be between {minimum} and {maximum}")
    return result


def _validate_relay_key(value: object) -> str:
    if not isinstance(value, str) or len(value) != 64:
        raise RelayConfigError("relay_key must be exactly 64 hexadecimal characters")
    try:
        int(value, 16)
    except ValueError as exc:
        raise RelayConfigError("relay_key must be exactly 64 hexadecimal characters") from exc
    return value.lower()


def _validate_prefix(value: object) -> str:
    if not isinstance(value, str) or not value.startswith("/"):
        raise RelayConfigError("allowed_path_prefixes entries must start with /")
    decoded = urllib.parse.unquote(value)
    if "\\" in decoded or "\x00" in decoded or ".." in decoded.split("/"):
        raise RelayConfigError("allowed path prefix is unsafe")
    return value


def load_config(path: str | Path) -> RelayConfig:
    source = Path(path)
    try:
        raw = json.loads(source.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise RelayConfigError(f"cannot read relay config: {exc}") from exc

    if not isinstance(raw, dict):
        raise RelayConfigError("relay config must be a JSON object")

    upstream_base = raw.get("upstream_base")
    if not isinstance(upstream_base, str):
        raise RelayConfigError("upstream_base is required")

    parsed = urllib.parse.urlsplit(upstream_base)
    if parsed.scheme.lower() != "https" or not parsed.hostname:
        raise RelayConfigError("upstream_base must use https:// with a hostname")
    if parsed.username or parsed.password or parsed.query or parsed.fragment:
        raise RelayConfigError("upstream_base must not contain credentials, query or fragment")

    base_path = parsed.path or "/"
    decoded_base_path = urllib.parse.unquote(base_path)
    if "\x00" in decoded_base_path or "\\" in decoded_base_path:
        raise RelayConfigError("upstream_base path is unsafe")
    if ".." in decoded_base_path.split("/"):
        raise RelayConfigError("upstream_base path is unsafe")
    if not base_path.endswith("/"):
        base_path += "/"

    network_values = raw.get("allowed_clients", [])
    if not isinstance(network_values, list):
        raise RelayConfigError("allowed_clients must be a JSON array")
    networks: list[Union[ipaddress.IPv4Network, ipaddress.IPv6Network]] = []
    for value in network_values:
        if not isinstance(value, str):
            raise RelayConfigError("allowed_clients entries must be IP addresses or CIDRs")
        try:
            networks.append(ipaddress.ip_network(value, strict=False))
        except ValueError as exc:
            raise RelayConfigError(f"invalid allowed client network: {value}") from exc

    prefix_values = raw.get("allowed_path_prefixes", ["/api.php", "/wp-json/see64you/"])
    if not isinstance(prefix_values, list) or not prefix_values:
        raise RelayConfigError("allowed_path_prefixes must be a non-empty JSON array")
    prefixes = tuple(_validate_prefix(value) for value in prefix_values)

    listen_host = raw.get("listen_host", "0.0.0.0")
    if not isinstance(listen_host, str) or not listen_host:
        raise RelayConfigError("listen_host must be a non-empty string")

    return RelayConfig(
        listen_host=listen_host,
        listen_port=_positive_int(raw.get("listen_port", 6464), "listen_port", 1, 65535),
        upstream_base=upstream_base,
        upstream_scheme="https",
        upstream_netloc=parsed.netloc,
        upstream_base_path=base_path,
        relay_key=_validate_relay_key(raw.get("relay_key")),
        allowed_networks=tuple(networks),
        allowed_path_prefixes=prefixes,
        max_request_bytes=_positive_int(
            raw.get("max_request_bytes", 65536),
            "max_request_bytes",
            1,
            1048576,
        ),
        max_response_bytes=_positive_int(
            raw.get("max_response_bytes", 262144),
            "max_response_bytes",
            1,
            4194304,
        ),
        timeout_seconds=_positive_number(
            raw.get("timeout_seconds", 20),
            "timeout_seconds",
            1.0,
            120.0,
        ),
    )


def client_allowed(config: RelayConfig, address: str) -> bool:
    if not config.allowed_networks:
        return True
    try:
        ip = ipaddress.ip_address(address)
    except ValueError:
        return False
    return any(ip in network for network in config.allowed_networks)


def relay_key_allowed(config: RelayConfig, supplied: str | None) -> bool:
    if supplied is None:
        return False
    return hmac.compare_digest(config.relay_key, supplied.strip().lower())


def _safe_incoming_path(path: str) -> str:
    decoded = urllib.parse.unquote(path)
    if "\x00" in decoded or "\\" in decoded or not decoded.startswith("/"):
        raise RelayConfigError("unsafe relay path")
    if ".." in decoded.split("/"):
        raise RelayConfigError("unsafe relay path")
    return path


def build_upstream_url(config: RelayConfig, incoming_target: str) -> str:
    incoming = urllib.parse.urlsplit(incoming_target)
    path = _safe_incoming_path(incoming.path)

    if not any(
        path == prefix or (prefix.endswith("/") and path.startswith(prefix))
        for prefix in config.allowed_path_prefixes
    ):
        raise RelayConfigError("path is not allowed by relay configuration")

    relative = path.lstrip("/")
    upstream_path = config.upstream_base_path + relative
    return urllib.parse.urlunsplit(
        (
            config.upstream_scheme,
            config.upstream_netloc,
            upstream_path,
            incoming.query,
            "",
        )
    )


def request_headers(headers) -> dict[str, str]:
    forwarded: dict[str, str] = {}
    for name in FORWARDED_REQUEST_HEADERS:
        value = headers.get(name)
        if value:
            forwarded[name] = value
    forwarded["User-Agent"] = "See64You-Relay/1"
    forwarded["Accept-Encoding"] = "identity"
    return forwarded


def _read_bounded(stream, maximum: int) -> bytes:
    chunks: list[bytes] = []
    total = 0
    while True:
        chunk = stream.read(min(16384, maximum + 1 - total))
        if not chunk:
            break
        total += len(chunk)
        if total > maximum:
            raise RelayConfigError("upstream response exceeds configured limit")
        chunks.append(chunk)
    return b"".join(chunks)


def _build_opener() -> urllib.request.OpenerDirector:
    context = ssl.create_default_context()
    return urllib.request.build_opener(
        urllib.request.HTTPSHandler(context=context),
        NoRedirect(),
    )


class RelayHandler(BaseHTTPRequestHandler):
    server_version = "See64YouRelay/1.0"
    protocol_version = "HTTP/1.0"
    config: RelayConfig
    opener: urllib.request.OpenerDirector

    def do_GET(self):  # noqa: N802
        self._relay()

    def do_POST(self):  # noqa: N802
        self._relay()

    def do_PUT(self):  # noqa: N802
        self._relay()

    def do_PATCH(self):  # noqa: N802
        self._relay()

    def do_DELETE(self):  # noqa: N802
        self._relay()

    def log_message(self, fmt: str, *args) -> None:
        sys.stderr.write(
            f"{self.client_address[0]} - {self.command} "
            f"{urllib.parse.urlsplit(self.path).path} - {fmt % args}\n"
        )

    def _reject(self, status: int, message: str) -> None:
        body = (message + "\n").encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "text/plain; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def _read_request_body(self) -> bytes | None:
        length_text = self.headers.get("Content-Length")
        if length_text is None:
            return b""
        try:
            length = int(length_text)
        except ValueError:
            self._reject(400, "Invalid Content-Length")
            return None
        if length < 0 or length > self.config.max_request_bytes:
            self._reject(413, "Request body too large")
            return None
        body = self.rfile.read(length)
        if len(body) != length:
            self._reject(400, "Incomplete request body")
            return None
        return body

    def _relay(self) -> None:
        if self.command not in SUPPORTED_METHODS:
            self._reject(405, "Method not allowed")
            return

        if not client_allowed(self.config, self.client_address[0]):
            self._reject(403, "Client address not allowed")
            return

        if not relay_key_allowed(self.config, self.headers.get(RELAY_HEADER)):
            self._reject(401, "Relay authentication failed")
            return

        try:
            upstream_url = build_upstream_url(self.config, self.path)
        except RelayConfigError as exc:
            self._reject(403, str(exc))
            return

        body = self._read_request_body()
        if body is None:
            return

        request = urllib.request.Request(
            upstream_url,
            data=body if self.command in {"POST", "PUT", "PATCH", "DELETE"} else None,
            headers=request_headers(self.headers),
            method=self.command,
        )

        try:
            try:
                response = self.opener.open(request, timeout=self.config.timeout_seconds)
            except urllib.error.HTTPError as exc:
                response = exc

            with response:
                response_body = _read_bounded(response, self.config.max_response_bytes)
                status = int(response.getcode())
                self.send_response(status)
                for name in FORWARDED_RESPONSE_HEADERS:
                    value = response.headers.get(name)
                    if value:
                        self.send_header(name, value)
                self.send_header("Content-Length", str(len(response_body)))
                self.send_header("Cache-Control", "no-store")
                self.end_headers()
                self.wfile.write(response_body)
        except RelayConfigError as exc:
            self._reject(502, str(exc))
        except (urllib.error.URLError, TimeoutError, OSError) as exc:
            self._reject(502, f"Upstream connection failed: {type(exc).__name__}")


def handler_for(config: RelayConfig):
    opener = _build_opener()

    class ConfiguredRelayHandler(RelayHandler):
        pass

    ConfiguredRelayHandler.config = config
    ConfiguredRelayHandler.opener = opener
    return ConfiguredRelayHandler


def run(config: RelayConfig) -> None:
    server = ThreadingHTTPServer(
        (config.listen_host, config.listen_port),
        handler_for(config),
    )
    print(
        f"See64You relay listening on {config.listen_host}:{config.listen_port}; "
        f"upstream {config.upstream_base}",
        flush=True,
    )
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="See64You trusted-LAN HTTPS relay")
    parser.add_argument("--config", default="relay/config.json", help="relay JSON config")
    parser.add_argument(
        "--generate-key",
        action="store_true",
        help="print a new 32-byte relay key and exit",
    )
    args = parser.parse_args(list(argv) if argv is not None else None)

    if args.generate_key:
        print(secrets.token_hex(32))
        return 0

    try:
        config = load_config(args.config)
    except RelayConfigError as exc:
        print(f"Relay configuration error: {exc}", file=sys.stderr)
        return 2

    run(config)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
