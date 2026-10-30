#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import tempfile
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "generate_module_manifest.py"
spec = importlib.util.spec_from_file_location("module_manifest_generator", SCRIPT)
module = importlib.util.module_from_spec(spec)
assert spec.loader is not None
sys.modules[spec.name] = module
spec.loader.exec_module(module)

def make_prg(path: Path, load: int, startup: int, tail: bytes) -> None:
    # BASIC line: next ptr, line number 10, SYS token, decimal target, NUL,
    # then zero next-line pointer and arbitrary machine-code bytes.
    sys_text = str(startup).encode("ascii")
    line = bytes((0x0B, 0x08, 10, 0, 0x9E)) + sys_text + b"\x00\x00\x00" + tail
    path.write_bytes(bytes((load & 0xFF, load >> 8)) + line)

def main() -> int:
    with tempfile.TemporaryDirectory() as td:
        directory = Path(td)
        for index, (_, filename) in enumerate(module.MODULES):
            make_prg(directory / filename, 0x0801, 2061 + index, bytes((index + 1,)) * 8)
        # A valid manifest always has three fixed named records, 4-digit
        # addresses, at most 5 decimal length digits, and 8-digit CRCs.
        # This must fit the launcher's smaller static input buffer.
        maximum = len(module.MAGIC) + 1
        for label, filename in module.MODULES:
            maximum += len(label) + len(filename) + 4 + 4 + 5 + 8 + 6
        if maximum > 192:
            raise AssertionError(f"valid manifest could exceed 192 bytes: {maximum}")
        rendered = module.render(directory)
        if len(rendered.encode("ascii")) > 192:
            raise AssertionError("rendered module manifest exceeds launcher bound")
        if not rendered.startswith(module.MAGIC + "\n"):
            raise AssertionError("manifest magic missing")
        lines = rendered.splitlines()
        if len(lines) != 4:
            raise AssertionError(f"unexpected line count: {len(lines)}")
        if "|0801|080D|" not in lines[1]:
            raise AssertionError(f"load/start metadata not derived: {lines[1]}")
        manifest = directory / "S64MODULE.MNF"
        manifest.write_text(rendered, encoding="ascii")
        module.validate(manifest, directory)

        broken = bytearray((directory / "S64PUB.PRG").read_bytes())
        broken[-1] ^= 0xFF
        (directory / "S64PUB.PRG").write_bytes(bytes(broken))
        try:
            module.validate(manifest, directory)
        except ValueError:
            pass
        else:
            raise AssertionError("changed PRG did not invalidate manifest")

    print("module manifest generator tests passed")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
