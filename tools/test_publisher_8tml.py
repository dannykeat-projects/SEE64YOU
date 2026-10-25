#!/usr/bin/env python3
"""Run production native Publisher code against an isolated local PHP source service."""
import argparse
import json
import os
from pathlib import Path
import secrets
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "client/build/publisher-8tml-tests"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference-tools", type=Path)
    parser.add_argument("--rom-dir", type=Path, help="Also test cc65/PETSCII JSON under VICE")
    args = parser.parse_args()
    BUILD.mkdir(parents=True, exist_ok=True)
    json_sources = ["client/tests/publisher_source_json_test.c", "client/src/buffer.c",
                    "client/src/document.c", "client/src/charset.c", "client/src/publisher/source_json.c",
                    "client/src/publisher/workspace.c"]
    json_test = BUILD / "source-json-test"
    subprocess.run(["cc", "-std=c99", "-Wall", "-Wextra", "-Werror",
                    "-DS64_INDEPENDENT_PUBLISHER=1", "-Iclient/src", *json_sources,
                    "-o", str(json_test)], cwd=ROOT, check=True)
    subprocess.run([str(json_test)], cwd=ROOT, check=True)
    binary = BUILD / "publisher-http-test"
    sources = ["client/tests/publisher_8tml_http_test.c", "client/src/buffer_reu.c",
               "client/src/document.c", "client/src/charset.c", "client/src/platform/network.c",
               "client/src/crypto/sha256.c", "client/src/publisher/direct_key.c",
               "client/src/publisher/direct_key_store.c", "client/src/publisher/direct_auth.c",
               "client/src/publisher/workspace.c", "client/src/publisher/standalone_direct.c",
               "client/src/publisher/source_json.c", "client/src/publisher/destination.c"]
    subprocess.run(["cc", "-std=c99", "-Wall", "-Wextra", "-DS64_REU_BUFFER=1",
                    "-DS64_INDEPENDENT_PUBLISHER=1", "-Iclient/src", *sources,
                    "-o", str(binary)], cwd=ROOT, check=True)
    with tempfile.TemporaryDirectory(prefix="s64-native-8tml-") as directory:
        site = Path(directory) / "site"
        shutil.copytree(ROOT / "site", site, ignore=shutil.ignore_patterns(
            "config.php", "ENABLE_SETUP", "data", "pages", "posts", "tests"))
        key = secrets.token_hex(32)
        (site / "config.php").write_text("<?php return ['password_hash'=>'unused-test-hash',"
            "'token_secret'=>'" + secrets.token_hex(32) + "','direct_secret'=>'" + key + "'];\n")
        with socket.socket() as sock:
            sock.bind(("127.0.0.1", 0))
            port = sock.getsockname()[1]
        env = dict(os.environ, S64_TEST_KEY=key)
        with (Path(directory) / "server.log").open("w") as log:
            server = subprocess.Popen(["php", "-S", f"127.0.0.1:{port}", "-t", str(site)],
                                      stdout=log, stderr=log)
            try:
                for _ in range(50):
                    try:
                        with urllib.request.urlopen(f"http://127.0.0.1:{port}/api.php?action=status", timeout=2):
                            break
                    except urllib.error.URLError:
                        time.sleep(.1)
                else:
                    raise AssertionError("Disposable PHP server did not start")

                def run(phase):
                    subprocess.run([str(binary), str(port), phase, str(BUILD)],
                                   cwd=ROOT, env=env, check=True, timeout=120)

                def record(path):
                    guard, payload = path.read_text().split("\n", 1)
                    assert guard == "<?php http_response_code(404); exit; ?>"
                    return json.loads(payload)

                run("create")
                fixtures = (("pages/native-page.8tml.php", "publisher-small.8tml"),
                            ("posts/native-post.8tml.php", "publisher-large.8tml"),
                            ("pages/empty-page.8tml.php", "publisher-empty.8tml"))
                for stored, exported in fixtures:
                    document = record(site / stored)
                    assert document["raw8tml"].encode("ascii") == (BUILD / exported).read_bytes()
                post_path = site / "posts/native-post.8tml.php"
                post = record(post_path)
                assert len(post["raw8tml"]) == 61440 and post["status"] == "published"
                post["date"], post["order"] = "2020-01-02T03:04:05+00:00", -2147483648
                post_path.write_text("<?php http_response_code(404); exit; ?>\n" + json.dumps(post))
                run("update")
                updated = record(post_path)
                assert updated["raw8tml"] == post["raw8tml"] and updated["status"] == "draft"
                assert (updated["date"], updated["order"]) == (post["date"], post["order"])
                run("delete")
                assert not list((site / "pages").glob("*.8tml.php"))
                assert not list((site / "posts").glob("*.8tml.php"))
                if args.reference_tools:
                    subprocess.run(["python3", str(args.reference_tools.resolve() / "8tml"), "check", "-v",
                                    *(str(BUILD / filename) for _, filename in fixtures)], check=True)
            finally:
                server.terminate()
                try:
                    server.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    server.kill()
                    server.wait(timeout=5)
    print("native Publisher/PHP 8TML source round-trip tests passed")
    if args.rom_dir:
        objects = []
        for source in json_sources:
            obj = BUILD / (Path(source).stem + ".o")
            subprocess.run(["cl65", "-t", "c64", "-Os", "-DS64_INDEPENDENT_PUBLISHER=1",
                            "-DS64_NATIVE_PETSCII=1", "-Iclient/src", "-c", "-o", str(obj), source],
                           cwd=ROOT, check=True)
            objects.append(str(obj))
        subprocess.run(["cl65", "-t", "c64", *objects, "-o", str(BUILD / "PUBJSON.PRG")],
                       cwd=ROOT, check=True)
        sys.path.insert(0, str(ROOT / "tools"))
        from test_independent_programs_vice import run as vice_run
        vice_run("PUBJSON.PRG", "8TML JSON tests passed", ord("X"), 6552,
                 args.rom_dir.resolve(), package_dir=BUILD, prompt_timeout=90)


if __name__ == "__main__":
    main()
