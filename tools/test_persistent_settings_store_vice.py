#!/usr/bin/env python3
"""Execute the real persistent Settings storage overlay under VICE."""
from __future__ import annotations

import argparse
import subprocess
import sys
import time
from pathlib import Path

sys.dont_write_bytecode = True
import build_editor_overlay_fit as fit

ROOT = Path(__file__).resolve().parents[1]
CLIENT = ROOT / "client"
OUT = CLIENT / "build" / "persistent-settings-store-vice"

GROUP = (
    "src/publisher/settings_store.c",
    "src/publisher/settings_exchange.c",
    "src/publisher/settings_validate.c",
    "src/publisher/destination.c",
    "src/publisher/persistent_settings_control.c",
)

MAIN = r"""
#include <stdint.h>
#include <string.h>

#include "platform/code_overlay.h"
#include "platform/dos.h"
#include "platform/reu.h"
#include "platform/reu_runtime.h"
#include "publisher/settings_store.h"
#include "publisher/persistent_settings_control.h"

extern const uint8_t sstore_image[], sstore_image_end[];

static S64ReuRuntimeControl control;
static S64ReuAllocation asset;
static uint8_t stored[512];
static uint8_t temporary[512];
static uint16_t stored_length;
static uint16_t temporary_length;
static const uint8_t* read_source;
static uint16_t read_length;
static uint16_t read_offset;
static uint8_t read_open;
static uint8_t write_open;
static S64Dos dos;
static S64SettingsExchange settings;
static S64SettingsExchange loaded;
static uint8_t asset_slot;

static uint8_t overlay_reader(
    uint8_t bank, uint16_t offset, void* target, uint16_t length)
{
    return s64_reu_load(bank, offset, target, length);
}

static void require(uint8_t passed, uint8_t step)
{
    *(volatile uint8_t*)0xc060 = step;
    if (!passed) {
        *(volatile uint8_t*)0xc062 = step;
        *(volatile uint8_t*)0xc061 = 0x5au;
        for (;;) {}
    }
}

static S64UciResult mock_execute(
    const S64UciCommandPart* parts,
    uint8_t part_count,
    S64UciReply* reply)
{
    if (!parts || part_count != 2u
        || parts[0].length != 3u
        || parts[0].data[0] != S64_DOS_TARGET_PRIMARY
        || parts[0].data[1] != 0x02u
        || parts[0].data[2] != 0x06u) {
        return S64_UCI_PROTOCOL_ERROR;
    }
    temporary_length = 0u;
    write_open = 1u;
    if (reply && reply->status && reply->status_capacity >= 3u) {
        reply->status[0] = '0';
        reply->status[1] = '0';
        reply->status[2] = 0;
        reply->status_length = 2u;
    }
    return S64_UCI_OK;
}

S64DosResult s64_dos_change_dir(S64Dos* dos, const char* directory)
{
    (void)dos;
    return directory ? S64_DOS_OK : S64_DOS_INVALID_ARGUMENT;
}

S64DosResult s64_dos_file_exists(
    S64Dos* dos, const char* filename, uint8_t* exists)
{
    (void)dos;
    if (!filename || !exists) return S64_DOS_INVALID_ARGUMENT;
    *exists = strcmp(filename, S64_SETTINGS_FILENAME) == 0
        ? (stored_length != 0u) : 0u;
    return S64_DOS_OK;
}

S64DosResult s64_dos_open_read(S64Dos* dos, const char* filename)
{
    (void)dos;
    if (!filename || strcmp(filename, S64_SETTINGS_FILENAME) != 0
        || !stored_length) return S64_DOS_NOT_FOUND;
    read_source = stored;
    read_length = stored_length;
    read_offset = 0u;
    read_open = 1u;
    return S64_DOS_OK;
}

S64DosResult s64_dos_read(
    S64Dos* dos, uint16_t requested, S64UciDataSink sink,
    void* context, uint16_t* bytes_read)
{
    uint16_t remaining;
    uint16_t count;
    (void)dos;
    if (bytes_read) *bytes_read = 0u;
    if (!read_open || !sink) return S64_DOS_NO_FILE_OPEN;
    remaining = (uint16_t)(read_length - read_offset);
    count = remaining < requested ? remaining : requested;
    if (count && !sink(read_source + read_offset, count, context))
        return S64_DOS_CONSUMER_ABORTED;
    read_offset = (uint16_t)(read_offset + count);
    if (bytes_read) *bytes_read = count;
    return S64_DOS_OK;
}

S64DosResult s64_dos_write(
    S64Dos* dos, const uint8_t* data, uint16_t length)
{
    (void)dos;
    if (!write_open || !data
        || (uint32_t)temporary_length + length > sizeof(temporary))
        return S64_DOS_TARGET_ERROR;
    memcpy(temporary + temporary_length, data, length);
    temporary_length = (uint16_t)(temporary_length + length);
    return S64_DOS_OK;
}

S64DosResult s64_dos_close(S64Dos* dos)
{
    (void)dos;
    read_open = 0u;
    write_open = 0u;
    return S64_DOS_OK;
}

S64DosResult s64_dos_delete_file(S64Dos* dos, const char* filename)
{
    (void)dos;
    if (!filename) return S64_DOS_INVALID_ARGUMENT;
    if (!strcmp(filename, S64_SETTINGS_FILENAME)) stored_length = 0u;
    if (!strcmp(filename, "SEE64YOU.CFG.TMP")) temporary_length = 0u;
    return S64_DOS_OK;
}

S64DosResult s64_dos_rename_file(
    S64Dos* dos, const char* filename, const char* new_name)
{
    (void)dos;
    if (!filename || !new_name
        || strcmp(filename, "SEE64YOU.CFG.TMP") != 0
        || strcmp(new_name, S64_SETTINGS_FILENAME) != 0
        || !temporary_length) {
        return S64_DOS_INVALID_ARGUMENT;
    }
    memcpy(stored, temporary, temporary_length);
    stored_length = temporary_length;
    temporary_length = 0u;
    return S64_DOS_OK;
}

int main(void)
{
    static const char key[] =
        "0123456789abcdef0123456789abcdef"
        "0123456789abcdef0123456789abcdef";
    *(volatile uint8_t*)0xc061 = 0u;
    require(s64_reu_probe() && s64_reu_probe_banks() == 256u, 1u);

    s64_reu_runtime_init(&control, 0x0202u, 0x8899aabbu);
    require(s64_reu_runtime_allocate(
        &control, S64_REU_OWNER_MODULE,
        (uint16_t)(sstore_image_end - sstore_image),
        256u, &asset), 2u);
    asset_slot = (uint8_t)(control.allocation_count - 1u);
    asset.flags = 0x74u;
    control.allocations[asset_slot].flags = asset.flags;
    require(s64_reu_store(
        (uint8_t)(asset.address >> 16),
        (uint16_t)asset.address,
        sstore_image,
        (uint16_t)(sstore_image_end - sstore_image)), 3u);
    require(s64_code_overlay_configure(
        &control, &asset, 1u,
        (void*)S64_CODE_OVERLAY_ADDRESS,
        S64_CODE_OVERLAY_CAPACITY, overlay_reader), 4u);

    memset(&dos, 0, sizeof(dos));
    dos.execute = mock_execute;
    dos.target = S64_DOS_TARGET_PRIMARY;

    s64_settings_exchange_init(&settings);
    settings.destination_type = (uint8_t)S64_DESTINATION_STANDALONE;
    settings.allow_insecure_http = 1u;
    strcpy(settings.base_url, "http://example.test");
    strcpy(settings.direct_key, key);

    strcpy(settings.base_url, "http://example.test/");
    require(s64_persistent_settings_validate_values(&settings)
            == S64_SETTINGS_CONTROL_OK
        && !strcmp(settings.base_url, "http://example.test"), 5u);

    settings.destination_type = (uint8_t)S64_DESTINATION_STANDALONE;
    strcpy(settings.base_url, "not a url");
    require(s64_persistent_settings_validate_values(&settings)
            == S64_SETTINGS_CONTROL_INVALID_DESTINATION, 6u);
    strcpy(settings.base_url, "http://example.test");

    require(s64_settings_store_save(&dos, &settings)
        == S64_SETTINGS_STORE_OK, 9u);
    require(stored_length == sizeof(settings), 8u);

    memset(&loaded, 0, sizeof(loaded));
    require(s64_settings_store_load(&dos, &loaded)
        == S64_SETTINGS_STORE_OK, 7u);
    require(loaded.destination_type == settings.destination_type
        && loaded.allow_insecure_http == 1u
        && !strcmp(loaded.base_url, "http://example.test")
        && !strcmp(loaded.direct_key, key)
        && s64_settings_exchange_validate(&loaded), 10u);

    settings.state = S64_SETTINGS_STATE_READY;
    settings.result = S64_SETTINGS_RESULT_OK;
    require(s64_settings_exchange_store(&settings), 11u);

    strcpy(settings.base_url, "http://changed.test");
    settings.state = S64_SETTINGS_STATE_EDIT_REQUEST;
    settings.checksum = 0u;
    require(s64_persistent_settings_restore_original(&settings)
            == S64_SETTINGS_COMMIT_OK
        && !strcmp(settings.base_url, "http://example.test")
        && settings.state == S64_SETTINGS_STATE_READY, 12u);

    strcpy(settings.base_url, "http://changed.test");
    settings.state = S64_SETTINGS_STATE_EDIT_REQUEST;
    settings.checksum = 0u;
    require(s64_persistent_settings_commit(&dos, &settings)
            == S64_SETTINGS_COMMIT_OK
        && settings.state == S64_SETTINGS_STATE_READY, 13u);

    memset(&loaded, 0, sizeof(loaded));
    require(s64_settings_store_load(&dos, &loaded)
            == S64_SETTINGS_STORE_OK
        && !strcmp(loaded.base_url, "http://changed.test"), 14u);

    *(volatile uint8_t*)0xc060 = 0x80u;
    *(volatile uint8_t*)0xc061 = 0xa5u;
    *(volatile uint8_t*)0xc062 = 0u;
    for (;;) {}
}
"""

