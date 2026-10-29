#!/usr/bin/env python3
"""Package validated standard SD/USB1 client snapshots as alpha releases."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
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


def verify_receipts(validation, receipt, commit):
    if receipt.get("restored") is not True or receipt.get("active") is not False \
            or receipt.get("dos_context_restored") is not True or receipt.get("storage_root") != "/sd/":
        raise ValueError("Hardware receipt must confirm complete SD restoration")
    if set(receipt.get("package_sha256", {})) not in (FILES, FILES | {"T8PROBE.PRG"}):
        raise ValueError("Hardware inventory must be ten shipping files plus optional T8PROBE.PRG")
    required_gates = {"focused_source_and_storage", "SD_USB_fit_manifests",
                      "SD_USB_packaged_VICE_startup_BASIC_return", "host_suite", "C64U_SD_disk_restoration"}
    if any(validation.get("gates", {}).get(name) != "passed" for name in required_gates):
        raise ValueError("Required software/package/SD hardware gates have not passed")
    for label, root in (("sd", "/sd/"), ("usb", "/USB1/")):
        package = validation.get("packages", {}).get(label, {})
        if package.get("storage_root") != root or set(package.get("sha256", {})) != FILES:
            raise ValueError("Validation must hash ten files for /sd/ and exact /USB1/")
    if validation["packages"]["sd"]["sha256"] != {name: receipt["package_sha256"][name] for name in FILES}:
        raise ValueError("SD validation and hardware package hashes differ")
    base = validation["base_commit"]
    if receipt["commit"] != base:
        raise ValueError("Validation and hardware receipts have different tested base commits")
    subprocess.run(["git", "merge-base", "--is-ancestor", base, commit], cwd=ROOT, check=True)
    hashes = validation.get("source_sha256", {})
    def runtime(name):
        return name.startswith(("client/src/", "client/diagnostics/"))
    changed = capture("git", "diff", "--name-only", base, commit).splitlines()
    if any(name not in hashes for name in changed if runtime(name)):
        raise ValueError("Runtime changes are missing from validation source hashes")
    verified = {name: value for name, value in hashes.items() if runtime(name)}
    if not verified:
        raise ValueError("Validation receipt has no runtime source hashes")
    for name, expected in verified.items():
        path = (ROOT / name).resolve()
        if not path.is_relative_to(ROOT) or not path.is_file() or digest(path.read_bytes()) != expected:
            raise ValueError("Validated runtime source differs: " + name)
    for name, expected in receipt.get("test_source_sha256", {}).items():
        if hashes.get(name) != expected:
            raise ValueError("Hardware and validation source hashes differ: " + name)
    return verified


def package_bytes(package, expected):
    if not package.is_dir() or package.is_symlink():
        raise ValueError("Expected a package directory")
    paths = list(package.iterdir())
    if {path.name for path in paths} != FILES or any(not p.is_file() or p.is_symlink() for p in paths):
        raise ValueError("Package must contain exactly the ten shipping files")
    payload = {name: (package / name).read_bytes() for name in sorted(FILES)}
    hashes = {name: digest(data) for name, data in payload.items()}
    if hashes != expected:
        raise ValueError("Package differs from the validation receipt: " + str(package))
    for script, manifest in (("generate_module_manifest.py", "S64MODULE.MNF"),
                             ("generate_code_asset_manifest.py", "S64OVL.MNF")):
        command = [sys.executable, str(ROOT / "tools" / script), str(package)]
        if "code_asset" in script:
            command += ["--root", str(package / "S64EDIT.PRG")]
        subprocess.run(command + ["--validate", str(package / manifest)], cwd=ROOT, check=True)
    return payload, hashes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--hardware-receipt", type=Path, required=True)
    parser.add_argument("--sd-package", type=Path, required=True)
    parser.add_argument("--usb-package", type=Path, required=True)
    parser.add_argument("--validation-receipt", type=Path, required=True)
    args = parser.parse_args()
    if not re.fullmatch(r"v[0-9]+\.[0-9]+\.[0-9]+-alpha\.[0-9]+", args.version):
        raise ValueError("Expected a new alpha version")
    output = args.output.resolve()
    if not output.is_relative_to(ROOT / "client/build"):
        raise ValueError("Release output must be a new directory under client/build")
    commit = committed_source()
    guide = (ROOT / "docs/INSTALL.md").read_bytes()
    hardware_data = args.hardware_receipt.read_bytes()
    validation_data = args.validation_receipt.read_bytes()
    receipt = json.loads(hardware_data)
    validation = json.loads(validation_data)
    verified_source = verify_receipts(validation, receipt, commit)
    snapshots = {label: package_bytes(package.resolve(), validation["packages"][label]["sha256"])
                 for label, package in (("sd", args.sd_package), ("usb", args.usb_package))}
    hardware = {"tested_commit": receipt["commit"], "input": receipt["input"],
                "physical_keyboard": receipt["physical_keyboard"],
                "cold_power_cycle": receipt["cold_power_cycle"], "restored": True}
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
            if zipped.testzip() is not None or set(zipped.namelist()) != set(entries) \
                    or any(zipped.read(filename) != data for filename, data in entries.items()):
                raise RuntimeError("Archive verification failed")
        archives[name] = digest(path.read_bytes())
        print("Verified " + name, flush=True)

    for label, root in (("sd", "/sd/"), ("usb1", "/USB1/")):
        key = "usb" if label == "usb1" else label
        files, hashes = snapshots[key]
        payload = {"SEE64YOU/" + name: data for name, data in files.items()}
        build = {"version": args.version, "source_commit": commit, "storage_root": root,
                 "release_commit": commit, "tested_base_commit": validation["base_commit"],
                 "packaging": "Exact validated snapshot; no rebuild", "file_count": len(FILES),
                 "toolchain": validation.get("toolchain"),
                 "build_command": validation["packages"][key].get("build_command"),
                 "package_sha256": hashes, "sd_editor_hardware": hardware,
                 "guide_sha256": digest(guide), "validation_receipt_sha256": digest(validation_data),
                 "hardware_receipt_sha256": digest(hardware_data),
                 "validated_runtime_source_sha256": verified_source,
                 "validation_gates": validation["gates"],
                 "fit_summary": validation["packages"][key].get("fit_summary", []),
                 "acceptance": "Alpha preview; remaining gates are not certified by this release",
                 "remaining_gates": ["physical keyboard/Link on this package", "cold-power exact-source reopen",
                    "uncontrolled interruption/recovery", "real USB-only operation",
                    "external compatible-reader navigation", "Publisher/Settings hardware on this exact package",
                    "historical non-shipping native matrix reconciliation"]}
        payload["BUILD.json"] = (json.dumps(build, indent=2, sort_keys=True) + "\n").encode()
        payload["README.md"] = guide
        payload["SHA256SUMS"] = "".join(digest(data) + "  " + name + "\n"
                                         for name, data in sorted(payload.items())).encode()
        archive("SEE64YOU-" + args.version + "-c64u-" + label + ".zip", payload)

    if committed_source() != commit:
        raise RuntimeError("Source advanced during packaging; do not publish these assets")
    (output / "SHA256SUMS").write_bytes("".join(value + "  " + name + "\n"
                                              for name, value in sorted(archives.items())).encode())
    print("Validated standard SD/USB1 alpha assets verified. Nothing uploaded.")


if __name__ == "__main__":
    main()
