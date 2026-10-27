#!/usr/bin/env python3
"""Check standard protocol wire bytes with actual C64 charset and SHA-256."""
import argparse
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/vice"))
from test_reu_probe import wait_for_monitor


def check_result():
    monitor = wait_for_monitor("127.0.0.1", 6527, 30)
    deadline = time.monotonic() + 30
    try:
        while time.monotonic() < deadline:
            progress, result, detail = monitor.read_memory(0xC060, 0xC062)
            if result == 0xA5:
                return
            if result == 0x5A:
                raise AssertionError(f"Standard protocol case {detail} failed (progress={progress})")
            monitor.resume()
            time.sleep(.03)
        raise TimeoutError("Standard protocol diagnostic did not complete")
    finally:
        monitor.quit()
        monitor.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom-dir", type=Path, required=True)
    args = parser.parse_args()
    build = ROOT / "client/build/standard-protocol"
    build.mkdir(parents=True, exist_ok=True)
    objects = []
    for index, source in enumerate(("diagnostics/standard_protocol_probe.c",
            "src/publisher/standard_keyfile.c", "src/publisher/standard_publish.c",
            "src/publisher/standard_publish_names.c", "src/publisher/standard_reply.c",
            "src/publisher/standard_request.c", "src/publisher/direct_key.c",
            "src/publisher/direct_auth.c", "src/crypto/sha256.c", "src/charset.c")):
        obj = build / f"{index}.o"
        subprocess.run(["cl65", "-t", "c64", "-Os", "-DS64_NATIVE_PETSCII=1",
                        "-I", str(ROOT / "client/src"), "-c", "-o", str(obj),
                        str(ROOT / "client" / source)], check=True)
        objects.append(str(obj))
    prg = build / "standard-protocol.prg"
    subprocess.run(["cl65", "-t", "c64", "-o", str(prg), *objects], check=True)
    roms = []
    for name in ("kernal", "basic", "chargen"):
        path = args.rom_dir.resolve() / (name + ".rom")
        if not path.is_file():
            raise FileNotFoundError(path)
        roms += ["-" + name, str(path)]
    with (build / "vice.log").open("w") as log:
        process = subprocess.Popen(["xvfb-run", "-a", "x64sc", *roms,
            "+sound", "-warp", "-autostartprgmode", "1", "-binarymonitor",
            "-binarymonitoraddress", "ip4://127.0.0.1:6527", "-autostart", str(prg)],
            stdout=log, stderr=subprocess.STDOUT)
        try:
            check_result()
            print("PASS native LF/CRLF key import, put/list HMAC vectors, reply and request paths")
        finally:
            if process.poll() is None:
                process.terminate()
            process.wait(timeout=10)


if __name__ == "__main__":
    main()
