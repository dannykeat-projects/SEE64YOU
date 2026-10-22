#!/usr/bin/env python3
"""Generate diagnostic payloads with the actual cc65 Editor/REU implementations."""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import subprocess
import shutil
import tempfile
import re
import zlib

ROOT = Path(__file__).resolve().parents[1]
SOURCES = (
    "editor.c", "buffer_reu.c", "history_reu.c", "clipboard.c", "document.c", "charset.c",
    "platform/application_state.c", "platform/application_state_wire.c",
    "platform/application_state_payload.c", "platform/application_state_editor_decode.c",
    "platform/application_state_read.c", "platform/application_state_build.c",
    "platform/application_state_editor.c", "platform/application_state_reserve.c",
    "platform/reu_runtime.c", "platform/reu_runtime_build.c", "platform/reu_runtime_write.c",
    "platform/reu_runtime_allocate.c",
    "platform/session_journal.c", "platform/crc32.c",
)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "client/build/resume-editor-fixture")
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    objects = output / "objects"
    objects.mkdir(exist_ok=True)
    sources = [ROOT / "client/src" / source for source in SOURCES]
    sources.append(ROOT / "client/tests/resume_editor_fixture.c")
    compiled = []
    for index, source in enumerate(sources):
        stem = f"{index:02d}_{source.stem}"
        assembly, obj = objects / f"{stem}.s", objects / f"{stem}.o"
        # C64 charmap and ABI for production; ASCII paths/stdio in the mock backend.
        target = "sim6502" if source.name == "resume_editor_fixture.c" else "c64"
        subprocess.run(["cc65", "-t", target, "-Os", "-DS64_REU_BUFFER=1",
                        "-DS64_NATIVE_PETSCII=1", "-I", str(ROOT / "client/src"),
                        "-o", str(assembly), str(source)], check=True)
        subprocess.run(["ca65", "-t", "sim6502", "-o", str(obj), str(assembly)], check=True)
        compiled.append(str(obj))
    executable = output / "resume-editor-fixture"
    subprocess.run(["cl65", "-t", "sim6502", "-m", str(output / "fixture.map"),
                    "-o", str(executable), *compiled], check=True)
    with tempfile.TemporaryDirectory(prefix="s64-resume-") as scratch:
        subprocess.run(["sim65", str(executable)], cwd=scratch, check=True)
        for generated in Path(scratch).glob("*"):
            if generated.name == "resume_fixture.h" or generated.suffix == ".DAT":
                shutil.copyfile(generated, output / generated.name)
    header = (output / "resume_fixture.h").read_text()
    files = {"S64RDOC.DAT": "DOCUMENT", "S64RHP.DAT": "HISTORY_PAYLOAD",
             "S64RHM.DAT": "HISTORY_META", "S64RCLP.DAT": "CLIPBOARD", "S64RAPP.DAT": "STATE"}
    for name, macro in files.items():
        data = (output / name).read_bytes()
        length = re.search(rf"S64_RESUME_{macro}_LENGTH (\d+)U", header)
        crc = re.search(rf"S64_RESUME_{macro}_CRC 0x([0-9A-F]+)UL", header)
        if not length or not crc or len(data) != int(length[1]) or zlib.crc32(data) != int(crc[1], 16):
            raise ValueError(f"Generated fixture length/CRC mismatch: {name}")
        print(f"{name}: {len(data)} bytes SHA256 {hashlib.sha256(data).hexdigest()}")
    state = (output / "S64RAPP.DAT").read_bytes()
    if state[8:32] != bytes(24) or state[508:512] != bytes(4):
        raise ValueError("Fixture template must have no journal binding or authenticated CRC")


if __name__ == "__main__":
    main()
