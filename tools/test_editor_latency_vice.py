#!/usr/bin/env python3
"""Measure direct production Editor operations in VICE using emulated jiffies.

This excludes the shipping overlay, redraw and physical keyboard. Zero ticks
means below clock resolution, not zero latency. Warp changes host run duration,
not the emulated clock. ROMs remain external and are never copied into reports.
"""
import argparse
from contextlib import suppress
import hashlib
import json
from pathlib import Path
import socket
import struct
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "client/build/editor-audit/latency-vice"
sys.path.insert(0, str(ROOT / "tools/vice"))
from test_reu_probe import wait_for_monitor

NAMES = ("insert_midpoint", "backspace_midpoint", "move_right_midpoint",
         "end_of_line_midpoint", "goto_last_line", "copy_all",
         "paste_64_midpoint", "undo_insert_midpoint")
SOURCE = r'''
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "editor.h"
#include "clipboard.h"
#include "platform/reu.h"
#define MAIL ((volatile unsigned char*)0xc040)
static S64Editor editor;
static S64Clipboard clipboard;
static unsigned char chunk[64];
static const unsigned int sizes[3] = {1024u,16384u,60000u};
static unsigned int n, lines, operation, size, offset, count, i, before;
static unsigned char index;
static int ok;
static clock_t started, elapsed;
static void word(unsigned char at, unsigned int value)
{ MAIL[at] = (unsigned char)value; MAIL[at+1] = (unsigned char)(value>>8); }
static void longword(unsigned char at, unsigned long value)
{ word(at,(unsigned int)value); word(at+2,(unsigned int)(value>>16)); }
static void failure(unsigned char code)
{ MAIL[1]=index; MAIL[2]=code; MAIL[0]=0x5a; for (;;) {} }
static void fixture(void)
{
    s64_editor_init(&editor);
    s64_clipboard_init(&clipboard);
    for (offset=0; offset<size; offset+=count) {
        count=size-offset; if(count>64u) count=64u;
        for(i=0;i<count;++i)
            chunk[i]=lines && (offset+i)%40u==39u ? '\n' : 'a';
        if(!s64_reu_store(0,(uint16_t)offset,chunk,(uint16_t)count)) failure(1);
    }
    editor.buffer.gap_start=size;
    editor.buffer.gap_end=S64_BUFFER_CAPACITY;
    editor.buffer.cursor=size/2u;
    s64_editor_document_loaded(&editor);
    s64_editor_recalculate_position(&editor);
    s64_editor_ensure_cursor_visible(&editor);
}
int main(void)
{
    MAIL[0]=1; index=0;
    for(n=0;n<3u;++n) for(lines=0;lines<2u;++lines)
    for(operation=0;operation<8u;++operation) {
        size=sizes[n]; fixture();
        if(operation==5u) s64_editor_select_all(&editor);
        if(operation==6u) {
            memset(chunk,'p',64u);
            if(!s64_reu_store(1,0,chunk,64u)) failure(2);
            clipboard.length=64u;
        }
        if(operation==7u) {
            if(!s64_editor_insert(&editor,'x')) failure(3);
            s64_editor_mark_saved(&editor);
        }
        before=s64_buffer_length(&editor.buffer);
        ok=1; started=clock();
        switch(operation) {
            case 0: ok=s64_editor_insert(&editor,'x'); break;
            case 1: ok=s64_editor_backspace(&editor); break;
            case 2: s64_editor_right(&editor); break;
            case 3: s64_editor_end(&editor); break;
            case 4: ok=s64_editor_go_to_line(&editor,lines ? size/40u+1u : 1u); break;
            case 5: ok=s64_editor_copy(&editor,&clipboard); break;
            case 6: ok=s64_editor_paste(&editor,&clipboard); break;
            case 7: ok=s64_editor_undo(&editor); break;
        }
        elapsed=clock()-started;
        if(!ok) failure(4);
        if(operation==0u && s64_buffer_length(&editor.buffer)!=before+1u) failure(5);
        if((operation==1u || operation==7u) && s64_buffer_length(&editor.buffer)!=before-1u) failure(6);
        if(operation==5u && clipboard.length!=size) failure(7);
        if(operation==6u && s64_buffer_length(&editor.buffer)!=before+64u) failure(8);
        MAIL[1]=index; word(2,size); longword(4,(unsigned long)elapsed);
        longword(8,(unsigned long)CLOCKS_PER_SEC);
        MAIL[12]=(unsigned char)lines; MAIL[13]=(unsigned char)operation;
        word(14,s64_buffer_length(&editor.buffer));
        MAIL[0]=0xa5;
        while(MAIL[0]==0xa5) {}
        ++index; MAIL[0]=1;
    }
    MAIL[0]=0xff;
    for(;;) {}
    return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=BUILD / "latency.json")
    parser.add_argument("--case-timeout", type=float, default=180)
    args = parser.parse_args()
    if args.case_timeout <= 0:
        parser.error("--case-timeout must be positive")
    BUILD.mkdir(parents=True, exist_ok=True)
    roms = []
    for name in ("kernal", "basic", "chargen"):
        path = args.rom_dir.resolve() / (name + ".rom")
        if not path.is_file():
            raise FileNotFoundError(path)
        roms += ["-" + name, str(path)]
    source = BUILD / "latency.c"
    source.write_text(SOURCE)
    sources = [source, ROOT / "client/src/buffer_reu.c", ROOT / "client/src/editor.c",
               ROOT / "client/src/clipboard.c", ROOT / "client/src/history_reu.c",
               ROOT / "client/src/platform/reu.c", ROOT / "client/src/platform/reu_dma.s"]
    objects = []
    source_hashes = {}
    for item in sources:
        # cl65 may create an intermediate .s next to its input even with -o.
        # Private copies prevent collisions with a concurrent shipping build.
        private = BUILD / "sources" / item.relative_to(ROOT)
        private.parent.mkdir(parents=True, exist_ok=True)
        content = item.read_bytes()
        private.write_bytes(content)
        if item != source:
            source_hashes[item.relative_to(ROOT).as_posix()] = hashlib.sha256(content).hexdigest()
        obj = BUILD / (item.stem + ".o")
        subprocess.run(["cl65", "-t", "c64", "-Os", "-DS64_REU_BUFFER=1",
                        "-Iclient/src", "-I" + str(item.parent), "-c", "-o", str(obj), str(private)],
                       cwd=ROOT, check=True, timeout=60)
        objects.append(str(obj))
    prg = BUILD / "LATENCY.PRG"
    subprocess.run(["cl65", "-t", "c64", "-Wl", "-D,__STACKSIZE__=0x0400",
                    "-m", str(BUILD / "latency.map"), "-o", str(prg), *objects],
                   cwd=ROOT, check=True, timeout=60)
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        port = sock.getsockname()[1]
    command = ["xvfb-run", "-a", "x64sc", "+sound", "-warp", "-reu", "-reusize", "16384",
               "-autostartprgmode", "1", "-binarymonitor",
               "-binarymonitoraddress", f"ip4://127.0.0.1:{port}", *roms,
               "-autostart", str(prg)]
    rows = []
    monitor = None
    with (BUILD / "vice.log").open("w") as log:
        process = subprocess.Popen(command, stdout=log, stderr=log)
        try:
            monitor = wait_for_monitor("127.0.0.1", port, 20)
            for index in range(48):
                deadline = time.monotonic() + args.case_timeout
                while True:
                    data = monitor.read_memory(0xc040, 0xc04f)
                    if data[0] == 0x5a:
                        raise AssertionError(f"Diagnostic failure case={data[1]} code={data[2]}")
                    if data[0] == 0xa5:
                        break
                    if process.poll() is not None or time.monotonic() >= deadline:
                        raise TimeoutError(f"VICE case {index}: mailbox {data.hex()}; see {BUILD / 'vice.log'}")
                    monitor.resume()
                    time.sleep(.05)
                assert data[1] == index, (index, data.hex())
                size, ticks, rate = struct.unpack_from("<HII", data, 2)
                assert rate > 0 and data[12] in (0, 1) and data[13] < len(NAMES)
                row = dict(size_bytes=size,
                           fixture="lines_40_columns" if data[12] else "single_long_line",
                           operation=NAMES[data[13]], ticks=ticks, clocks_per_second=rate,
                           emulated_seconds=ticks / rate,
                           below_clock_resolution=ticks == 0)
                rows.append(row)
                print(json.dumps(row), flush=True)
                monitor.write_memory(0xc040, b"\x00")
                monitor.resume()
        finally:
            if monitor is not None:
                with suppress(Exception):
                    monitor.quit()
                with suppress(Exception):
                    monitor.close()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5)
    report = {"measurement": "VICE C64 emulated clock() jiffies; zero means below resolution",
              "scope": "production direct Editor functions; excludes shipping overlay/UI and physical input",
              "fixture": "midpoint cursor, gap at EOF, word wrap on; setup excluded",
              "program_sha256": hashlib.sha256(prg.read_bytes()).hexdigest(),
              "production_source_sha256": source_hashes,
              "vice_mode": "16 MiB REU, warp enabled, default C64 machine timing",
              "compiler": subprocess.check_output(["cl65", "--version"], stderr=subprocess.STDOUT, text=True).strip(),
              "rows": rows}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print("Report:", args.output)


if __name__ == "__main__":
    main()
