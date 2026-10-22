#!/usr/bin/env python3
"""Exercise one persistent lifecycle owner across Editor/Publisher/Settings overlays.

Diagnostic only. It preloads three tiny mode assets into a 16 MiB REU once, then
runs repeated logical mode changes through the production overlay selector. No
production package or PRG-to-PRG transfer path is used.
"""
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
OUT = CLIENT / "build" / "persistent-runtime-probe"

MODEL = r"""
#include <stdint.h>
#pragma code-name ("MODEL_CODE")
#pragma rodata-name ("MODEL_RODATA")
extern void __fastcall__ s64_probe_mode_enter(uint8_t);
uint16_t __fastcall__ s64_overlay_model_s64_persistent_editor(uint16_t value)
{
    s64_probe_mode_enter(1u);
    return (uint16_t)(value ^ 0x1111u);
}
"""

FILE = r"""
#include <stdint.h>
#pragma code-name ("FILE_CODE")
#pragma rodata-name ("FILE_RODATA")
extern void __fastcall__ s64_probe_mode_enter(uint8_t);
uint16_t __fastcall__ s64_overlay_file_s64_persistent_publisher(uint16_t value)
{
    s64_probe_mode_enter(2u);
    return (uint16_t)(value + 0x0222u);
}
"""

RUNTIME = r"""
#include <stdint.h>
#pragma code-name ("RUNTIME_CODE")
#pragma rodata-name ("RUNTIME_RODATA")
extern void __fastcall__ s64_probe_mode_enter(uint8_t);
uint16_t __fastcall__ s64_overlay_runtime_s64_persistent_settings(uint16_t value)
{
    s64_probe_mode_enter(3u);
    return (uint16_t)(value - 0x0033u);
}
"""

