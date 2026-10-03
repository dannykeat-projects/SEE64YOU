#!/usr/bin/env python3
"""Generate or validate the diagnostic editor code-asset manifest."""
from __future__ import annotations

import argparse
import importlib.util
import struct
import sys
import zlib
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
MODULE_GENERATOR = ROOT / "tools" / "generate_module_manifest.py"
MAGIC = b"S64O"
FORMAT_VERSION = 1
ABI_VERSION = 1
SLOT_ADDRESS = 0x8C00
SLOT_CAPACITY = 0x3C00
MANIFEST_SIZE = 52
GROUPS = (
    ("MODEL", "model.bin", "S64MODEL.BIN"),
    ("FILE", "file.bin", "S64FILE.BIN"),
    ("RUNTIME", "runtime.bin", "S64RUN.BIN"),
)


def _load_prg_inspector():
    spec = importlib.util.spec_from_file_location("module_manifest_generator", MODULE_GENERATOR)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Could not load PRG inspector: {MODULE_GENERATOR}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module.inspect_prg


inspect_prg = _load_prg_inspector()


def _crc(data: bytes, label: str) -> int:
    value = zlib.crc32(data) & 0xFFFFFFFF
    if value == 0:
        raise ValueError(f"{label}: CRC32 must be nonzero")
    return value


def inspect_root(path: Path) -> tuple[int, int]:
    metadata = inspect_prg(path)
    end = metadata.load_address + metadata.byte_length - 3
    if metadata.load_address != 0x0801:
        raise ValueError(f"{path.name}: load address must be $0801")
    if end < metadata.load_address or end >= SLOT_ADDRESS:
        raise ValueError(f"{path.name}: resident payload reaches ${end:04X}; it must end before ${SLOT_ADDRESS:04X}")
    if not metadata.load_address <= metadata.startup_address <= end:
        raise ValueError(f"{path.name}: SYS target ${metadata.startup_address:04X} is outside resident payload")
    data = path.read_bytes()
    if len(data) > SLOT_ADDRESS - metadata.load_address + 2:
        raise ValueError(f"{path.name}: PRG length is unreasonable for the resident region")
    return len(data), _crc(data, path.name)


def _asset_metadata(path: Path) -> tuple[int, int]:
    data = path.read_bytes()
    if not 1 <= len(data) <= SLOT_CAPACITY:
        raise ValueError(f"{path.name}: asset length must be 1..{SLOT_CAPACITY} bytes")
    return len(data), _crc(data, path.name)


def render(directory: Path, root: Path | None = None, app_version: int = 1) -> bytes:
    if not 1 <= app_version <= 0xFFFF:
        raise ValueError("app version must be in 1..65535")
    root_path = root if root is not None else directory / "editor-overlay-fit.prg"
    root_length, root_crc = inspect_root(root_path)
    result = bytearray(MANIFEST_SIZE)
    result[0:4] = MAGIC
    result[4] = FORMAT_VERSION
    result[5] = len(GROUPS)
    struct.pack_into("<HHHH", result, 6, ABI_VERSION, app_version, SLOT_ADDRESS, SLOT_CAPACITY)
    struct.pack_into("<II", result, 16, root_length, root_crc)
    for index, (group, _source, canonical) in enumerate(GROUPS):
        length, checksum = _asset_metadata(directory / canonical)
        struct.pack_into("<II", result, 24 + index * 8, length, checksum)
    struct.pack_into("<I", result, 48, zlib.crc32(result[:48]) & 0xFFFFFFFF)
    return bytes(result)


def generate(directory: Path, output: Path, root: Path | None = None, app_version: int = 1) -> bytes:
    root_path = root if root is not None else directory / "editor-overlay-fit.prg"
    if not 1 <= app_version <= 0xFFFF:
        raise ValueError("app version must be in 1..65535")
    inspect_root(root_path)
    assets: list[tuple[Path, Path, bytes]] = []
    for _group, source_name, canonical_name in GROUPS:
        source = directory / source_name
        data = source.read_bytes()
        if not 1 <= len(data) <= SLOT_CAPACITY:
            raise ValueError(f"{source.name}: asset length must be 1..{SLOT_CAPACITY} bytes")
        _crc(data, source.name)
        assets.append((source, directory / canonical_name, data))
    for _source, canonical, data in assets:
        canonical.write_bytes(data)
    manifest = render(directory, root, app_version)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(manifest)
    return manifest


def validate(manifest_path: Path, directory: Path, root: Path | None = None,
             app_version: int = 1) -> None:
    expected = render(directory, root, app_version)
    actual = manifest_path.read_bytes()
    if actual != expected:
        raise ValueError("code asset manifest does not match root PRG and canonical code assets")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    parser.add_argument("--root", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--validate", type=Path)
    parser.add_argument("--app-version", type=int, default=1)
    args = parser.parse_args()
    output = args.output or args.directory / "S64OVL.MNF"
    try:
        if args.validate is not None:
            validate(args.validate, args.directory, args.root, args.app_version)
            print(f"code asset manifest validated: {args.validate}")
        else:
            generate(args.directory, output, args.root, args.app_version)
            print(f"code asset manifest written: {output}")
        return 0
    except (OSError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
