#!/usr/bin/env python3
"""REST-assisted 8TML Editor/real-DOS acceptance on an idle C64U.

Requires explicit hardware/file-write authorization and an empty BASIC session.
Backs up the entire app folder; never resets hardware or tests power loss.
Receipt backups can contain private configuration. Keep them outside Git.
"""
import argparse
import ftplib
import hashlib
import io
import json
from pathlib import Path
import re
import subprocess
import time
import urllib.parse
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
FILES = {"SEE64YOU.PRG", "S64EDIT.PRG", "S64PUB.PRG", "S64CFG.PRG", "S64WEB.PRG",
         "S64FILE.BIN", "S64MODEL.BIN", "S64PREP.BIN", "S64RUN.BIN", "S64MODULE.MNF", "S64OVL.MNF"}
EMPTY = b"---\n8tml: 1\ntitle: Untitled\n---\n"
SOURCE = (b"---\r\n8tml: 1\r\ntitle: Hardware source\r\nX-Future:  Keep : CASE  \r\n"
          b"# preserve this comment\r\n---\r\n<unknown>Literal text</unknown>\r\n"
          b":::future data\r\nopaque\ttext\r\n:::\r\n")


def native(text):
    return bytes(c - 32 if 97 <= c <= 122 else c + 128 if 65 <= c <= 90 else c
                 for c in text.encode("ascii"))


def storage_root(text):
    if not re.fullmatch(r"/[A-Za-z0-9][A-Za-z0-9_-]*/", text):
        raise argparse.ArgumentTypeError("Use one absolute volume with a trailing slash, e.g. /sd/ or /USB1/")
    return text


