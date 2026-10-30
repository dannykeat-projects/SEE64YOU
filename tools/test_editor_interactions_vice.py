#!/usr/bin/env python3
"""VICE diagnostic of production REU Editor, UI, menu and clipboard functions.

Uses a small diagnostic driver, not the shipping launcher/overlay lifecycle.
Large fixtures are written through REU setup, never by monitor-editing state.
Prompt input uses the KERNAL keyboard buffer; physical keyboard is untested.
"""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from test_independent_programs_vice import screen, key
from test_independent_programs_vice import wait_for_monitor

HARNESS = r'''
#include <conio.h>
#include <stdint.h>
#include <string.h>
#include "editor.h"
#include "ui.h"
#include "menu.h"
#include "platform/reu.h"
#define STAGE (*(volatile unsigned char*)0xc040)
#define ACK (*(volatile unsigned char*)0xc041)
#define FAIL (*(volatile unsigned char*)0xc042)
#define VISUAL (*(volatile unsigned char*)0xc043)
static S64Editor editor;
static S64Clipboard clipboard;
static unsigned char chunk[64];
static unsigned char checked[64];
static unsigned int length;
static void require(unsigned char ok, unsigned char detail) {
    if (!ok) { FAIL=detail; for (;;) {} }
}
static void checkpoint(unsigned char stage) {
    STAGE=stage; while (ACK!=stage) {} VISUAL=0;
}
static unsigned char fixture_byte(unsigned int offset) {
    return offset%40u==39u ? '\n' : (unsigned char)(0x41u+offset%26u);
}
static void exact(unsigned char detail) {
    unsigned int at, i, count;
    require(s64_buffer_length(&editor.buffer)==length,detail);
    for (at=0;at<length;at+=count) {
        count=length-at; if (count>64u) count=64u;
        require(s64_buffer_read(&editor.buffer,at,checked,count)==count,detail);
        for (i=0;i<count;++i) require(checked[i]==fixture_byte(at+i),detail);
    }
}
static void fixture(unsigned int size) {
    unsigned int at,i,count;
    s64_editor_init(&editor); s64_clipboard_init(&clipboard);
    length=size;
    for (at=0;at<size;at+=count) {
        count=size-at; if (count>64u) count=64u;
        for (i=0;i<count;++i) chunk[i]=fixture_byte(at+i);
        require(s64_reu_store(0u,at,chunk,count),1);
    }
    editor.buffer.gap_start=size;
    editor.buffer.gap_end=S64_BUFFER_CAPACITY;
    editor.buffer.cursor=0u;
    s64_editor_document_loaded(&editor);
    s64_ui_init(); s64_ui_render(&editor);
    exact(2);
}
static void blank_tail(unsigned char first) {
    unsigned int at;
    for(at=(unsigned int)first*40u;at<960u;++at)
        if ((*(volatile unsigned char*)(0x0400u+at)&127u)!=32u) VISUAL=1;
}
int main(void) {
    static const unsigned int sizes[]={64u,1024u,16384u,60000u,61440u};
    unsigned char n,i,group;
    char prompt[64];
    STAGE=0; ACK=0; FAIL=0; VISUAL=0;
    require(s64_reu_probe(),4);
    for(n=0;n<sizeof(sizes)/sizeof(sizes[0]);++n) {
        fixture(sizes[n]);
        for(i=0;i<3u;++i) {
            s64_ui_show_help(); s64_ui_render(&editor); exact(5);
            if(length==64u) blank_tail(3u);
        }
        checkpoint(10u+n);
        for(group=0;group<S64_MENU_GROUPS;++group) {
            s64_menu_render(group,0u);
            s64_ui_render(&editor); exact(6);
            if(length==64u) blank_tail(3u);
        }
        checkpoint(20u+n);
        s64_editor_select_all(&editor);
        require(s64_editor_copy(&editor,&clipboard),7);
        require(clipboard.length==length,8); exact(9);
        require(s64_editor_cut(&editor,&clipboard),10);
        require(s64_buffer_length(&editor.buffer)==0u,11);
        s64_ui_render(&editor); blank_tail(2u);
        checkpoint(30u+n);
        s64_ui_show_help(); s64_ui_render(&editor); blank_tail(2u);
        require(s64_editor_paste(&editor,&clipboard),12); exact(13);
        s64_ui_render(&editor);
        checkpoint(40u+n);
        /* Replace a whole selection with the same-size clipboard at capacity. */
        s64_editor_select_all(&editor);
        require(s64_editor_paste(&editor,&clipboard),14); exact(15);
        s64_ui_render(&editor);
        checkpoint(50u+n);
    }
    fixture(64u);
    checkpoint(60u);
    require(!s64_ui_prompt_text("FIND:",prompt,sizeof(prompt)),16);
    require(prompt[0]==0,17);
    s64_ui_render(&editor); exact(18); blank_tail(3u);
    checkpoint(61u);
    require(!s64_ui_prompt_yes_no("DISCARD CHANGES? Y/N"),19);
    s64_ui_render(&editor); exact(20); blank_tail(3u);
    checkpoint(62u);
    require(s64_ui_prompt_text("FIND:",prompt,sizeof(prompt)),21);
    require(strlen(prompt)==50u,22);
    s64_ui_render(&editor); exact(23); blank_tail(3u);
    checkpoint(63u);
    STAGE=0xa5u;
    for(;;) {}
    return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom-dir", type=Path, required=True)
    parser.add_argument("--port", type=int, default=6595)
    parser.add_argument("--sizes", type=int, nargs="+", default=[64,1024,16384,60000,61440])
    args = parser.parse_args()
    if not 1 <= len(args.sizes) <= 5 or any(size < 1 or size > 61440 for size in args.sizes):
        parser.error("Choose one to five fixture sizes between 1 and 61440 bytes")
    out = ROOT / "client/build/editor-interactions-vice"
    out.mkdir(parents=True, exist_ok=True)
    source, prg = out / "interactions.c", out / "interactions.prg"
    source.write_text(HARNESS.replace("{64u,1024u,16384u,60000u,61440u}",
                                     "{"+",".join(f"{size}u" for size in args.sizes)+"}"))
    names = ("ui.c", "ui_status.s", "menu.c", "editor.c", "editor_format.c",
             "buffer_reu.c", "history_reu.c", "clipboard.c", "platform/reu.c",
             "platform/reu_dma.s")
    source_hashes = {name: hashlib.sha256((ROOT / "client/src" / name).read_bytes()).hexdigest()
                     for name in names}
    copied_sources = []
    for name in names:
        copy = out / "sources" / name
        copy.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / "client/src" / name, copy)
        copied_sources.append(str(copy))
    subprocess.run(["cl65", "-t", "c64", "-Os", "-DS64_REU_BUFFER=1",
                    "-DS64_NATIVE_PETSCII=1", "-DS64_INDEPENDENT_EDITOR=1",
                    "-I", str(ROOT / "client/src"),
                    "-I", str(ROOT / "client/src/platform"),
                    "-m", str(out / "interactions.map"),
                    "-o", str(prg), str(source),
                    *copied_sources], check=True)
    command = ["xvfb-run", "-a", "x64sc", "+sound", "-warp", "-reu", "-reusize", "16384",
               "-autostartprgmode", "1", "-binarymonitor",
               "-binarymonitoraddress", f"ip4://127.0.0.1:{args.port}"]
    for name in ("basic", "kernal", "chargen"):
        command += [f"-{name}", str(args.rom_dir / f"{name}.rom")]
    stages = []
    report_path = out / "report.json"
    def save_report(complete=False):
        report_path.write_text(json.dumps(dict(
            scope=__doc__, complete=complete,
            fixture_sizes=args.sizes,
            source_sha256=source_hashes,
            sha256=hashlib.sha256(prg.read_bytes()).hexdigest(), stages=stages), indent=2)+"\n")
    with (out / "vice.log").open("w") as log:
        process = subprocess.Popen(command + ["-autostart", str(prg)], stdout=log, stderr=log)
        monitor = None
        try:
            monitor = wait_for_monitor("127.0.0.1", args.port, 20)
            deadline = time.monotonic() + 240
            while time.monotonic() < deadline:
                stage, ack, failure, visual = monitor.read_memory(0xc040, 0xc043)
                if failure:
                    raise AssertionError(f"stage {stage}, failure {failure}: {screen(monitor)!r}")
                if stage == 0xa5:
                    break
                if stage and stage != ack:
                    text = screen(monitor)
                    stages.append(dict(stage=stage, blank_tail_failure=bool(visual),
                                       screen_rows=[text[i:i+40] for i in range(0,1000,40)]))
                    save_report()
                    print(f"checkpoint {stage}: blank_tail_failure={bool(visual)}", flush=True)
                    deadline = time.monotonic() + 240
                    monitor.write_memory(0xc041, bytes([stage]))
                    monitor.resume()
                    if stage in (60, 61, 62):
                        time.sleep(0.15)
                        values = b"CANCELLED\x03" if stage == 60 else b"N" if stage == 61 else b"A"*50+b"\r"
                        for value in values:
                            key(monitor, value)
                            time.sleep(0.025)
                else:
                    monitor.resume()
                time.sleep(0.02)
            else:
                raise AssertionError(f"Timeout at stage {stage}, screen={screen(monitor)!r}")
        finally:
            if monitor:
                monitor.quit()
                monitor.close()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
    save_report(complete=True)
    visual_failures = [item["stage"] for item in stages if item["blank_tail_failure"]]
    print(f"{len(stages)} checkpoints: exact REU clipboard/source and prompts passed; "
          f"visual failure stages={visual_failures}; {out}")
    if visual_failures:
        raise AssertionError(f"Stale menu/Help/tail text at stages {visual_failures}; see report.json")


if __name__ == "__main__":
    main()
