#!/usr/bin/env python3
"""Run deterministic production REU editor tests; timeout 120s per process.
Usage (Linux/WSL): python3 tools/test_editor_stress.py
Requires a C compiler; temporary executable is removed automatically.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitize', action='store_true', help='enable address and undefined-behavior sanitizers')
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="s64-editor-stress-") as temp:
    binary = str(Path(temp) / "editor-stress")
    command = [os.environ.get("CC", "cc"), "-std=c99", "-O1", "-g", "-Wall", "-Wextra", "-Werror", "-DS64_REU_BUFFER=1", "-Iclient/src", "client/tests/editor_stress_test.c", "client/src/buffer_reu.c", "client/src/clipboard.c", "client/src/history_reu.c", "client/src/editor.c", "-o", binary]
    if args.sanitize:
        command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    subprocess.run(command, cwd=root, check=True, timeout=120)
    subprocess.run([binary], cwd=root, check=True, timeout=120)