def config_text() -> str:
    return """FEATURES { STARTADDRESS: default = $0801; }
SYMBOLS {
    __LOADADDR__: type = import;
    __EXEHDR__: type = import;
    __STACKSIZE__: type = weak, value = $0400;
    __HIMEM__: type = weak, value = $D000;
}
MEMORY {
    ZP: file = "", define = yes, start = $0002, size = $001A;
    LOADADDR: file = %O, start = %S - 2, size = $0002;
    HEADER: file = %O, define = yes, start = %S, size = $000D;
    MAIN: file = %O, define = yes, start = __HEADER_LAST__, size = __HIMEM__ - __HEADER_LAST__;
    BSS: file = "", start = __ONCE_RUN__, size = $8C00 - __ONCE_RUN__;
    SSTORE: file = "build/persistent-settings-store-vice/sstore.bin", start = $8C00, size = $3C00;
}
SEGMENTS {
    ZEROPAGE: load = ZP, type = zp;
    LOADADDR: load = LOADADDR, type = ro;
    EXEHDR: load = HEADER, type = ro;
    STARTUP: load = MAIN, type = ro;
    LOWCODE: load = MAIN, type = ro, optional = yes;
    CODE: load = MAIN, type = ro;
    RODATA: load = MAIN, type = ro;
    DATA: load = MAIN, type = rw;
    INIT: load = MAIN, type = rw;
    ONCE: load = MAIN, type = ro, define = yes;
    BSS: load = BSS, type = bss, define = yes;
    SSTORE_CODE: load = SSTORE, type = ro, define = yes;
    SSTORE_RODATA: load = SSTORE, type = ro, define = yes;
}
FEATURES {
    CONDES: type = constructor, label = __CONSTRUCTOR_TABLE__, count = __CONSTRUCTOR_COUNT__, segment = ONCE;
    CONDES: type = destructor, label = __DESTRUCTOR_TABLE__, count = __DESTRUCTOR_COUNT__, segment = RODATA;
    CONDES: type = interruptor, label = __INTERRUPTOR_TABLE__, count = __INTERRUPTOR_COUNT__, segment = RODATA, import = __CALLIRQ__;
}
"""