MAIN = r"""
#include <stdint.h>
#include <string.h>

#include "platform/code_overlay.h"
#include "platform/reu.h"
#include "platform/reu_runtime.h"
#include "platform/session_journal.h"
#include "platform/session_lifecycle.h"
#include "platform/system_state.h"

extern const uint8_t model_image[], model_image_end[];
extern const uint8_t file_image[], file_image_end[];
extern const uint8_t runtime_image[], runtime_image_end[];
extern uint16_t __fastcall__ s64_persistent_editor(uint16_t);
extern uint16_t __fastcall__ s64_persistent_publisher(uint16_t);
extern uint16_t __fastcall__ s64_persistent_settings(uint16_t);

typedef struct {
    uint16_t transitions;
    uint16_t overlay_reads;
    uint8_t current_mode;
    uint8_t captures;
    uint8_t applies;
    uint8_t restores;
    uint8_t verifies;
    uint8_t active_writes;
    uint8_t clean_writes;
    uint8_t preloads;
} ProbeOwner;

static ProbeOwner owner;
static S64ReuRuntimeControl control;
static S64ReuAllocation caches[3];
static S64ReuAllocation shared_state;
static S64SystemState system_state;
static S64SystemBackend system_backend;
static S64SessionLifecycle lifecycle;
static S64SessionLifecycleStore lifecycle_store;

static void require(uint8_t passed, uint8_t step)
{
    *(volatile uint8_t*)0xc060 = step;
    if (!passed) {
        *(volatile uint8_t*)0xc062 = step;
        *(volatile uint8_t*)0xc061 = 0x5a;
        for (;;) {}
    }
}

static uint8_t backend_capture(void* context, S64SystemSnapshot* snapshot)
{
    ProbeOwner* state = (ProbeOwner*)context;
    ++state->captures;
    snapshot->turbo_control = 2u;
    snapshot->turbo_control_valid = 1u;
    return 1u;
}

static uint8_t backend_apply(void* context, S64SystemMode mode)
{
    ProbeOwner* state = (ProbeOwner*)context;
    (void)mode;
    ++state->applies;
    return 1u;
}

static uint8_t backend_restore(void* context, const S64SystemSnapshot* snapshot)
{
    ProbeOwner* state = (ProbeOwner*)context;
    ++state->restores;
    return snapshot != 0 && snapshot->turbo_control_valid
        && snapshot->turbo_control == 2u;
}

static uint8_t backend_verify(void* context, const S64SystemSnapshot* snapshot)
{
    ProbeOwner* state = (ProbeOwner*)context;
    ++state->verifies;
    return snapshot != 0 && snapshot->turbo_control_valid
        && snapshot->turbo_control == 2u;
}

static S64SessionJournalResult save_journal(void* context, S64SessionJournal* journal)
{
    ProbeOwner* state = (ProbeOwner*)context;
    if (journal == 0) return S64_SESSION_JOURNAL_INVALID;
    if (journal->state == S64_SESSION_STATE_ACTIVE) ++state->active_writes;
    else if (journal->state == S64_SESSION_STATE_CLEAN) ++state->clean_writes;
    else return S64_SESSION_JOURNAL_INVALID;
    return S64_SESSION_JOURNAL_OK;
}

/* Keep this probe independent of DOS-backed journal I/O while exercising the
 * production lifecycle ordering. These are the minimal wire-independent journal
 * semantics that lifecycle_begin/end consume. */
uint8_t s64_session_journal_prepare(
    S64SessionJournal* journal,
    uint32_t session_id,
    uint16_t package_version,
    uint32_t recovery_ref,
    const S64SystemSnapshot* snapshot)
{
    if (journal == 0 || snapshot == 0 || session_id == 0UL
        || package_version == 0u) return 0u;
    memset(journal, 0, sizeof(*journal));
    journal->state = S64_SESSION_STATE_ACTIVE;
    journal->package_version = package_version;
    journal->session_id = session_id;
    journal->sequence = 1UL;
    journal->recovery_ref = recovery_ref;
    if (snapshot->turbo_control_valid) {
        journal->restore_flags = S64_SESSION_JOURNAL_RESTORE_TURBO;
        journal->turbo_control = snapshot->turbo_control;
    }
    return 1u;
}

uint8_t s64_session_journal_mark_clean(S64SessionJournal* journal)
{
    if (journal == 0 || journal->state != S64_SESSION_STATE_ACTIVE) return 0u;
    journal->state = S64_SESSION_STATE_CLEAN;
    ++journal->sequence;
    return journal->sequence != 0UL;
}

uint8_t s64_session_journal_restore_snapshot(
    const S64SessionJournal* journal,
    S64SystemSnapshot* snapshot)
{
    if (journal == 0 || snapshot == 0) return 0u;
    memset(snapshot, 0, sizeof(*snapshot));
    if (journal->restore_flags & S64_SESSION_JOURNAL_RESTORE_TURBO) {
        snapshot->turbo_control_valid = 1u;
        snapshot->turbo_control = journal->turbo_control;
    }
    return 1u;
}

static uint8_t overlay_reader(
    uint8_t bank, uint16_t offset, void* target, uint16_t length)
{
    ++owner.overlay_reads;
    return s64_reu_load(bank, offset, target, length);
}

static uint8_t shared_load(uint32_t* value)
{
    return s64_reu_load(
        (uint8_t)(shared_state.address >> 16),
        (uint16_t)shared_state.address,
        value,
        (uint16_t)sizeof(*value));
}

static uint8_t shared_store(uint32_t value)
{
    return s64_reu_store(
        (uint8_t)(shared_state.address >> 16),
        (uint16_t)shared_state.address,
        &value,
        (uint16_t)sizeof(value));
}

void __fastcall__ s64_probe_mode_enter(uint8_t mode)
{
    uint32_t value;
    require(mode >= 1u && mode <= 3u, 40u);
    require(lifecycle.active_record_written && !lifecycle.clean_record_written, 41u);
    require(shared_load(&value), 42u);
    value = value * 33UL + mode;
    require(shared_store(value), 43u);
    ++owner.transitions;
    owner.current_mode = mode;
}

static uint8_t preload_asset(
    uint8_t index, const uint8_t* first, const uint8_t* last)
{
    uint16_t length = (uint16_t)(last - first);
    if (length == 0u || length > S64_CODE_OVERLAY_CAPACITY
        || !s64_reu_runtime_allocate(
            &control, S64_REU_OWNER_MODULE, length, 256u, &caches[index])) {
        return 0u;
    }
    if (!s64_reu_store(
            (uint8_t)(caches[index].address >> 16),
            (uint16_t)caches[index].address,
            first, length)) {
        return 0u;
    }
    ++owner.preloads;
    return 1u;
}

static uint32_t expected_step(uint32_t value, uint8_t mode)
{
    return value * 33UL + mode;
}

int main(void)
{
    uint8_t cycle;
    uint32_t expected = 0x10203040UL;
    uint32_t actual;

    *(volatile uint8_t*)0xc061 = 0u;
    memset(&owner, 0, sizeof(owner));

    require(s64_reu_probe() && s64_reu_probe_banks() == 256u, 1u);

    memset(&system_backend, 0, sizeof(system_backend));
    system_backend.capture = backend_capture;
    system_backend.apply_mode = backend_apply;
    system_backend.restore = backend_restore;
    system_backend.verify = backend_verify;
    s64_system_state_init(&system_state, &system_backend, &owner);

    lifecycle_store.save = save_journal;
    lifecycle_store.context = &owner;
    s64_session_lifecycle_init(&lifecycle, &system_state, &lifecycle_store);
    require(s64_session_lifecycle_begin(
        &lifecycle, 0x11223344UL, 0x0202u, 0UL,
        S64_SYSTEM_MODE_INTERACTIVE) == S64_SESSION_LIFECYCLE_OK, 2u);
    require(owner.captures == 1u && owner.active_writes == 1u
        && owner.clean_writes == 0u, 3u);

    s64_reu_runtime_init(&control, 0x0202u, 0x11223344UL);
    require(preload_asset(0u, model_image, model_image_end), 4u);
    require(preload_asset(1u, file_image, file_image_end), 5u);
    require(preload_asset(2u, runtime_image, runtime_image_end), 6u);
    require(owner.preloads == 3u, 7u);
    require(s64_reu_runtime_allocate(
        &control, S64_REU_OWNER_EDITOR, (uint32_t)sizeof(expected),
        256u, &shared_state), 8u);
    require(shared_store(expected), 9u);

    require(s64_code_overlay_configure(
        &control, caches, 3u, (void*)S64_CODE_OVERLAY_ADDRESS,
        S64_CODE_OVERLAY_CAPACITY, overlay_reader), 10u);

    for (cycle = 1u; cycle <= 30u; ++cycle) {
        require(s64_persistent_editor(cycle)
            == (uint16_t)(cycle ^ 0x1111u), 11u);
        expected = expected_step(expected, 1u);

        require(s64_persistent_publisher(cycle)
            == (uint16_t)(cycle + 0x0222u), 12u);
        expected = expected_step(expected, 2u);

        require(s64_persistent_settings((uint16_t)(cycle + 0x0033u))
            == cycle, 13u);
        expected = expected_step(expected, 3u);

        require(s64_persistent_publisher(cycle)
            == (uint16_t)(cycle + 0x0222u), 14u);
        expected = expected_step(expected, 2u);

        require(s64_persistent_editor(cycle)
            == (uint16_t)(cycle ^ 0x1111u), 15u);
        expected = expected_step(expected, 1u);

        require(shared_load(&actual) && actual == expected, 16u);
        require(owner.transitions == (uint16_t)cycle * 5u
            && owner.current_mode == 1u, 17u);
        require(owner.active_writes == 1u && owner.clean_writes == 0u, 18u);
    }

    /* First cycle loads all five mode entries. Each later cycle begins with an
     * Editor cache hit, then performs four real swaps. */
    require(owner.overlay_reads == 121u, 19u);
    require(owner.preloads == 3u, 20u);
    require(owner.captures == 1u && owner.applies == 1u, 21u);

    require(s64_session_lifecycle_end(&lifecycle)
        == S64_SESSION_LIFECYCLE_OK, 22u);
    require(owner.active_writes == 1u && owner.clean_writes == 1u
        && owner.restores == 1u && owner.verifies == 1u, 23u);
    require(!lifecycle.active_record_written && lifecycle.clean_record_written, 24u);
    require(shared_load(&actual) && actual == expected, 25u);

    *(volatile uint8_t*)0xc060 = 0x80u;
    *(volatile uint8_t*)0xc061 = 0xa5u;
    *(volatile uint8_t*)0xc062 = 0u;
    for (;;) {}
}
"""


