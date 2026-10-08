#!/usr/bin/env python3
"""Execute the real staged persistent direct-open path under VICE."""
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
OUT = CLIENT / "build" / "persistent-open-vice"

GROUPS = {
    "PCONTENT": (
        "src/publisher/direct_key.c",
        "src/publisher/direct_key_store.c",
        "diagnostics/persistent_document_read.c",
    ),
    "PNETWORK": (
        "src/platform/network.c",
        "src/crypto/sha256.c",
        "src/publisher/direct_auth.c",
        "src/publisher/standalone_direct.c",
        "diagnostics/persistent_document_stage.c",
    ),
}
GROUP_IDS = {"PCONTENT": 1, "PNETWORK": 2}

PNETWORK_WRITER = r"""
#include <stdint.h>
#include "document.h"
#pragma code-name ("PNETWORK_CODE")
#pragma rodata-name ("PNETWORK_RODATA")
S64DocumentResult s64_overlay_pnetwork_s64_persistent_document_write_wire(
    const S64Document* document, S64DocumentSink sink, void* context)
{
    (void)document;
    (void)sink;
    (void)context;
    return S64_DOCUMENT_INVALID_ARGUMENT;
}
"""

MAIN = r"""
#include <stdint.h>
#include <string.h>

#include "buffer.h"
#include "charset.h"
#include "document.h"
#include "platform/code_overlay.h"
#include "platform/reu.h"
#include "platform/reu_layout.h"
#include "platform/reu_runtime.h"
#include "publisher/standalone_direct.h"

extern const uint8_t pcontent_image[], pcontent_image_end[];
extern const uint8_t pnetwork_image[], pnetwork_image_end[];

static S64ReuRuntimeControl control;
static S64ReuAllocation assets[2];
static S64HttpTransport direct_http;
static S64PersistentOpenStage open_stage;
static S64Buffer body;
static S64Document document;
static uint8_t fake_http_open_count;
static uint8_t opened_body[6];

void s64_buffer_init(S64Buffer* value)
{
    value->gap_start = 0u;
    value->gap_end = S64_BUFFER_CAPACITY;
    value->cursor = 0u;
}

int s64_persistent_document_slug_valid(const char* slug)
{
    return slug && !strcmp(slug, "remote-1");
}

const char* s64_persistent_document_type_name(S64DocumentType type)
{
    return type == S64_DOCUMENT_POST ? "post" : 0;
}

void s64_persistent_document_wire_reader_init(
    S64DocumentWireReader* reader, S64Document* value)
{
    if (!reader) return;
    memset(reader, 0, sizeof(*reader));
    reader->document = value;
    reader->result = S64_DOCUMENT_INVALID_ARGUMENT;
}

uint8_t s64_persistent_document_wire_reader_write(
    const uint8_t* data, uint16_t length, void* context)
{
    (void)data;
    (void)length;
    (void)context;
    return 0u;
}

S64DocumentResult s64_persistent_document_wire_reader_finish(
    S64DocumentWireReader* reader)
{
    (void)reader;
    return S64_DOCUMENT_INVALID_ARGUMENT;
}

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

static uint8_t allocate_asset(
    uint8_t index, const uint8_t* first, const uint8_t* last, uint8_t flags)
{
    uint16_t length = (uint16_t)(last - first);
    uint8_t slot;

    if (!length || length > S64_CODE_OVERLAY_CAPACITY
        || !s64_reu_runtime_allocate(
            &control, S64_REU_OWNER_MODULE, length, 256u, &assets[index])) {
        return 0u;
    }
    slot = (uint8_t)(control.allocation_count - 1u);
    assets[index].flags = flags;
    control.allocations[slot].flags = flags;
    return s64_reu_store(
        (uint8_t)(assets[index].address >> 16),
        (uint16_t)assets[index].address,
        first,
        length
    );
}

static S64UciResult fake_http_uci(
    const S64UciCommandPart* parts, uint8_t part_count, S64UciReply* reply)
{
    static const uint8_t challenge_response[] = {
        0x48u, 0x54u, 0x54u, 0x50u, 0x2fu, 0x31u, 0x2eu, 0x30u, 0x20u, 0x32u, 0x30u, 0x30u,
        0x20u, 0x4fu, 0x4bu, 0x0du, 0x0au, 0x43u, 0x6fu, 0x6eu, 0x74u, 0x65u, 0x6eu, 0x74u,
        0x2du, 0x4cu, 0x65u, 0x6eu, 0x67u, 0x74u, 0x68u, 0x3au, 0x20u, 0x34u, 0x38u, 0x0du,
        0x0au, 0x0du, 0x0au, 0x7bu, 0x22u, 0x63u, 0x68u, 0x61u, 0x6cu, 0x6cu, 0x65u, 0x6eu,
        0x67u, 0x65u, 0x22u, 0x3au, 0x22u, 0x30u, 0x31u, 0x32u, 0x33u, 0x34u, 0x35u, 0x36u,
        0x37u, 0x38u, 0x39u, 0x61u, 0x62u, 0x63u, 0x64u, 0x65u, 0x66u, 0x30u, 0x31u, 0x32u,
        0x33u, 0x34u, 0x35u, 0x36u, 0x37u, 0x38u, 0x39u, 0x61u, 0x62u, 0x63u, 0x64u, 0x65u,
        0x66u, 0x22u, 0x7du
    };
    static const uint8_t document_response[] = {
        0x48u, 0x54u, 0x54u, 0x50u, 0x2fu, 0x31u, 0x2eu, 0x30u, 0x20u, 0x32u, 0x30u, 0x30u,
        0x20u, 0x4fu, 0x4bu, 0x0du, 0x0au, 0x43u, 0x6fu, 0x6eu, 0x74u, 0x65u, 0x6eu, 0x74u,
        0x2du, 0x4cu, 0x65u, 0x6eu, 0x67u, 0x74u, 0x68u, 0x3au, 0x20u, 0x31u, 0x30u, 0x38u,
        0x0du, 0x0au, 0x0du, 0x0au, 0x53u, 0x45u, 0x45u, 0x36u, 0x34u, 0x59u, 0x4fu, 0x55u,
        0x2fu, 0x31u, 0x0au, 0x54u, 0x79u, 0x70u, 0x65u, 0x3au, 0x20u, 0x70u, 0x6fu, 0x73u,
        0x74u, 0x0au, 0x53u, 0x6cu, 0x75u, 0x67u, 0x3au, 0x20u, 0x72u, 0x65u, 0x6du, 0x6fu,
        0x74u, 0x65u, 0x2du, 0x31u, 0x0au, 0x54u, 0x69u, 0x74u, 0x6cu, 0x65u, 0x3au, 0x20u,
        0x52u, 0x65u, 0x6du, 0x6fu, 0x74u, 0x65u, 0x20u, 0x74u, 0x65u, 0x73u, 0x74u, 0x0au,
        0x53u, 0x74u, 0x61u, 0x74u, 0x75u, 0x73u, 0x3au, 0x20u, 0x70u, 0x75u, 0x62u, 0x6cu,
        0x69u, 0x73u, 0x68u, 0x65u, 0x64u, 0x0au, 0x44u, 0x61u, 0x74u, 0x65u, 0x3au, 0x20u,
        0x32u, 0x30u, 0x32u, 0x36u, 0x2du, 0x31u, 0x30u, 0x2du, 0x30u, 0x38u, 0x0au, 0x4fu,
        0x72u, 0x64u, 0x65u, 0x72u, 0x3au, 0x20u, 0x2du, 0x37u, 0x0au, 0x0au, 0x48u, 0x65u,
        0x6cu, 0x6cu, 0x6fu, 0x0au
    };
    const uint8_t* response;
    uint16_t response_length;
    uint16_t sent;
    uint8_t count[2];
    uint8_t handle = 0x31u;
    uint8_t command;

    if (!parts || !part_count || !reply || !parts[0].data
        || parts[0].length < 2u || parts[0].data[0] != 0x03u
        || !reply->status || reply->status_capacity < 3u) {
        return S64_UCI_PROTOCOL_ERROR;
    }

    command = parts[0].data[1];
    reply->status[0] = '0';
    reply->status[1] = '0';
    reply->status[2] = 0;
    reply->status_length = 2u;

    if (command == 0x07u) {
        ++fake_http_open_count;
        if (!reply->data_sink
            || !reply->data_sink(&handle, 1u, reply->data_context)) {
            return S64_UCI_CONSUMER_ABORTED;
        }
        return S64_UCI_OK;
    }

    if (command == 0x11u) {
        if (part_count != 2u || !reply->data_sink) {
            return S64_UCI_PROTOCOL_ERROR;
        }
        sent = parts[1].length;
        count[0] = (uint8_t)sent;
        count[1] = (uint8_t)(sent >> 8);
        if (!reply->data_sink(count, 2u, reply->data_context)) {
            return S64_UCI_CONSUMER_ABORTED;
        }
        return S64_UCI_OK;
    }

    if (command == 0x10u) {
        if (!reply->data_sink) return S64_UCI_PROTOCOL_ERROR;
        if (fake_http_open_count & 1u) {
            response = challenge_response;
            response_length = (uint16_t)sizeof(challenge_response);
        } else {
            response = document_response;
            response_length = (uint16_t)sizeof(document_response);
        }
        count[0] = (uint8_t)response_length;
        count[1] = (uint8_t)(response_length >> 8);
        if (!reply->data_sink(count, 2u, reply->data_context)
            || !reply->data_sink(response, response_length, reply->data_context)) {
            return S64_UCI_CONSUMER_ABORTED;
        }
        return S64_UCI_OK;
    }

    if (command == 0x09u) return S64_UCI_OK;
    return S64_UCI_PROTOCOL_ERROR;
}

int main(void)
{
    static const char key_hex[] =
        "000102030405060708090a0b0c0d0e0f"
        "101112131415161718191a1b1c1d1e1f";
    static const uint8_t expected_body[] = {
        0x48u, 0x65u, 0x6cu, 0x6cu, 0x6fu, 0x0au
    };
    uint8_t ascii;
    uint8_t i;
    S64StandaloneResult result;
    S64DocumentResult document_result;

    *(volatile uint8_t*)0xc061 = 0u;
    require(s64_reu_probe() && s64_reu_probe_banks() == 256u, 1u);

    s64_reu_runtime_init(&control, 0x0202u, 0x44556677UL);
    require(allocate_asset(0u, pcontent_image, pcontent_image_end, 0x61u), 2u);
    require(allocate_asset(1u, pnetwork_image, pnetwork_image_end, 0x62u), 3u);
    require(s64_code_overlay_configure(
        &control, assets, 2u, (void*)S64_CODE_OVERLAY_ADDRESS,
        S64_CODE_OVERLAY_CAPACITY, overlay_reader), 4u);

    document.body = &body;
    s64_buffer_init(&body);

    require(s64_direct_publisher_key(key_hex), 5u);
    fake_http_open_count = 0u;
    direct_http.execute = fake_http_uci;
    result = s64_persistent_standalone_open_stage(
        &direct_http,
        "http://example.test/api.php?action=get&type=post&slug=remote-1&format=s64",
        1u,
        &open_stage
    );
    require(result == S64_STANDALONE_OK, 6u);
    require(fake_http_open_count == 2u
        && open_stage.body_started
        && open_stage.body_length == 6u
        && open_stage.header_length != 0u, 7u);

    document_result = s64_persistent_open_stage_commit(
        &open_stage, &document);
    require(document_result == S64_DOCUMENT_OK,
        (uint8_t)(80u + document_result));
    require(document.type == S64_DOCUMENT_POST
        && document.status == S64_DOCUMENT_PUBLISHED
        && document.order == -7
        && !strcmp(document.slug, "remote-1")
        && !strcmp(document.title, "Remote test")
        && !strcmp(document.date, "2026-10-08"), 9u);
    require(body.gap_start == 6u
        && body.gap_end == S64_BUFFER_CAPACITY
        && body.cursor == 0u, 10u);
    require(s64_reu_load(
        S64_REU_DOCUMENT_BANK,
        S64_REU_DOCUMENT_BASE,
        opened_body,
        (uint16_t)sizeof(opened_body)), 11u);
    for (i = 0u; i < (uint8_t)sizeof(expected_body); ++i) {
        require(s64_charset_native_to_ascii(opened_body[i], &ascii)
            && ascii == expected_body[i], 12u);
    }

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
    PCONTENT: file = "build/persistent-open-vice/pcontent.bin", start = $8C00, size = $3C00;
    PNETWORK: file = "build/persistent-open-vice/pnetwork.bin", start = $8C00, size = $3C00;
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
    PCONTENT_CODE: load = PCONTENT, type = ro, define = yes;
    PCONTENT_RODATA: load = PCONTENT, type = ro, define = yes;
    PNETWORK_CODE: load = PNETWORK, type = ro, define = yes;
    PNETWORK_RODATA: load = PNETWORK, type = ro, define = yes;
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
    ]

    grouped_names: dict[str, list[str]] = {"PCONTENT": [], "PNETWORK": []}
    sources: list[Path] = []

    for group, paths in GROUPS.items():
        names: list[str] = []
        if group == "PNETWORK":
            names.append("s64_persistent_document_write_wire")
        for path in paths:
            names.extend(fit.public_s64_definitions(
                (CLIENT / path).read_text(encoding="utf-8")
            ))
        if group == "PNETWORK":
            names = [name for name in names
                     if name != "s64_standalone_result_message"]
        grouped_names[group] = list(dict.fromkeys(names))
        for path in paths:
            generated_path = generated / f"{group.lower()}_{Path(path).stem}.c"
            generated_path.write_text(
                fit.generated_translation_unit(
                    group, path, grouped_names[group], generated, CLIENT, bridges={}
                )
            )
            sources.append(generated_path)

    writer = generated / "pnetwork_writer.c"
    writer.write_text(PNETWORK_WRITER)
    sources.append(writer)

    main = generated / "main.c"
    main.write_text(MAIN)
    sources.append(main)

    resident = [
        CLIENT / "src/platform/code_overlay.c",
        CLIENT / "src/platform/reu.c",
        CLIENT / "src/platform/reu_dma.s",
        CLIENT / "src/platform/reu_runtime.c",
        CLIENT / "src/platform/reu_runtime_build.c",
        CLIENT / "src/platform/reu_runtime_write.c",
        CLIENT / "src/charset.c",
    ]
    sources.extend(resident)

    wrappers = generated / "wrappers.s"
    wrappers.write_text(
        fit.assembler_wrappers(grouped_names, group_ids=GROUP_IDS, bridges={})
    )
    sources.append(wrappers)

    objects: list[Path] = []
    for index, source in enumerate(sources):
        obj = objects_dir / f"{index:03d}_{source.stem}.o"
        fit.run(["cl65", *flags, "-c", "-o", str(obj), str(source)], cwd=CLIENT)
        objects.append(obj)

    blobs = generated / "blobs.s"
    blob_obj = objects_dir / "blobs.o"
    prg = OUT / "persistent-open-vice.prg"
    map_path = OUT / "persistent-open-vice.map"

    for initial in (True, False, False):
        lines = ['.segment "RODATA"']
        for group in ("pcontent", "pnetwork"):
            lines += [
                f".export _{group}_image, _{group}_image_end",
                f"_{group}_image:",
                ".byte 0" if initial else f'.incbin "{OUT.as_posix()}/{group}.bin"',
                f"_{group}_image_end:",
            ]
        blobs.write_text("\n".join(lines) + "\n")
        fit.run(["cl65", *flags, "-c", "-o", str(blob_obj), str(blobs)], cwd=CLIENT)
        fit.run(
            ["cl65", *flags, "-m", str(map_path), "-o", str(prg),
             *(str(obj) for obj in objects), str(blob_obj)],
            cwd=CLIENT,
        )

    for group in ("pcontent", "pnetwork"):
        size = (OUT / f"{group}.bin").stat().st_size
        if not size or size > fit.OVERLAY_LIMIT:
            raise fit.FitError(f"{group} asset size invalid: {size}")
        print(f"{group.upper()} staged-open asset: {size} / {fit.OVERLAY_LIMIT}")

    end = fit.bss_end(map_path)
    print(f"Persistent open VICE probe BSS ends at 0x{end:04X}")
    if end >= fit.RESIDENT_BSS_LIMIT:
        raise fit.FitError("persistent open VICE probe resident BSS overlaps slot")
    return prg


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-only", action="store_true")
    parser.add_argument("--port", type=int, default=6534)
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
                        "VICE staged persistent open passed: authenticated "
                        "PNETWORK receive, REU staging, PCONTENT metadata/body "
                        "commit, exact PETSCII document"
                    )
                    return 0
                if state[1] == 0x5a:
                    raise fit.FitError(
                        f"persistent open VICE probe failed at step {state[2]}"
                    )
                monitor.resume()
                time.sleep(0.03)
            raise fit.FitError(f"persistent open VICE probe timed out: {state.hex()}")
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
