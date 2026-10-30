#!/usr/bin/env python3
"""Focused fail-closed and deterministic standard alpha packaging checks."""
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile

SPEC = importlib.util.spec_from_file_location("packager", Path(__file__).resolve().parents[1] / "package_8tml_release.py")
PACK = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PACK)


class PackageTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix="s64-release-", dir=PACK.ROOT / "client/build")
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        (self.root / "client/src").mkdir(parents=True)
        (self.root / "client/src/test.c").write_bytes(b"tested source")
        (self.root / "client/build").mkdir()
        (self.root / "docs").mkdir()
        (self.root / "docs/INSTALL.md").write_bytes(b"alpha installation guide\n")
        self.hashes = {name: PACK.digest(name.encode()) for name in PACK.FILES}
        self.validation = {"base_commit": "base", "source_sha256": {
            "client/src/test.c": PACK.digest(b"tested source")},
            "packages": {label: {"storage_root": root, "sha256": self.hashes}
                         for label, root in (("sd", "/sd/"), ("usb", "/USB1/"))},
            "gates": {name: "passed" for name in ("focused_source_and_storage", "SD_USB_fit_manifests",
                "SD_USB_packaged_VICE_startup_BASIC_return", "host_suite", "C64U_SD_disk_restoration")}}
        self.receipt = {"commit": "base", "restored": True, "active": False, "dos_context_restored": True,
                        "storage_root": "/sd/", "package_sha256": dict(self.hashes, **{"T8PROBE.PRG": "probe"}),
                        "input": "REST keyboard-buffer injection", "physical_keyboard": False,
                        "cold_power_cycle": False}
        self.root_patch = patch.object(PACK, "ROOT", self.root)
        self.root_patch.start()
        self.addCleanup(self.root_patch.stop)
        self.run_patch = patch.object(PACK.subprocess, "run")
        self.run_patch.start()
        self.addCleanup(self.run_patch.stop)
        self.capture_patch = patch.object(PACK, "capture", side_effect=lambda *args:
            "client/src/test.c" if "diff" in args else "1793232000")
        self.capture_patch.start()
        self.addCleanup(self.capture_patch.stop)

    def test_receipt_rejections(self):
        PACK.verify_receipts(self.validation, self.receipt, "release")
        for kind in ("extra", "restoration", "package", "root", "gate", "runtime", "missing-runtime",
                     "native-only", "UI-not-tested"):
            validation, receipt = copy.deepcopy(self.validation), copy.deepcopy(self.receipt)
            if kind == "extra": receipt["package_sha256"]["UNEXPECTED.PRG"] = "x"
            if kind == "restoration": receipt["restored"] = False
            if kind == "package": receipt["package_sha256"]["SEE64YOU.PRG"] = "wrong"
            if kind == "root": validation["packages"]["usb"]["storage_root"] = "/Usb0/"
            if kind == "gate": validation["gates"]["host_suite"] = "failed"
            if kind == "runtime": validation["source_sha256"]["client/src/test.c"] = "wrong"
            if kind == "missing-runtime": validation["source_sha256"] = {}
            if kind == "native-only": receipt["storage_only"] = True
            if kind == "UI-not-tested": receipt["shipping_editor_UI_tested"] = False
            with self.subTest(kind=kind), self.assertRaises(ValueError):
                PACK.verify_receipts(validation, receipt, "release")

    def test_nested_package_inventory_fails_closed(self):
        package = self.root / "layout"
        package.mkdir()
        for name in PACK.FILES:
            item = package / name
            item.parent.mkdir(parents=True, exist_ok=True)
            item.write_bytes(name.encode())
        payload, hashes = PACK.package_bytes(package, self.hashes)
        self.assertEqual(set(payload), PACK.FILES)
        self.assertEqual(hashes, self.hashes)

        # Flat-layout support files must never accidentally be released.
        flat = package / "S64OVL.MNF"
        flat.write_bytes(b"obsolete")
        with self.assertRaises(ValueError):
            PACK.package_bytes(package, self.hashes)
        flat.unlink()

        nested = package / "SYSTEM" / "UNEXPECTED.BIN"
        nested.write_bytes(b"not part of the package")
        with self.assertRaises(ValueError):
            PACK.package_bytes(package, self.hashes)
        nested.unlink()

        extra_directory = package / "ARCHIVE"
        extra_directory.mkdir()
        with self.assertRaises(ValueError):
            PACK.package_bytes(package, self.hashes)
        extra_directory.rmdir()

        missing = package / "SYSTEM/S64MODULE.MNF"
        saved = missing.read_bytes()
        missing.unlink()
        with self.assertRaises(ValueError):
            PACK.package_bytes(package, self.hashes)
        missing.write_bytes(saved)

        # Reject a symlinked root rather than silently .resolve()ing it.
        linked = self.root / "linked-package"
        try:
            linked.symlink_to(package, target_is_directory=True)
        except (OSError, NotImplementedError):
            pass  # Symlink creation can be restricted on native Windows.
        else:
            with self.assertRaises(ValueError):
                PACK.package_bytes(linked, self.hashes)

    def test_dirty_source_rejected(self):
        with patch.object(PACK.subprocess, "run", side_effect=subprocess.CalledProcessError(1, "git diff")):
            with self.assertRaises(subprocess.CalledProcessError):
                PACK.committed_source()

    def test_snapshots_and_deterministic_client_archives(self):
        for label in ("sd", "usb"):
            package = self.root / label
            package.mkdir()
            for name in PACK.FILES:
                file = package / name
                file.parent.mkdir(parents=True, exist_ok=True)
                file.write_bytes(name.encode())
        extra = self.root / "sd/EXTRA"
        extra.write_bytes(b"extra")
        with self.assertRaises(ValueError): PACK.package_bytes(self.root / "sd", self.hashes)
        extra.unlink()
        with self.assertRaises(ValueError): PACK.package_bytes(self.root / "sd", {})
        for name, data in (("validation.json", self.validation), ("hardware.json", self.receipt)):
            (self.root / name).write_text(json.dumps(data))
        outputs = []
        for attempt in ("one", "two"):
            output = self.root / "client/build" / attempt
            args = ["packager", "--version", "v0.2.0-alpha.4", "--output", str(output),
                    "--sd-package", str(self.root / "sd"), "--usb-package", str(self.root / "usb"),
                    "--validation-receipt", str(self.root / "validation.json"),
                    "--hardware-receipt", str(self.root / "hardware.json")]
            with patch.object(sys, "argv", args), patch.object(PACK, "committed_source", return_value="release"):
                PACK.main()
            outputs.append({path.name: path.read_bytes() for path in output.iterdir()})
            self.assertEqual(len(outputs[-1]), 3)
            for path in output.glob("*.zip"):
                self.assertNotIn("php", path.name)
                with zipfile.ZipFile(path) as archive:
                    self.assertEqual(len(archive.namelist()), 13)
                    members = set(archive.namelist())
                    self.assertIn("SEE64YOU/SYSTEM/S64OVL.MNF", members)
                    self.assertIn("SEE64YOU/SYSTEM/S64MODULE.MNF", members)
                    self.assertNotIn("SEE64YOU/S64OVL.MNF", members)
                    self.assertNotIn("SEE64YOU/S64MODULE.MNF", members)
                    build = json.loads(archive.read("BUILD.json"))
                    self.assertEqual(build["tested_base_commit"], "base")
                    self.assertEqual(build["release_commit"], "release")
                    self.assertEqual(build["guide_sha256"], PACK.digest(archive.read("README.md")))
                    self.assertTrue(build["remaining_gates"])
        self.assertEqual(outputs[0], outputs[1])


if __name__ == "__main__":
    unittest.main()
