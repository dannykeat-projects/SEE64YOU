#!/usr/bin/env python3
"""Smoke-test actual package startup/exit without Ultimate UCI emulation.

This checks absent-UCI guidance and offline menus, not SD/network acceptance.
"""
from pathlib import Path
import argparse
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/vice"))
from test_reu_probe import wait_for_monitor


def screen(monitor):
    data = monitor.read_memory(0x0400, 0x07e7)
    return "".join(chr((value & 127) + 64) if (value & 127) < 32
                   else chr(value & 127) for value in data)


def wait_text(monitor, expected, timeout=30):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        text = screen(monitor)
        if expected.upper() in text.upper():
            return
        monitor.resume()
        time.sleep(0.05)
    raise AssertionError(f"Missing {expected!r}; screen={text!r}")


def key(monitor, value):
    monitor.write_memory(0x0277, bytes((value,)))
    monitor.write_memory(0x00c6, b"\x01")
    monitor.resume()


def run(filename, prompt, exit_key, port, rom_dir, checks=(), package_dir=None,
        prompt_timeout=30):
    package = package_dir or ROOT / "client/build/independent-package/SEE64YOU"
    out = ROOT / "client/build/independent-vice"
    out.mkdir(parents=True, exist_ok=True)
    command = ["xvfb-run", "-a", "x64sc", "+sound", "-reu", "-reusize", "16384",
               "-autostartprgmode", "1", "-binarymonitor",
               "-binarymonitoraddress", f"ip4://127.0.0.1:{port}"]
    for kind in ("kernal", "basic", "chargen"):
        rom = rom_dir / f"{kind}.rom"
        if not rom.is_file():
            raise RuntimeError(f"Missing ROM fixture: {rom}")
        command += [f"-{kind}", str(rom)]
    with (out / f"{filename}.log").open("w") as log:
        process = subprocess.Popen(command + ["-autostart", str(package / filename)],
                                   stdout=log, stderr=log)
        monitor = None
        try:
            monitor = wait_for_monitor("127.0.0.1", port, 20)
            wait_text(monitor, prompt, timeout=prompt_timeout)
            if filename == "S64WEB.PRG":
                for url, expected in ((b"HTTPS://EXAMPLE.COM/", "HTTPS unavailable"),
                                      (b"HTTP://EXAMPLE.COM/", "Ultimate network unavailable"),
                                      (b"HTTP://USER@EXAMPLE.COM/", "Invalid URL")):
                    key(monitor, ord("O"))
                    wait_text(monitor, "OPEN PUBLIC URL")
                    for value in url + b"\r":
                        key(monitor, value)
                        time.sleep(0.03)
                    wait_text(monitor, expected)
                key(monitor, 13)
                wait_text(monitor, "Invalid URL")
                print("VICE browser: URL prompt, HTTPS/refused URL, unavailable network, stale-link guard passed")
            if filename == "S64CFG.PRG":
                key(monitor, ord("M"))
                wait_text(monitor, "CONTINUE? Y/N")
                key(monitor, ord("N"))
                wait_text(monitor, "Session migration cancelled")
                wait_text(monitor, "STEP 1/3")
                key(monitor, ord("M"))
                wait_text(monitor, "CONTINUE? Y/N")
                key(monitor, ord("Y"))
                wait_text(monitor, "Migration needs Ultimate DOS")
                wait_text(monitor, "STEP 1/3")
                print("VICE Settings migration: cancel and absent-DOS refusal passed")
                key(monitor, ord("N"))
                wait_text(monitor, "Enter your website first")
                key(monitor, ord("1"))
                wait_text(monitor, "SITE URL:")
                for value in b"HTTP://EXAMPLE.COM\r":
                    key(monitor, value)
                    time.sleep(0.03)
                key(monitor, ord("N"))
                wait_text(monitor, "STEP 2/3")
                key(monitor, ord("N"))
                wait_text(monitor, "Import or enter a valid key first")
                key(monitor, ord("1"))
                wait_text(monitor, "Settings storage unavailable")
                key(monitor, ord("2"))
                wait_text(monitor, "DIRECT KEY:")
                for value in b"A" * 64:
                    key(monitor, value)
                    time.sleep(0.03)
                assert "********" in screen(monitor), "Key entry must remain masked"
                key(monitor, 13)
                wait_text(monitor, "Direct key replaced")
                key(monitor, ord("N"))
                wait_text(monitor, "STEP 3/3")
                key(monitor, ord("S"))
                wait_text(monitor, "HTTP consent required")
                key(monitor, ord("1"))
                wait_text(monitor, "ALLOW UNENCRYPTED HTTP? Y/N")
                key(monitor, ord("N"))
                wait_text(monitor, "Plain HTTP remains blocked")
                key(monitor, ord("1"))
                wait_text(monitor, "ALLOW UNENCRYPTED HTTP? Y/N")
                key(monitor, ord("Y"))
                key(monitor, ord("S"))
                wait_text(monitor, "SAVE FAILED - DOS UNAVAILABLE")
                key(monitor, ord("B"))
                wait_text(monitor, "STEP 2/3")
                key(monitor, ord("W"))
                wait_text(monitor, "A ADVANCED SETTINGS")
                print("VICE Settings setup: required fields, masked key, HTTP consent, "
                      "failed-save retention and back/basic navigation passed")
            for value, status in checks:
                key(monitor, value)
                wait_text(monitor, status)
            key(monitor, exit_key)
            wait_text(monitor, "READY.")
            # Prove BASIC executes input after exit rather than merely showing READY.
            monitor.write_memory(0xc000, b"\x00")
            for value in b"POKE49152,165\r":
                key(monitor, value)
                time.sleep(0.03)
            deadline = time.monotonic() + 10
            while time.monotonic() < deadline:
                if monitor.read_memory(0xc000, 0xc000) == b"\xa5":
                    print(f"VICE packaged {filename}: startup, {len(checks)} unavailable-service checks, "
                          "exit, responsive BASIC passed")
                    break
                monitor.resume()
                time.sleep(0.05)
            else:
                raise AssertionError(f"{filename}: BASIC did not execute POKE after exit")
            key(monitor, 147)
            for value in b'PRINT"YEH"\r':
                key(monitor, value)
                time.sleep(0.03)
            deadline = time.monotonic() + 10
            while time.monotonic() < deadline:
                text = screen(monitor)
                assert "OUT OF MEMORY" not in text, (filename, text)
                if any(text[i:i+40].startswith("YEH ") for i in range(0, 1000, 40)):
                    print(f"VICE packaged {filename}: BASIC string PRINT passed")
                    break
                monitor.resume()
                time.sleep(0.05)
            else:
                raise AssertionError(f"{filename}: BASIC string PRINT did not execute")
        finally:
            if monitor is not None:
                monitor.quit()
                monitor.close()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom-dir", type=Path, required=True,
                        help="Compatible locally provided C64 ROM directory")
    args = parser.parse_args()
    subprocess.run(["make", "-C", str(ROOT / "client"), "browser-palette-native-test"], check=True)
    run("PALTEST.PRG", "Native palette tests passed", ord("X"), 6548, args.rom_dir,
        ((ord("R"), "SEE64YOU PUBLIC BROWSER"), (3, "Native colour restore passed")),
        package_dir=ROOT / "client/build/browser-native-vice")
    run("SEE64YOU.PRG", "R RECHECK    X EXIT", ord("X"), 6544, args.rom_dir)
    run("S64PUB.PRG", "RUN/STOP  EXIT TO BASIC", 3, 6545, args.rom_dir,
        ((ord("3"), "Load .8TML or open remote first"),
         (ord("D"), "Open a remote page/post first"),
         (ord("L"), "C64U DOS unavailable"),
         (ord("2"), "C64U network target unavailable")))
    run("S64CFG.PRG", "STEP 1/3", 3, 6546, args.rom_dir,
        ((ord("S"), "SAVE FAILED - DOS UNAVAILABLE"),))
    run("S64WEB.PRG", "SEE64YOU PUBLIC BROWSER", 3, 6547, args.rom_dir)
