#!/usr/bin/env python3
"""Build verified SD/USB SEE64YOU client and PHP site packages."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
FILES = {"SEE64YOU.PRG", "S64EDIT.PRG", "S64PUB.PRG", "S64CFG.PRG",
         "S64FILE.BIN", "S64MODEL.BIN", "S64PREP.BIN", "S64RUN.BIN", "S64MODULE.MNF", "S64OVL.MNF"}


def capture(*command):
    return subprocess.check_output(command, cwd=ROOT, text=True, stderr=subprocess.STDOUT).strip()


def digest(data):
    return hashlib.sha256(data).hexdigest()


def committed_source():
    # Match the Windows checkout's Git text normalization under WSL. Explicit
    # binary attributes still apply; do not ignore content or whitespace changes.
    git = ("git", "-c", "core.autocrlf=true")
    for command in (git + ("diff", "--exit-code"), git + ("diff", "--cached", "--exit-code")):
        subprocess.run(command, cwd=ROOT, check=True, stdout=subprocess.DEVNULL)
    return capture("git", "rev-parse", "HEAD")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--hardware-receipt", type=Path, required=True)
    args = parser.parse_args()
    if not re.fullmatch(r"v[0-9]+\.[0-9]+\.[0-9]+-alpha\.[0-9]+", args.version):
        raise ValueError("Expected a new alpha version")
    output = args.output.resolve()
    if not output.is_relative_to(ROOT / "client/build"):
        raise ValueError("Release output must be a new directory under client/build")
    commit = committed_source()
    guide = (ROOT / "docs/INSTALL.md").read_text()
    receipt = json.loads(args.hardware_receipt.read_text())
    if not receipt.get("restored") or receipt.get("active") or not receipt.get("dos_context_restored"):
        raise ValueError("Hardware receipt must confirm complete restoration")
    hardware = {"tested_commit": receipt["commit"], "input": receipt["input"],
                "physical_keyboard": receipt["physical_keyboard"],
                "cold_power_cycle": receipt["cold_power_cycle"], "restored": True}
    version = capture("cl65", "--version")
    timestamp = datetime.datetime.fromtimestamp(int(capture("git", "show", "-s", "--format=%ct", "HEAD")),
                                                 datetime.timezone.utc).timetuple()[:6]
    output.mkdir(parents=True, exist_ok=False)
    archives = {}

    def archive(name, entries):
        path = output / name
        with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zipped:
            for filename, data in sorted(entries.items()):
                info = zipfile.ZipInfo(filename, timestamp)
                info.compress_type = zipfile.ZIP_DEFLATED
                info.external_attr = 0o100644 << 16
                zipped.writestr(info, data)
        with zipfile.ZipFile(path) as zipped:
            assert zipped.testzip() is None and set(zipped.namelist()) == set(entries)
            assert all(zipped.read(filename) == data for filename, data in entries.items())
        archives[name] = digest(path.read_bytes())
        print("Verified " + name, flush=True)

    for label, root in (("usb0", "/Usb0/"), ("sd", "/sd/")):
        command = ["make", "-B", "-C", "client", "STORAGE_ROOT=" + root, "independent-package"]
        with (output / (label + "-build.log")).open("w") as log:
            subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
        package = ROOT / "client/build/independent-package/SEE64YOU"
        assert {path.name for path in package.iterdir() if path.is_file()} == FILES
        payload = {"SEE64YOU/" + name: (package / name).read_bytes() for name in FILES}
        hashes = {name: digest(payload["SEE64YOU/" + name]) for name in sorted(FILES)}
        if label == "sd":
            assert hashes == {name: receipt["package_sha256"][name] for name in FILES}, \
                "Release SD binaries differ from hardware-tested candidate"
        build = {"version": args.version, "source_commit": commit, "storage_root": root,
                 "build_command": " ".join(command), "toolchain": version, "file_count": len(FILES),
                 "package_sha256": hashes, "sd_editor_hardware": hardware,
                 "acceptance": "Editor alpha preview; physical keyboard, cold-power, USB-only and Publisher hardware gates pending"}
        payload["BUILD.json"] = (json.dumps(build, indent=2) + "\n").encode()
        payload["README.md"] = guide.encode()
        payload["SHA256SUMS"] = "".join(hashes[name] + "  SEE64YOU/" + name + "\n" for name in sorted(FILES)).encode()
        archive("SEE64YOU-" + args.version + "-c64u-" + label + ".zip", payload)

    # Snapshot tracked site sources only: installed pages/configuration or local
    # runtime files must never enter a distributable archive.
    with tempfile.TemporaryDirectory(prefix="site-snapshot-", dir=output) as temporary:
        snapshot = Path(temporary)
        tracked = subprocess.check_output(["git", "ls-files", "-z", "--", "site"], cwd=ROOT).split(b"\0")
        for item in filter(None, tracked):
            relative = Path(item.decode())
            if relative == Path("site/config.php"):
                raise ValueError("Refusing tracked private configuration")
            target = snapshot / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / relative, target)
        packaged = output / "see64you-site"
        subprocess.run(["php", "-r", "require $argv[1]; s64_package_site($argv[2], $argv[3]);",
                        str(ROOT / "tools/package-site.php"), str(snapshot), str(packaged)], check=True)
    entries = {"see64you-site/" + path.relative_to(packaged).as_posix(): path.read_bytes()
               for path in packaged.rglob("*") if path.is_file()}
    assert "see64you-site/config.php" not in entries and "see64you-site/ENABLE_SETUP" in entries
    assert not any(name.startswith("see64you-site/tests/") for name in entries)
    entries["README.md"] = guide.encode()
    entries["BUILD.json"] = (json.dumps({"version": args.version, "source_commit": commit,
                                        "files": {name: digest(data) for name, data in sorted(entries.items())}},
                                       indent=2) + "\n").encode()
    archive("SEE64YOU-" + args.version + "-php-site.zip", entries)
    if committed_source() != commit:
        raise RuntimeError("Source advanced during packaging; do not publish these assets")
    (output / "SHA256SUMS").write_text("".join(value + "  " + name + "\n" for name, value in sorted(archives.items())))
    print("Release assets verified; default SD build restored. Nothing uploaded.")


if __name__ == "__main__":
    main()
