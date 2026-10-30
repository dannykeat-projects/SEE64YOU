#!/usr/bin/env python3
"""Exercise preflight, document and journal I/O with SD and USB-only namespaces."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    sources = ["client/tests/storage_context_test.c", "client/src/buffer.c", "client/src/document.c",
               "client/src/charset.c", "client/src/platform/dos.c", "client/src/storage/local.c",
               "client/src/platform/session_journal.c", "client/src/platform/session_journal_start.c",
               "client/src/platform/preflight.c", "client/src/platform/requirements.c"]
    with tempfile.TemporaryDirectory(prefix="s64-storage-paths-") as directory:
        binary = str(Path(directory) / "test")
        for root in ("/sd/", "/Usb0/"):
            for launcher in (False, True):
                defines = [f'-DS64_STORAGE_ROOT_DIRECTORY="{root}"']
                if launcher:
                    defines.append("-DS64_CODE_ASSET_LAUNCHER=1")
                subprocess.run(["cc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-Iclient/src",
                                *defines, *sources, "-o", binary], cwd=ROOT, check=True)
                subprocess.run([binary], cwd=ROOT, check=True)
                print(f"Storage root {root}; launcher={launcher}: passed", flush=True)
            for extra_sources, extra_flags in (
                (["client/tests/publisher_settings_store_test.c", "client/src/charset.c",
                  "client/src/publisher/settings_exchange.c", "client/src/publisher/settings_validate.c",
                  "client/src/publisher/settings_store.c", "client/src/publisher/standard_key_store.c",
                  "client/src/publisher/standard_keyfile.c", "client/src/publisher/standard_publish_names.c",
                  "client/src/publisher/direct_key.c"], ["-DS64_REU_BUFFER=1"]),
                (["client/tests/journal_migration_test.c", "client/src/platform/journal_migration.c",
                  "client/src/platform/session_journal.c", "client/src/platform/session_journal_start.c",
                  "client/src/platform/dos.c", "client/src/platform/dos_file.c"], []),
            ):
                subprocess.run(["cc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-Iclient/src",
                                f'-DS64_STORAGE_ROOT_DIRECTORY="{root}"', *extra_flags, *extra_sources,
                                "-o", binary], cwd=ROOT, check=True)
                subprocess.run([binary], cwd=ROOT, check=True)


if __name__ == "__main__":
    main()