def build() -> Path:
    OUT.mkdir(parents=True, exist_ok=True)
    cfg = OUT / "probe.cfg"
    cfg.write_text((CLIENT / "cfg/editor-overlay-fit.cfg").read_text().replace(
        "build/editor-overlay-fit/", "build/persistent-runtime-probe/"))

    wrappers = OUT / "wrappers.s"
    wrappers.write_text(fit.assembler_wrappers(
        {
            "MODEL": ["s64_persistent_editor"],
            "FILE": ["s64_persistent_publisher"],
            "RUNTIME": ["s64_persistent_settings"],
        },
        bridges={},
    ))

    sources = []
    for name, content in (
        ("model", MODEL), ("file", FILE), ("runtime", RUNTIME), ("main", MAIN)
    ):
        source = OUT / f"{name}.c"
        source.write_text(content)
        sources.append(source)

    flags = ["-t", "c64", "-Os", "-I", "src", "-C", str(cfg)]
    common_sources = [
        *sources,
        wrappers,
        CLIENT / "src/platform/code_overlay.c",
        CLIENT / "src/platform/reu.c",
        CLIENT / "src/platform/reu_dma.s",
        CLIENT / "src/platform/reu_runtime.c",
        CLIENT / "src/platform/reu_runtime_build.c",
        CLIENT / "src/platform/reu_runtime_allocate.c",
        CLIENT / "src/platform/reu_runtime_write.c",
        CLIENT / "src/platform/system_state.c",
        CLIENT / "src/platform/system_state_start.c",
        CLIENT / "src/platform/session_lifecycle.c",
        CLIENT / "src/platform/session_lifecycle_start.c",
    ]
    objects = []
    for index, source in enumerate(common_sources):
        obj = OUT / f"{index:02d}_{source.stem}.o"
        fit.run(["cl65", *flags, "-c", "-o", str(obj), str(source)], cwd=CLIENT)
        objects.append(str(obj))

    blobs = OUT / "blobs.s"
    prg = OUT / "persistent-runtime-probe.prg"
    map_path = OUT / "persistent-runtime-probe.map"

    # Relink after embedding the exact generated assets because their resident
    # blob addresses can move exported resident callbacks used by overlay code.
    for initial in (True, False, False):
        lines = ['.segment "RODATA"']
        for group in ("model", "file", "runtime"):
            lines += [
                f".export _{group}_image, _{group}_image_end",
                f"_{group}_image:",
                ".byte 0" if initial else f'.incbin "{OUT.as_posix()}/{group}.bin"',
                f"_{group}_image_end:",
            ]
        blobs.write_text("\n".join(lines) + "\n")
        blob_object = OUT / "blobs.o"
        fit.run(["cl65", *flags, "-c", "-o", str(blob_object), str(blobs)], cwd=CLIENT)
        fit.run(
            ["cl65", *flags, "-m", str(map_path), "-o", str(prg),
             *objects, str(blob_object)],
            cwd=CLIENT,
        )

    for group in ("model", "file", "runtime"):
        asset = OUT / f"{group}.bin"
        size = asset.stat().st_size
        print(f"{group.upper()} representative asset: {size} / {fit.OVERLAY_LIMIT} bytes")
        if size == 0 or size > fit.OVERLAY_LIMIT:
            raise fit.FitError(f"{group} representative asset does not fit")

    end = fit.bss_end(map_path)
    headroom = fit.RESIDENT_BSS_LIMIT - end - 1
    print(f"Persistent-owner diagnostic BSS ends at ${end:04X}; "
          f"{headroom} bytes before $8C00")
    if headroom < 1024:
        raise fit.FitError(
            f"persistent-owner diagnostic has only {headroom} bytes before $8C00"
        )
    print("Persistent-owner diagnostic fit passed; real Editor-root fit remains unverified.")
    return prg


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-only", action="store_true")
    parser.add_argument("--port", type=int, default=6526)
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
        "xvfb-run", "-a", args.vice_bin, "+sound",
        "-reu", "-reusize", "16384",
        "-autostartprgmode", "1",
        "-binarymonitor",
        "-binarymonitoraddress", f"ip4://127.0.0.1:{args.port}",
    ]
    for kind in ("kernal", "basic", "chargen"):
        rom = args.rom_dir / f"{kind}.rom"
        if not rom.is_file():
            raise fit.FitError(
                f"Missing {kind} ROM in {args.rom_dir}; "
                "use the pinned Open ROMs from tools/test_reu_vice.sh"
            )
        command += [f"-{kind}", str(rom)]
    command += ["-autostart", str(prg)]

    with (OUT / "vice.log").open("w") as log:
        process = subprocess.Popen(command, stdout=log, stderr=log)
        monitor = None
        try:
            monitor = wait_for_monitor("127.0.0.1", args.port, 20)
            deadline = time.monotonic() + 30
            while time.monotonic() < deadline:
                state = monitor.read_memory(0xc060, 0xc062)
                if state[1] == 0xa5:
                    print(
                        "VICE persistent runtime passed: one lifecycle owner, "
                        "3 initial REU asset preloads, 30 Editor/Publisher/Settings "
                        "cycles, exact resident+REU state, one CLEAN"
                    )
                    return 0
                if state[1] == 0x5a:
                    raise fit.FitError(
                        f"persistent runtime probe failed at step {state[2]}"
                    )
                monitor.resume()
                time.sleep(0.03)
            raise fit.FitError(f"persistent runtime probe timed out: {state.hex()}")
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
