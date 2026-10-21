#!/usr/bin/env python3
"""Check Editor source persistence; keep the licensed reference tools external."""
import argparse
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "client/build/8tml-tests"


def run(*command):
    subprocess.run(command, cwd=ROOT, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference-tools", type=Path, help="External specs/tools directory")
    parser.add_argument("--rom-dir", type=Path, help="Also run actual cc65 codec under VICE")
    args = parser.parse_args()
    BUILD.mkdir(parents=True, exist_ok=True)
    common = ["client/src/buffer.c", "client/src/document.c", "client/src/charset.c"]
    for name in ("document", "local"):
        sources = common + (["client/src/platform/dos.c", "client/src/storage/local.c"]
                            if name == "local" else [])
        binary = BUILD / f"{name}-test"
        run("cc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-DS64_INDEPENDENT_EDITOR=1",
            "-Iclient/src", f"client/tests/8tml_{name}_test.c", *sources, "-o", str(binary))
        run(str(binary))
    binary = BUILD / "publisher-local-test"
    run("cc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-DS64_INDEPENDENT_PUBLISHER=1",
        "-Iclient/src", "client/tests/8tml_local_test.c", *common,
        "client/src/platform/dos.c", "client/src/storage/local.c", "-o", str(binary))
    run(str(binary))
    binary = BUILD / "reu-document-test"
    run("cc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-DS64_INDEPENDENT_EDITOR=1",
        "-DS64_REU_BUFFER=1", "-Iclient/src", "client/tests/8tml_document_test.c",
        "client/src/buffer_reu.c", *common[1:], "-o", str(binary))
    run(str(binary))
    if args.reference_tools:
        run(sys.executable, str(args.reference_tools.resolve() / "8tml"), "check", "-v",
            "client/build/8tml-empty.8tml", "client/build/8tml-roundtrip.8tml")
    if args.rom_dir:
        objects = []
        for source in ["client/tests/8tml_document_test.c", *common]:
            obj = BUILD / (Path(source).stem + ".o")
            run("cl65", "-t", "c64", "-Os", "-DS64_INDEPENDENT_EDITOR=1",
                "-DS64_NATIVE_PETSCII=1", "-Iclient/src", "-c", "-o", str(obj), source)
            objects.append(str(obj))
        run("cl65", "-t", "c64", *objects, "-o", str(BUILD / "TMLTEST.PRG"))
        sys.path.insert(0, str(ROOT / "tools"))
        from test_independent_programs_vice import run as vice_run
        vice_run("TMLTEST.PRG", "8TML native tests passed", ord("X"), 6551,
                 args.rom_dir.resolve(), package_dir=BUILD, prompt_timeout=180)


if __name__ == "__main__":
    main()
