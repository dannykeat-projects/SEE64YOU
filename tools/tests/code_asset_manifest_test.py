#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import struct
import sys
import tempfile
import zlib
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "generate_code_asset_manifest.py"
SPEC = importlib.util.spec_from_file_location("code_asset_manifest", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
GEN = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = GEN
SPEC.loader.exec_module(GEN)


def make_prg(path: Path, load: int, startup: int, tail: bytes = b"\xEA" * 32) -> None:
    line = bytes((0x0B, 0x08, 10, 0, 0x9E)) + str(startup).encode("ascii") + b"\x00\x00\x00"
    path.write_bytes(struct.pack("<H", load) + line + tail)


def setup(directory: Path) -> None:
    make_prg(directory / "editor-overlay-fit.prg", 0x0801, 0x080D)
    for _group, source, _canonical in GEN.GROUPS:
        (directory / source).write_bytes((source.encode("ascii") * 17)[:73])


def must_fail(action, fragment: str) -> None:
    try:
        action()
    except ValueError as exc:
        if fragment not in str(exc):
            raise AssertionError(f"expected {fragment!r}, got {exc!r}") from exc
    else:
        raise AssertionError(f"expected failure containing {fragment!r}")


def main() -> int:
    with tempfile.TemporaryDirectory() as td:
        directory = Path(td)
        setup(directory)
        manifest_path = directory / "S64OVL.MNF"
        encoded = GEN.generate(directory, manifest_path, app_version=0x2345)
        assert len(encoded) == 52
        assert encoded[:6] == b"S64O\x01\x03"
        assert struct.unpack_from("<HHHH", encoded, 6) == (1, 0x2345, 0x8C00, 0x3C00)

        root = (directory / "editor-overlay-fit.prg").read_bytes()
        assert struct.unpack_from("<II", encoded, 16) == (
            len(root), zlib.crc32(root) & 0xFFFFFFFF
        )
        for index, (_group, _source, canonical) in enumerate(GEN.GROUPS):
            data = (directory / canonical).read_bytes()
            assert struct.unpack_from("<II", encoded, 24 + index * 8) == (
                len(data), zlib.crc32(data) & 0xFFFFFFFF
            )
        assert encoded[14:16] == b"\x00\x00"
        assert struct.unpack_from("<I", encoded, 48)[0] == zlib.crc32(encoded[:48]) & 0xFFFFFFFF
        GEN.validate(manifest_path, directory, app_version=0x2345)
        must_fail(lambda: GEN.validate(manifest_path, directory), "does not match")

        default_manifest = directory / "S64OVL.DEFAULT.MNF"
        default_encoded = GEN.generate(directory, default_manifest)
        assert GEN.package_version() == 0x0202
        assert struct.unpack_from("<H", default_encoded, 8)[0] == 0x0202
        GEN.validate(default_manifest, directory)

        # Canonical names are bound to group order, and every asset is checked.
        file_path = directory / "S64FILE.BIN"
        original_file = file_path.read_bytes()
        file_path.write_bytes((directory / "S64RUN.BIN").read_bytes())
        must_fail(lambda: GEN.validate(manifest_path, directory, app_version=0x2345), "does not match")
        file_path.write_bytes(original_file)
        manifest_path.write_bytes(bytes([encoded[24] ^ 1]) + encoded[1:])
        must_fail(lambda: GEN.validate(manifest_path, directory, app_version=0x2345), "does not match")
        manifest_path.write_bytes(encoded)

        (directory / "S64MODEL.BIN").write_bytes(b"x" * (GEN.SLOT_CAPACITY + 1))
        must_fail(lambda: GEN.validate(manifest_path, directory, app_version=0x2345), "asset length")
        must_fail(lambda: GEN.render(directory, app_version=0), "app version")
        must_fail(lambda: GEN.render(directory, app_version=0x10000), "app version")
        bad_version = directory / "bad-package-version.h"
        bad_version.write_text("#define S64_PACKAGE_VERSION OTHER_VERSION\n", encoding="ascii")
        must_fail(lambda: GEN.package_version(bad_version), "simple integer constant")
        bad_version.write_text("#define S64_PACKAGE_VERSION 0x10000u\n", encoding="ascii")
        must_fail(lambda: GEN.package_version(bad_version), "1..65535")

        make_prg(directory / "bad-load.prg", 0x0800, 0x080D)
        must_fail(lambda: GEN.inspect_root(directory / "bad-load.prg"), "load address")
        make_prg(directory / "bad-start.prg", 0x0801, 0x9000)
        must_fail(lambda: GEN.inspect_root(directory / "bad-start.prg"), "SYS target")
        make_prg(directory / "start-before-payload.prg", 0x0801, 0x0800)
        must_fail(lambda: GEN.inspect_root(directory / "start-before-payload.prg"), "SYS target")
        # The final resident byte may be $8BFF; $8C00 belongs to the overlay slot.
        make_prg(directory / "largest-root.prg", 0x0801, 0x080D,
                 b"x" * (GEN.SLOT_ADDRESS - 0x0801 - 12))
        expected_largest_length = GEN.SLOT_ADDRESS - 0x0801 + 2
        assert len((directory / "largest-root.prg").read_bytes()) == expected_largest_length
        GEN.inspect_root(directory / "largest-root.prg")
        make_prg(directory / "colliding-root.prg", 0x0801, 0x080D,
                 b"x" * (GEN.SLOT_ADDRESS - 0x0801 - 11))
        must_fail(lambda: GEN.inspect_root(directory / "colliding-root.prg"), "must end before")
        make_prg(directory / "too-large.prg", 0x0801, 0x080D, b"x" * 0x8400)
        must_fail(lambda: GEN.inspect_root(directory / "too-large.prg"), "must end before")

    print("code asset manifest tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
