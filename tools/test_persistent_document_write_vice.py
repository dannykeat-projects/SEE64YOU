#!/usr/bin/env python3
"""Execute the real persistent Publisher document writer under VICE."""
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
OUT = CLIENT / "build" / "persistent-document-write-vice"

EXPECTED = (
    b"SEE64YOU/1\n"
    b"Type: post\n"
    b"Slug: entry-1\n"
    b"Title: Persistent test\n"
    b"Status: draft\n"
    b"Date: \n"
    b"Order: 7\n\n"
)


def c_bytes(data: bytes) -> str:
    return ", ".join(f"0x{value:02x}u" for value in data)


def source_text() -> str:
    return f"""
#include <stdint.h>
#include <string.h>

#include "buffer.h"
#include "document.h"
#include "publisher/persistent_services.h"

static uint8_t output[256];
static uint16_t output_length;

void s64_buffer_init(S64Buffer* value)
{{
    value->gap_start = 0u;
    value->gap_end = S64_BUFFER_CAPACITY;
    value->cursor = 0u;
}}

unsigned int s64_buffer_length(const S64Buffer* value)
{{
    return (unsigned int)(S64_BUFFER_CAPACITY
        - (value->gap_end - value->gap_start));
}}

unsigned char s64_buffer_char_at(const S64Buffer* value, unsigned int index)
{{
    (void)value;
    (void)index;
    return 0u;
}}

int s64_persistent_document_slug_valid(const char* slug)
{{
    return slug && !strcmp(slug, "entry-1");
}}

const char* s64_persistent_document_type_name(S64DocumentType type)
{{
    return type == S64_DOCUMENT_POST ? "post" : 0;
}}

static uint8_t capture(const uint8_t* data, uint16_t length, void* context)
{{
    uint16_t i;
    (void)context;
    if ((uint16_t)(output_length + length) > (uint16_t)sizeof(output))
        return 0u;
    for (i = 0u; i < length; ++i)
        output[output_length++] = data[i];
    return 1u;
}}

static void require(uint8_t passed, uint8_t step)
{{
    *(volatile uint8_t*)0xc060 = step;
    if (!passed) {{
        *(volatile uint8_t*)0xc062 = step;
        *(volatile uint8_t*)0xc061 = 0x5au;
        for (;;) {{}}
    }}
}}

int main(void)
{{
    static const uint8_t expected[] = {{ {c_bytes(EXPECTED)} }};
    S64Buffer body;
    S64Document document;

    *(volatile uint8_t*)0xc061 = 0u;
    memset(&body, 0, sizeof(body));
    memset(&document, 0, sizeof(document));
    s64_buffer_init(&body);
    document.body = &body;
    document.type = S64_DOCUMENT_POST;
    strcpy(document.slug, "entry-1");
    strcpy(document.title, "Persistent test");
    document.status = S64_DOCUMENT_DRAFT;
    document.date[0] = 0;
    document.order = 7;

    output_length = 0u;
    require(
        s64_persistent_document_write_wire(&document, capture, 0)
            == S64_DOCUMENT_OK,
        1u
    );
    require(output_length == (uint16_t)sizeof(expected), 2u);
    require(!memcmp(output, expected, sizeof(expected)), 3u);

    *(volatile uint8_t*)0xc060 = 0x80u;
    *(volatile uint8_t*)0xc061 = 0xa5u;
    *(volatile uint8_t*)0xc062 = 0u;
    for (;;) {{}}
}}
"""


def build() -> Path:
    OUT.mkdir(parents=True, exist_ok=True)
    main_c = OUT / "main.c"
    prg = OUT / "persistent-document-write-vice.prg"
    main_c.write_text(source_text(), encoding="utf-8", newline="\n")
    fit.run(
        [
            "cl65", "-t", "c64", "-Os", "-I", "src",
            "-DS64_NATIVE_PETSCII=1",
            "-DS64_REU_BUFFER=1",
            "-DS64_PERSISTENT_RUNTIME_FIT=1",
            "-o", str(prg),
            str(main_c),
            "diagnostics/persistent_document_write.c",
            "src/charset.c",
        ],
        cwd=CLIENT,
    )
    return prg


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-only", action="store_true")
    parser.add_argument("--port", type=int, default=6533)
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
            monitor = wait_for_monitor("127.0.0.1", args.port, 60)
            deadline = time.monotonic() + 60
            state = b""
            while time.monotonic() < deadline:
                state = monitor.read_memory(0xc060, 0xc062)
                if state[1] == 0xa5:
                    print("VICE persistent document writer passed: native metadata emitted as exact ASCII wire bytes")
                    return 0
                if state[1] == 0x5a:
                    raise fit.FitError(
                        f"persistent document writer VICE probe failed at step {state[2]}"
                    )
                monitor.resume()
                time.sleep(0.03)
            raise fit.FitError(
                f"persistent document writer VICE probe timed out: {state.hex()}"
            )
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