def build() -> Path:
    OUT.mkdir(parents=True, exist_ok=True)
    generated = OUT / "generated"
    objects_dir = OUT / "objects"
    generated.mkdir(parents=True, exist_ok=True)
    objects_dir.mkdir(parents=True, exist_ok=True)

    cfg = OUT / "probe.cfg"
    cfg.write_text(config_text(), encoding="ascii", newline="\n")
    flags = [
        "-t", "c64", "-Os", "-I", "src", "-C", str(cfg),
        "-DS64_NATIVE_PETSCII=1",
        "-DS64_REU_BUFFER=1",
        "-DS64_PERSISTENT_RUNTIME_FIT=1",
        "-DS64_SETTINGS_CONFIG_ONLY=1",
    ]

    names: list[str] = []
    sources: list[Path] = []
    for source_path in GROUP:
        names.extend(fit.public_s64_definitions(
            (CLIENT / source_path).read_text(encoding="utf-8")))
    names = [
        name for name in dict.fromkeys(names)
        if name not in {
            "s64_destination_connection_url",
            "s64_destination_list_url",
            "s64_destination_open_url",
            "s64_destination_save_url",
            "s64_destination_delete_url",
        }
    ]
    for source_path in GROUP:
        out = generated / f"sstore_{Path(source_path).stem}.c"
        out.write_text(fit.generated_translation_unit(
            "SSTORE", source_path, names, generated, CLIENT, bridges={}))
        sources.append(out)

    main = generated / "main.c"
    main.write_text(MAIN)
    sources.append(main)
    sources.extend([
        CLIENT / "src/platform/code_overlay.c",
        CLIENT / "src/platform/reu.c",
        CLIENT / "src/platform/reu_dma.s",
        CLIENT / "src/platform/reu_runtime.c",
        CLIENT / "src/platform/reu_runtime_build.c",
        CLIENT / "src/platform/reu_runtime_write.c",
        CLIENT / "src/charset.c",
    ])

    wrappers = generated / "wrappers.s"
    wrappers.write_text(
        fit.assembler_wrappers(
            {"SSTORE": names}, group_ids={"SSTORE": 1}, bridges={}))
    sources.append(wrappers)

    objects: list[Path] = []
    for index, source in enumerate(sources):
        obj = objects_dir / f"{index:03d}_{source.stem}.o"
        fit.run(["cl65", *flags, "-c", "-o", str(obj), str(source)], cwd=CLIENT)
        objects.append(obj)

    blobs = generated / "blobs.s"
    blob_obj = objects_dir / "blobs.o"
    prg = OUT / "persistent-settings-store-vice.prg"
    map_path = OUT / "persistent-settings-store-vice.map"

    for initial in (True, False, False):
        blobs.write_text(
            '.segment "RODATA"\n'
            '.export _sstore_image, _sstore_image_end\n'
            '_sstore_image:\n'
            + ('.byte 0\n' if initial
               else f'.incbin "{OUT.as_posix()}/sstore.bin"\n')
            + '_sstore_image_end:\n'
        )
        fit.run(["cl65", *flags, "-c", "-o", str(blob_obj), str(blobs)], cwd=CLIENT)
        fit.run(
            ["cl65", *flags, "-m", str(map_path), "-o", str(prg),
             *(str(obj) for obj in objects), str(blob_obj)],
            cwd=CLIENT,
        )

    size = (OUT / "sstore.bin").stat().st_size
    if not size or size > fit.OVERLAY_LIMIT:
        raise fit.FitError(f"SSTORE asset size invalid: {size}")
    print(f"SSTORE focused asset: {size} / {fit.OVERLAY_LIMIT}")
    end = fit.bss_end(map_path)
    print(f"Persistent Settings storage VICE probe BSS ends at 0x{end:04X}")
    if end >= fit.RESIDENT_BSS_LIMIT:
        raise fit.FitError("persistent Settings storage VICE probe resident BSS overlaps slot")
    return prg

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-only", action="store_true")
    parser.add_argument("--port", type=int, default=6537)
    parser.add_argument("--vice-bin", default="x64sc")
    parser.add_argument("--rom-dir", type=Path, default=Path("/tmp/see64you-roms"))
    args = parser.parse_args()

    prg = build()
    if args.build_only:
        print(prg)
        return 0

    sys.path.insert(0, str(ROOT / "tools" / "vice"))
    from test_reu_probe import wait_for_monitor

    command = [
        "xvfb-run", "-a", args.vice_bin, "+sound", "-warp",
        "-reu", "-reusize", "16384",
        "-autostartprgmode", "1",
        "-binarymonitor",
        "-binarymonitoraddress", f"ip4://127.0.0.1:{args.port}",
    ]
    for kind in ("kernal", "basic", "chargen"):
        rom = args.rom_dir / f"{kind}.rom"
        if not rom.is_file():
            raise fit.FitError(f"Missing pinned Open ROM: {rom}")
        command += [f"-{kind}", str(rom)]
    command += ["-autostart", str(prg)]

    with (OUT / "vice.log").open("w") as log:
        process = subprocess.Popen(command, stdout=log, stderr=log)
        monitor = None
        try:
            monitor = wait_for_monitor("127.0.0.1", args.port, 90)
            deadline = time.monotonic() + 90
            state = b""
            while time.monotonic() < deadline:
                state = monitor.read_memory(0xc060, 0xc062)
                if state[1] == 0xa5:
                    print(
                        "VICE persistent Settings storage passed: real SSTORE "
                        "transactional save, readback verification and exact load"
                    )
                    return 0
                if state[1] == 0x5a:
                    raise fit.FitError(
                        f"persistent Settings storage VICE probe failed at step {state[2]}")
                monitor.resume()
                time.sleep(0.03)
            raise fit.FitError(
                f"persistent Settings storage VICE probe timed out: {state.hex()}")
        finally:
            if monitor:
                monitor.quit()
                monitor.close()
            if process.poll() is None:
                process.terminate()
            process.wait(timeout=10)

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except fit.FitError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