def build_probe(out, selected_root):
    sources = ["diagnostics/8tml_storage_probe.c", "src/buffer_reu.c", "src/document.c",
               "src/charset.c", "src/storage/local.c", "src/platform/dos.c", "src/platform/uci.c",
               "src/platform/reu.c", "src/platform/reu_dma.s", "src/platform/basic_workspace_reset.s"]
    objects = []
    for index, source in enumerate(sources):
        obj = out / f"probe-{index}.o"
        subprocess.run(["cl65", "-t", "c64", "-Os", "-I", str(ROOT / "client/src"),
                        "-DS64_REU_BUFFER=1", "-DS64_NATIVE_PETSCII=1", "-DS64_INDEPENDENT_EDITOR=1",
                        '-DS64_STORAGE_ROOT_DIRECTORY="' + selected_root + '"',
                        "-c", "-o", str(obj), str(ROOT / "client" / source)], check=True)
        objects.append(str(obj))
    prg = out / "T8PROBE.PRG"
    subprocess.run(["cl65", "-t", "c64", "-m", str(out / "probe.map"), "-o", str(prg), *objects], check=True)
    subprocess.run(["python3", str(ROOT / "tools/check_memory_headroom.py"), str(out / "probe.map"),
                    "--label", "8tml-storage-probe", "--stack-base", "0xC000", "--min-headroom", "1024"], check=True)
    return prg.read_bytes()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", required=True)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--reference-tools", type=Path, help="Optional external specs/tools directory")
    parser.add_argument("--storage-root", type=storage_root, default="/sd/")
    parser.add_argument("--package-dir", type=Path, default=ROOT / "client/build/independent-package/SEE64YOU")
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    package = args.package_dir.resolve()
    volume = args.storage_root.rstrip("/")
    folder = args.storage_root + "SEE64YOU"
    assert {p.name for p in package.iterdir() if p.is_file()} == FILES
    for script, manifest in (("generate_module_manifest.py", "S64MODULE.MNF"),
                             ("generate_code_asset_manifest.py", "S64OVL.MNF")):
        extra = ["--root", str(package / "S64EDIT.PRG")] if "code_asset" in script else []
        subprocess.run(["python3", str(ROOT / "tools" / script), str(package), *extra,
                        "--validate", str(package / manifest)], check=True)
    candidate = {p.name: p.read_bytes() for p in package.iterdir() if p.is_file()}
    assert native(folder + "/") + b"\0" in candidate["SEE64YOU.PRG"], \
        "Launcher storage root differs from requested test volume"
    candidate["T8PROBE.PRG"] = build_probe(out, args.storage_root)
    state = {"commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
             "active": False, "restored": False, "tests": [], "input": "REST keyboard-buffer injection",
             "physical_keyboard": False, "cold_power_cycle": False,
             "storage_root": args.storage_root, "folder": folder, "created_folder": False,
             "test_source_sha256": {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
                 for name in ("client/diagnostics/8tml_storage_probe.c", "tools/test_8tml_c64u.py")},
             "package_sha256": {n: hashlib.sha256(b).hexdigest() for n, b in candidate.items()}}

    def save():
        (out / "state.json").write_text(json.dumps(state, indent=2))

    def mark(test):
        state["tests"].append(test)
        save()
        print("PASS " + test, flush=True)

    def api(path, method="GET", **query):
        url = "http://" + args.host + "/v1/" + path
        if query:
            url += "?" + urllib.parse.urlencode(query)
        with urllib.request.urlopen(urllib.request.Request(url, method=method), timeout=12) as reply:
            return reply.read()

    def mem(address, length):
        return api("machine:readmem", address=f"{address:04X}", length=length)

    def screen(label):
        data = mem(0x400, 1000)
        text = "\n".join("".join(chr((v & 127) + 64 if (v & 127) < 32 else v & 127)
                                 for v in data[i:i + 40]) for i in range(0, 1000, 40))
        (out / (label + ".txt")).write_text(text)
        return text

    def keys(data):
        for start in range(0, len(data), 8):
            deadline = time.monotonic() + 10
            while mem(0xc6, 1) != b"\0":
                if time.monotonic() > deadline:
                    raise RuntimeError("Keyboard buffer busy")
                time.sleep(.05)
            chunk = data[start:start + 8]
            api("machine:writemem", "PUT", address="0277", data=chunk.hex())
            api("machine:writemem", "PUT", address="00C6", data=bytes([len(chunk)]).hex())
            time.sleep(.25)

    def wait(expected, label, timeout=70):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            text = screen(label)
            if expected.upper() in text.upper() and mem(0xc6, 1) == b"\0":
                return text
            if "8TML FAIL" in text:
                raise AssertionError(text)
            time.sleep(.25)
        raise AssertionError(f"{label}: missing {expected!r}; {text}")

    def basic(label):
        wait("READY.", label)
        state["active"] = False
        save()
        keys(b"\x93PRINT 6*7\r")
        assert re.search(r"(?:^|\n)\s*42\s*(?:\n|$)", wait("READY.", label + "-number"))
        keys(b'\x93PRINT"OK"\r')
        assert re.search(r"(?:^|\n)OK\s*(?:\n|$)", wait("READY.", label + "-string"))

    def live(address):
        assert not state["active"]
        keys(f"POKE827,PEEK({address})\r".encode("ascii"))
        wait("READY.", "live-register")
        return mem(827, 1)[0]

    def setlive(address, value):
        assert not state["active"]
        keys(f"POKE{address},{value}\r".encode("ascii"))
        wait("READY.", "set-live")

    save()
    assert "READY." in screen("before") and mem(0xc6, 1) == b"\0", "Need idle BASIC"
    assert mem(int.from_bytes(mem(0x2b, 2), "little"), 2) == b"\0\0", "Existing BASIC program"
    scratch = mem(827, 1)
    basic("baseline")
    prior = live(53297)
    colours = [live(a) for a in (53280, 53281, 646)]
    irq = mem(0x314, 2)
    drives = api("drives")
    configs = api("configs")
    (out / "info-before.json").write_bytes(api("info"))
    (out / "drives-before.json").write_bytes(drives)
    (out / "configs-before.json").write_bytes(configs)
    state.update(turbo=prior, colours=colours, irq=irq.hex())
    ftp = ftplib.FTP(args.host, timeout=20)
    ftp.login()

    def fetch(path):
        buffer = io.BytesIO()
        ftp.retrbinary("RETR " + path, buffer.write)
        return buffer.getvalue()

    def put(path, data):
        ftp.storbinary("STOR " + path, io.BytesIO(data))
        actual = fetch(path)
        if actual != data:
            (out / "write-mismatch-expected.bin").write_bytes(data)
            (out / "write-mismatch-actual.bin").write_bytes(actual)
            state["write_mismatch"] = {"path": path, "expected_bytes": len(data),
                                       "actual_bytes": len(actual),
                                       "expected_sha256": hashlib.sha256(data).hexdigest(),
                                       "actual_sha256": hashlib.sha256(actual).hexdigest()}
            save()
            raise AssertionError("Storage write/readback mismatch: " + path)

    def inventory(directory):
        files = {}
        for item in ftp.nlst(directory):
            name = item.rstrip("/").rsplit("/", 1)[-1]
            if name in (".", ".."):
                continue
            path = directory + "/" + name
            try:
                ftp.cwd(path)
            except ftplib.error_perm:
                files[path] = fetch(path)
            else:
                files.update(inventory(path))
        return files

    root_names = sorted(ftp.nlst(volume))
    assert not any(p.rstrip("/").rsplit("/", 1)[-1].upper() in
                   ("S64JRN0.DAT", "S64JRN1.DAT", "SEE64YOU.CFG") for p in root_names), "Legacy root state blocks isolation"
    folder_existed = any(path.rstrip("/").rsplit("/", 1)[-1].upper() == "SEE64YOU" for path in root_names)
    originals = inventory(folder) if folder_existed else {}
    state["folder_existed"] = folder_existed
    protected_sd = None
    protected_sd_roots = None
    protected_sd_folder_existed = False
    if args.storage_root.lower() != "/sd/":
        volumes = ftp.nlst("/")
        if any(path.rstrip("/").rsplit("/", 1)[-1].lower() == "sd" for path in volumes):
            protected_sd_roots = sorted(ftp.nlst("/sd"))
            protected_sd_folder_existed = any(
                path.rstrip("/").rsplit("/", 1)[-1].upper() == "SEE64YOU" for path in protected_sd_roots)
            if protected_sd_folder_existed:
                protected_sd = inventory("/sd/SEE64YOU")
            else:
                protected_sd = {}
            state["protected_sd_sha256"] = {path: hashlib.sha256(data).hexdigest()
                                             for path, data in protected_sd.items()}
    reserved = set(candidate) | {"T8EDIT.8tml", "T8EDIT2.8tml", "T8EMPTY.8tml", "T8IN.8tml",
                "T8BAD.8tml", "T8COPY.8tml", "T8COPY.8tml.tmp", "T8RECOVER.8tml",
                "x" * 28 + ".8tml"}
    fixture_names = {n.upper() for n in reserved - FILES}
    fixture_names.update(n + ".TMP" for n in tuple(fixture_names))
    assert not any(path.rsplit("/", 1)[-1].upper() in fixture_names
                   for path in originals), "Disposable fixture names already exist"
    state["originals"] = {}
    for index, (path, data) in enumerate(originals.items()):
        backup = f"original-{index}.bin"
        (out / backup).write_bytes(data)
        assert (out / backup).read_bytes() == data
        state["originals"][path] = {"backup": backup, "sha256": hashlib.sha256(data).hexdigest()}
    mark("complete-app-folder-backup")

    def launch(name, prompt):
        basic("before-" + name)
        entry = re.search(b"\x9e([0-9]+)\x00", candidate[name][2:34])
        assert entry, "Missing BASIC SYS entry"
        api("runners:load_prg", "PUT", file=folder + "/" + name)
        time.sleep(.6)
        setlive(53297, prior)
        state["active"] = True
        save()
        keys(b"SYS" + entry.group(1) + b"\r")
        wait(prompt, "start-" + name)

    def exitcheck(label):
        basic(label)
        assert live(53297) == prior and mem(0x314, 2) == irq and api("drives") == drives
        ticks = mem(0xa0, 3)
        time.sleep(.2)
        assert mem(0xa0, 3) != ticks
        mark(label + "-BASIC-IRQ-drives-clock-turbo")

    def save_exit(basename):
        keys(b"\x03")
        wait("S SAVE  D DISCARD", basename + "-save-choice")
        keys(b"S")
        wait("Save as:", basename + "-save-prompt")
        keys(native(basename) + b"\r")
        exitcheck(basename + "-exit")

    try:
        state["staged"] = True
        save()
        if not folder_existed:
            assert sorted(ftp.nlst(volume)) == root_names, "Target volume changed before staging"
            ftp.mkd(folder)
            state["created_folder"] = True
            save()
        for name, data in candidate.items():
            put(folder + "/" + name, data)
        for path in originals:
            if path.rsplit("/", 1)[-1].upper() in ("SEE64YOU.REC", "SEE64YOU.REC.TMP"):
                ftp.delete(path)
        mark("candidate-package-readback")
        launch("T8PROBE.PRG", "C CAPTURE T TEST R RESTORE")
        keys(b"C")
        wait("DOS CONTEXT CAPTURED", "dos-context-captured")
        exitcheck("context-capture-exit")
        context_before = mem(0xc000, 1024)
        (out / "dos-context-before.bin").write_bytes(context_before)
        state["dos_context_captured"] = True
        save()
        launch("SEE64YOU.PRG", "FILE EDIT SEARCH FORMAT")
        assert "PUB" not in screen("editor-header")[:40], "Independent Editor still exposes PUB"
        keys(b"\x8c")  # Native F8.
        wait("PRESS ANY KEY/CLICK/FIRE TO RETURN", "editor-help")
        assert "PUBLISHER" not in screen("editor-help-commands")
        keys(b"\r")
        wait("FILE EDIT SEARCH FORMAT", "editor-help-dismissed")
        assert all((value & 127) == 32 for value in mem(0x400 + 6 * 40, 18 * 40)), \
            "Help text remains below the short document"
        mark("shipping-editor-help-dismissal-clears-unused-rows-no-PUB")
        keys(b"\x03")
        wait("Exit to BASIC? Y/N", "clean-exit-confirmation")
        assert "FILE EDIT SEARCH FORMAT" in screen("clean-exit-still-editor")
        keys(b"N")
        wait("FILE EDIT SEARCH FORMAT", "clean-exit-no")
        keys(b"\x03")
        wait("Exit to BASIC? Y/N", "clean-exit-confirmation-again")
        keys(b"\x03")
        wait("FILE EDIT SEARCH FORMAT", "clean-exit-stop-cancel")
        keys(b"\x03")
        wait("Exit to BASIC? Y/N", "clean-exit-confirmation-yes")
        keys(b"Y")
        exitcheck("clean-confirmed-exit")
        mark("shipping-editor-clean-exit-requires-yes-N-and-STOP-cancel")
        launch("SEE64YOU.PRG", "FILE EDIT SEARCH FORMAT")
        keys(native("Hardware 8TML.") + b"\r")
        # Longer than the retired 30-second autosave interval: editing/idle
        # must not create a background recovery file.
        deadline = time.monotonic() + 35
        while time.monotonic() < deadline:
            assert not any(path.rsplit("/", 1)[-1].upper() == "SEE64YOU.REC"
                           for path in ftp.nlst(folder)), "Unexpected background autosave"
            time.sleep(1)
        mark("shipping-editor-no-background-autosave-after-35-seconds")
        save_exit("T8EDIT")
        saved = fetch(folder + "/T8EDIT.8tml")
        assert saved == EMPTY + b"Hardware 8TML.\n", repr(saved)
        (out / "editor-created.8tml").write_bytes(saved)
        mark("shipping-editor-create-body-save-basename-exact-source")
        put(folder + "/SEE64YOU.REC", saved)
        launch("SEE64YOU.PRG", "Recover snapshot? Y/N")
        keys(b"Y")
        wait("Recovered snapshot", "editor-recovered")
        keys(b"\x11" * 4 + native("Updated "))
        save_exit("T8EDIT2")
        resaved = fetch(folder + "/T8EDIT2.8tml")
        assert resaved == EMPTY + b"Updated Hardware 8TML.\n", repr(resaved)
        (out / "editor-recovered-edited.8tml").write_bytes(resaved)
        mark("shipping-editor-recovery-reopen-edit-save-exact-source")
        put(folder + "/T8IN.8tml", SOURCE)
        put(folder + "/T8BAD.8tml", EMPTY + b"bad\x80input")
        launch("T8PROBE.PRG", "C CAPTURE T TEST R RESTORE")
        keys(b"T")
        wait("8TML STORAGE TESTS PASSED", "native-storage-tests")
        exitcheck("native-storage-probe-exit")
        expected = SOURCE.replace(b"\r\n", b"\n") + b"Hardware edit.\n!"
        assert fetch(folder + "/T8COPY.8tml.tmp") == expected
        assert fetch(folder + "/T8RECOVER.8tml") == expected
        assert fetch(folder + "/T8EMPTY.8tml") == EMPTY
        assert fetch(folder + "/" + "x" * 28 + ".8tml") == EMPTY
        assert fetch(folder + "/T8IN.8tml") == SOURCE
        assert fetch(folder + "/T8BAD.8tml") == EMPTY + b"bad\x80input"
        (out / "native-storage-recovered.8tml").write_bytes(expected)
        mark("native-real-DOS-open-edit-save-metadata-CRLF-empty-filename-limit-injected-fault-recovery")
        if args.reference_tools:
            command = ["python3", str(args.reference_tools.resolve() / "8tml"), "check", "-v",
                       str(out / "editor-created.8tml"), str(out / "editor-recovered-edited.8tml"),
                       str(out / "native-storage-recovered.8tml")]
            result = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT)
            (out / "validator.txt").write_text(result.stdout)
            assert result.returncode == 0, result.stdout
            mark("external-reference-validator-zero-errors")
    except BaseException as error:
        state["error"] = repr(error)
        save()
        raise
    finally:
        if state["active"]:
            try:
                if "READY." not in screen("failure-screen"):
                    keys(b"\x03")
                    time.sleep(.5)
                    exit_screen = screen("failure-exit")
                    if "S SAVE  D DISCARD" in exit_screen:
                        keys(b"D")
                    elif "EXIT TO BASIC? Y/N" in exit_screen:
                        keys(b"Y")
                basic("failure-basic")
            except Exception as error:
                state["exit_error"] = repr(error)
                save()
        if not state["active"]:
            try:
                if state.get("dos_context_captured"):
                    launch("T8PROBE.PRG", "C CAPTURE T TEST R RESTORE")
                    for offset in range(0, len(context_before), 128):
                        api("machine:writemem", "PUT", address=f"{0xc000 + offset:04X}",
                            data=context_before[offset:offset + 128].hex())
                    assert mem(0xc000, 1024) == context_before
                    keys(b"R")
                    wait("DOS CONTEXT RESTORED", "dos-context-restored")
                    exitcheck("context-restore-exit")
                    state["dos_context_restored"] = True
                    save()
                known_temporary = {name.upper() for name in reserved}
                known_temporary.update((name + ".tmp").upper() for name in reserved)
                known_temporary.update(("SEE64YOU.REC", "SEE64YOU.REC.TMP", "S64JRN0.DAT", "S64JRN1.DAT"))
                current = inventory(folder) if folder_existed or state["created_folder"] else {}
                assert all(path in originals or path.rsplit("/", 1)[-1].upper() in known_temporary
                           for path in current), "Unexpected new file; preserve it for review"
                for path in current:
                    if path not in originals:
                        ftp.delete(path)
                for path, data in originals.items():
                    put(path, data)
                assert (inventory(folder) if folder_existed or state["created_folder"] else {}) == originals
                if state["created_folder"]:
                    assert not state.get("dos_context_captured") or state.get("dos_context_restored"), \
                        "Restore captured DOS contexts before removing test folder"
                    ftp.cwd(volume)
                    ftp.rmd(folder)
                    state["created_folder_removed"] = True
                assert sorted(ftp.nlst(volume)) == root_names
                if protected_sd is not None:
                    assert sorted(ftp.nlst("/sd")) == protected_sd_roots, "Protected SD root changed"
                    current_sd = inventory("/sd/SEE64YOU") if protected_sd_folder_existed else {}
                    assert current_sd == protected_sd, "Protected SD app folder changed"
                    state["protected_sd_unchanged"] = True
                setlive(53297, prior)
                for address, value in zip((53280, 53281, 646), colours):
                    setlive(address, value)
                assert live(53297) == prior
                assert [live(a) for a in (53280, 53281, 646)] == colours
                assert mem(0x314, 2) == irq and api("drives") == drives and api("configs") == configs
                basic("restored-basic")
                api("machine:writemem", "PUT", address="033B", data=scratch.hex())
                keys(b"NEW\r")
                wait("READY.", "restored-final")
                assert mem(int.from_bytes(mem(0x2b, 2), "little"), 2) == b"\0\0"
                state["restored"] = True
                mark("original-files-config-machine-baseline-restored")
            except Exception as error:
                state["restore_error"] = repr(error)
                save()
        else:
            state["restoration_blocked"] = "Program active; backups retained, no reset or original-file overwrite."
            save()
        ftp.quit()
    assert state["restored"]


if __name__ == "__main__":
    main()
