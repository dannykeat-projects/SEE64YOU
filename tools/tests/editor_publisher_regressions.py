"""Host gates shared by the shell and PowerShell validation entry points."""
import argparse
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default="cc")
    parser.add_argument("--sanitizers", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2] / "client"
    common = [args.cc, "-std=c99", "-Wall", "-Wextra", "-Isrc"]
    publisher = [
        "tests/standard_publisher_roundtrip_test.c", "src/document.c", "src/charset.c",
        "src/platform/dos.c", "src/storage/local.c", "src/publisher/standard_publish_names.c",
        "src/publisher/standard_transport.c", "src/publisher/standard_request.c",
        "src/publisher/standard_reply.c", "src/publisher/standard_publish.c",
        "src/publisher/direct_auth.c", "src/publisher/direct_key.c", "src/crypto/sha256.c",
    ]
    reu = ["-DS64_REU_BUFFER=1"]
    publish = ["-DS64_INDEPENDENT_PUBLISHER=1", "-DS64_STORAGE_READ_ONLY=1"]
    sanitize = ["-fsanitize=address,undefined"]
    display = ["-Itests/ui_prompt_stubs", "tests/standard_result_screen_test.c",
               "src/publisher/standard_display.c"]
    cases = [
        ("REU window scans", reu + ["tests/reu_window_scan_test.c", "src/buffer_reu.c",
                                    "src/clipboard.c", "src/history_reu.c", "src/editor.c"]),
        ("Publisher round-trip", publish + publisher + ["src/buffer.c"]),
        ("Publisher round-trip with REU", publish + reu + publisher + ["src/buffer_reu.c"]),
        ("Publisher result screen", ["-Werror"] + display),
    ]
    if args.sanitizers:
        cases.extend([
            ("Publisher round-trip with sanitizers", publish + sanitize + publisher + ["src/buffer.c"]),
            ("Publisher result screen with sanitizers", sanitize + display),
        ])
    with tempfile.TemporaryDirectory() as tmp:
        binary = str(Path(tmp) / "regression.exe")
        for name, args in cases:
            print(f"== {name} ==", flush=True)
            subprocess.run(common + args + ["-o", binary], cwd=root, check=True)
            subprocess.run([binary], cwd=root, check=True)


if __name__ == "__main__":
    main()
