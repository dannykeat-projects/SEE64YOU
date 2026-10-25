#!/usr/bin/env python3
"""Measure production Editor REU I/O scaling on the host, not C64 latency.

This is the host-counter fallback for a future VICE cycle diagnostic. It builds
the actual editor/buffer/history/clipboard code with a simulated two-bank REU.
Counters exclude fixture preparation, UI, overlays, disk and network. Host CPU
time is reported separately and is not a proxy for C64 responsiveness.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "client/build/editor-audit"
HARNESS = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "editor.h"
#include "clipboard.h"
#include "platform/reu.h"

static unsigned char memory[2][65536];
static unsigned long loads, stores, loaded, stored;
static S64Editor editor;
static S64Clipboard clipboard;

uint8_t s64_reu_probe(void) { return 1; }
uint16_t s64_reu_probe_banks(void) { return 2; }
uint8_t s64_reu_load(uint8_t bank, uint16_t address, void* target, uint16_t length)
{
    assert(bank < 2 && (unsigned long)address + length <= 65536ul);
    ++loads; loaded += length;
    memcpy(target, memory[bank] + address, length);
    return 1;
}
uint8_t s64_reu_store(uint8_t bank, uint16_t address, const void* source, uint16_t length)
{
    assert(bank < 2 && (unsigned long)address + length <= 65536ul);
    ++stores; stored += length;
    memcpy(memory[bank] + address, source, length);
    return 1;
}

static void fixture(unsigned int size, unsigned int lines)
{
    unsigned int i;
    memset(memory, 0, sizeof(memory));
    s64_editor_init(&editor);
    s64_clipboard_init(&clipboard);
    for (i = 0; i < size; ++i)
        memory[0][i] = lines && i % 40u == 39u ? '\n' : 'a';
    editor.buffer.gap_start = size;
    editor.buffer.gap_end = S64_BUFFER_CAPACITY;
    editor.buffer.cursor = size / 2u;
    s64_editor_document_loaded(&editor);
    s64_editor_recalculate_position(&editor);
    s64_editor_ensure_cursor_visible(&editor);
    /* Every measurement begins at the midpoint with normal position caches. */
    assert(s64_buffer_cursor(&editor.buffer) == size / 2u);
}

int main(void)
{
    static const unsigned int sizes[] = {1024u, 16384u, 60000u};
    static const char* names[] = {
        "insert_midpoint", "backspace_midpoint", "move_right_midpoint",
        "end_of_line_midpoint", "goto_last_line", "copy_all",
        "paste_64_midpoint", "undo_insert_midpoint"
    };
    unsigned int n, lines, operation;
    for (n = 0; n < 3u; ++n) for (lines = 0; lines < 2u; ++lines)
    for (operation = 0; operation < 8u; ++operation) {
        unsigned int size = sizes[n];
        unsigned int before;
        clock_t started;
        double seconds;
        int ok = 1;
        fixture(size, lines);
        if (operation == 5u) s64_editor_select_all(&editor);
        if (operation == 6u) {
            memset(memory[1], 'p', 64u);
            clipboard.length = 64u;
        }
        if (operation == 7u) {
            assert(s64_editor_insert(&editor, 'x'));
            s64_editor_mark_saved(&editor);
        }
        before = s64_buffer_length(&editor.buffer);
        loads = stores = loaded = stored = 0;
        started = clock();
        switch (operation) {
            case 0: ok = s64_editor_insert(&editor, 'x'); break;
            case 1: ok = s64_editor_backspace(&editor); break;
            case 2: s64_editor_right(&editor); break;
            case 3: s64_editor_end(&editor); break;
            case 4: ok = s64_editor_go_to_line(&editor, lines ? size / 40u + 1u : 1u); break;
            case 5: ok = s64_editor_copy(&editor, &clipboard); break;
            case 6: ok = s64_editor_paste(&editor, &clipboard); break;
            case 7: ok = s64_editor_undo(&editor); break;
        }
        seconds = (double)(clock() - started) / CLOCKS_PER_SEC;
        assert(ok);
        /* Validate each operation independently of its I/O counters. */
        if (operation == 0u) assert(s64_buffer_length(&editor.buffer) == before + 1u);
        if (operation == 1u || operation == 7u)
            assert(s64_buffer_length(&editor.buffer) == before - 1u);
        if (operation == 5u) assert(clipboard.length == size);
        if (operation == 6u) assert(s64_buffer_length(&editor.buffer) == before + 64u);
        printf("%u,%s,%s,%lu,%lu,%lu,%lu,%.9f\n", size,
            lines ? "lines_40_columns" : "single_long_line", names[operation],
            loads, stores, loaded, stored, seconds);
    }
    return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default="cc", help="Host C compiler")
    parser.add_argument("--output", type=Path, default=BUILD / "editor-io-scaling.json")
    args = parser.parse_args()
    BUILD.mkdir(parents=True, exist_ok=True)
    source = BUILD / "editor-io-scaling.c"
    binary = BUILD / "editor-io-scaling"
    source.write_text(HARNESS)
    production_sources = ["client/src/buffer_reu.c", "client/src/clipboard.c",
                          "client/src/history_reu.c", "client/src/editor.c"]
    source_hashes = {}
    private_sources = []
    for name in production_sources:
        content = (ROOT / name).read_bytes()
        source_hashes[name] = hashlib.sha256(content).hexdigest()
        private = BUILD / "host-sources" / name
        private.parent.mkdir(parents=True, exist_ok=True)
        private.write_bytes(content)
        private_sources.append(str(private))
    subprocess.run([args.cc, "-std=c99", "-Wall", "-Wextra", "-Werror",
                    "-DS64_REU_BUFFER=1", "-Iclient/src", str(source),
                    *private_sources, "-o", str(binary)],
                   cwd=ROOT, check=True)
    result = subprocess.run([str(binary)], cwd=ROOT, text=True, capture_output=True,
                            check=True, timeout=120)
    rows = []
    for line in result.stdout.splitlines():
        size, fixture, operation, loads, stores, loaded, stored, seconds = line.split(",")
        rows.append(dict(size_bytes=int(size), fixture=fixture, operation=operation,
                         reu_load_calls=int(loads), reu_store_calls=int(stores),
                         reu_loaded_bytes=int(loaded), reu_stored_bytes=int(stored),
                         host_cpu_seconds=float(seconds)))
    assert len(rows) == 48
    report = {"measurement": "host REU I/O counters; not latency or C64 cycles",
              "scope": "production direct Editor functions; excludes shipping overlay/UI",
              "fixture": "midpoint cursor, gap at EOF, word wrap on; setup and position caches excluded",
              "host_abi": "metadata sizes follow the host ABI and differ from cc65",
              "timing": "host process CPU time for one operation; resolution CLOCKS_PER_SEC; not C64 latency",
              "production_source_sha256": source_hashes,
              "rows": rows}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(report["measurement"])
    print("bytes fixture operation load_calls store_calls loaded_bytes stored_bytes host_cpu_seconds")
    for row in rows:
        print(" ".join(str(value) for value in row.values()))
    print("Report:", args.output)


if __name__ == "__main__":
    main()
