#!/usr/bin/env python3
"""Build a diagnostic editor image with mutually exclusive REU code overlays.

This target measures whether editor, file, and destination-runtime code can share
one 15 KiB C64 RAM slot, including trusted-entry bootstrap and destination
ownership. It does not establish that an overlay is executable on hardware.
"""
from __future__ import annotations

import argparse
import os
import re
import shlex
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Iterable


ROOT = Path(__file__).resolve().parents[1]
CLIENT = ROOT / "client"
GROUPS: dict[str, tuple[str, ...]] = {
    "MODEL": ("src/editor.c", "src/editor_format.c"),
    "FILE": (
        "src/document.c",
        "src/storage/local.c",
        "src/ui.c",
        "src/menu.c",
        "src/publisher/editor_handoff.c",
    ),
    "RUNTIME": (
        "src/platform/module_runtime.c",
        "src/platform/transfer_capsule.c",
        "src/platform/module_session.c",
        "src/platform/module_session_read.c",
        "src/platform/session_lifecycle.c",
        "src/platform/session_journal.c",
        "src/platform/system_state.c",
        "src/platform/turbo.c",
        "src/platform/code_assets_validate.c",
        "src/platform/code_assets_session.c",
        "src/platform/code_assets_session_read.c",
        "src/platform/managed_editor.c",
        "src/platform/managed_editor_exit.c",
        "src/platform/basic_return.c",
        "src/platform/basic_return_read.c",
    ),
}
GROUP_IDS = {"MODEL": 1, "FILE": 2, "RUNTIME": 3}
BRIDGES: dict[str, tuple[str, ...]] = {
    "FILE": (
        "s64_editor_has_selection",
        "s64_editor_selection_start",
        "s64_editor_selection_end",
    ),
}
OVERLAY_LIMIT = 0x3C00
RESIDENT_BSS_LIMIT = 0x8C00


class FitError(RuntimeError):
    pass


def run(command: list[str], *, cwd: Path) -> subprocess.CompletedProcess[str]:
    try:
        result = subprocess.run(
            command, cwd=cwd, text=True, capture_output=True, check=False
        )
    except OSError as exc:
        raise FitError(f"Could not run {command[0]}: {exc}") from exc
    if result.returncode:
        detail = (result.stdout + result.stderr).strip()
        raise FitError(
            f"Command failed ({result.returncode}): {shlex.join(command)}"
            + (f"\n{detail}" if detail else "")
        )
    return result


def make_variables(make_output: str) -> dict[str, str]:
    wanted = {"C64U_SOURCES", "MANAGED_EDITOR_RUNTIME_SOURCES", "CFLAGS", "CL65"}
    found: dict[str, str] = {}
    for line in make_output.splitlines():
        match = re.match(r"^(C64U_SOURCES|MANAGED_EDITOR_RUNTIME_SOURCES|CODE_ASSET_RUNTIME_SOURCES|CFLAGS|CL65)\s*[:?+]?=\s*(.*)$", line)
        if match:
            found[match.group(1)] = match.group(2).strip()
    missing = wanted - found.keys()
    if missing:
        raise FitError("make -pn did not report: " + ", ".join(sorted(missing)))
    return found


def source_lists(make_output: str) -> tuple[list[str], list[str]]:
    values = make_variables(make_output)
    c64u = shlex.split(values["C64U_SOURCES"])
    runtime = shlex.split(values["MANAGED_EDITOR_RUNTIME_SOURCES"])
    return c64u, runtime


def public_s64_definitions(source: str) -> list[str]:
    """Find file-scope public s64_* definitions, including multiline signatures."""
    # Replace comments and quoted literals while preserving braces and line layout.
    cleaned = re.sub(
        r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/',
        lambda m: " " * len(m.group(0)) if "\n" not in m.group(0) else "\n" * m.group(0).count("\n"),
        source,
    )
    names: list[str] = []
    depth = 0
    declaration_start = 0
    for index, char in enumerate(cleaned):
        if char == "{" and depth == 0:
            declaration = cleaned[declaration_start:index]
            if not re.search(r"\bstatic\b", declaration):
                match = re.search(r"\b(s64_[A-Za-z0-9_]+)\s*\(", declaration)
                if match:
                    names.append(match.group(1))
            depth = 1
        elif char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                declaration_start = index + 1
        elif char == ";" and depth == 0:
            declaration_start = index + 1
    return list(dict.fromkeys(names))


def overlay_symbol(group: str, name: str) -> str:
    return f"s64_overlay_{group.lower()}_{name}"


def bridge_symbol(group: str, name: str) -> str:
    return f"s64_overlay_bridge_{group.lower()}_{name}"


def generated_translation_unit(
    group: str, source_path: str, group_names: Iterable[str], generated_dir: Path,
    client: Path, *, bridges: dict[str, tuple[str, ...]] | None = None,
) -> str:
    bridges = BRIDGES if bridges is None else bridges
    lines = [
        f'#pragma code-name ("{group}_CODE")',
        f'#pragma rodata-name ("{group}_RODATA")',
    ]
    for name in sorted(set(group_names)):
        lines.append(f"#define {name} {overlay_symbol(group, name)}")
    for name in bridges.get(group, ()):
        lines.append(f"#define {name} {bridge_symbol(group, name)}")
    included = os.path.relpath(client / source_path, generated_dir).replace(os.sep, "/")
    lines.append(f'#include "{included}"')
    return "\n".join(lines) + "\n"


def assembler_wrappers(groups: dict[str, list[str]], *,
    group_ids: dict[str, int] | None = None,
    bridges: dict[str, tuple[str, ...]] | None = None) -> str:
    group_ids = GROUP_IDS if group_ids is None else group_ids
    bridges = BRIDGES if bridges is None else bridges
    lines = [
        '.segment "CODE"',
        '.import _s64_code_overlay_enter',
        '.importzp sreg',
        '.import __HIMEM__, __STACKSIZE__',
        '.import __MAIN_START__, __MAIN_SIZE__',
        '.assert __MAIN_START__ + __MAIN_SIZE__ = __HIMEM__, lderror, "crt0 software stack must start at HIMEM"',
        '.assert __HIMEM__ - __STACKSIZE__ - $C800 >= $0400, lderror, "Overlay requires 1 KiB before stack"',
    ]
    for group, names in groups.items():
        for name in sorted(set(names)):
            lines.extend(
                [
                    f".export _{name}",
                    f".import _{overlay_symbol(group, name)}",
                    f"_{name}:",
                    "    pha",
                    "    txa",
                    "    pha",
                    "    lda sreg",
                    "    pha",
                    "    lda sreg+1",
                    "    pha",
                    f"    lda #${group_ids[group]:02x}",
                    "    jsr _s64_code_overlay_enter",
                    "    pla",
                    "    sta sreg+1",
                    "    pla",
                    "    sta sreg",
                    "    pla",
                    "    tax",
                    "    pla",
                    f"    jmp _{overlay_symbol(group, name)}",
                ]
            )
    for group, names in bridges.items():
        for name in names:
            lines.extend(
                [
                    f".export _{bridge_symbol(group, name)}",
                    f"_{bridge_symbol(group, name)}:",
                    f"    jsr _{name}",
                    "    pha",
                    "    txa",
                    "    pha",
                    "    lda sreg",
                    "    pha",
                    "    lda sreg+1",
                    "    pha",
                    f"    lda #${group_ids[group]:02x}",
                    "    jsr _s64_code_overlay_enter",
                    "    pla",
                    "    sta sreg+1",
                    "    pla",
                    "    sta sreg",
                    "    pla",
                    "    tax",
                    "    pla",
                    "    rts",
                ]
            )
    return "\n".join(lines) + "\n"


def imported_symbols(output: str) -> set[str]:
    names: set[str] = set()
    for line in output.splitlines():
        match = re.match(r'^\s*Name:\s*(?:"([^"]+)"|(\S+))\s*$', line)
        if match:
            names.add(match.group(1) or match.group(2))
    return names


def required_veneer_names(groups: dict[str, list[str]], imports: set[str], *,
    bridges: dict[str, tuple[str, ...]] | None = None) -> dict[str, list[str]]:
    bridges = BRIDGES if bridges is None else bridges
    # Same-group calls already use renamed symbols. Only resident callers and
    # the explicit return bridges need original-name overlay entry veneers.
    required = {symbol[1:] for symbol in imports if symbol.startswith("_")}
    required.update(name for names in bridges.values() for name in names)
    return {group: [name for name in names if name in required] for group, names in groups.items()}


def cross_group_import_errors(
    group: str, symbols: Iterable[str], definitions: dict[str, str], *,
    bridges: dict[str, tuple[str, ...]] | None = None
) -> list[str]:
    bridges = BRIDGES if bridges is None else bridges
    allowed_bridges = {bridge_symbol(group, name) for name in bridges.get(group, ())}
    errors: list[str] = []
    for symbol in symbols:
        c_name = symbol[1:] if symbol.startswith("_") else symbol
        if c_name in allowed_bridges:
            continue
        if c_name.startswith("s64_overlay_bridge_"):
            errors.append(f"imports non-whitelisted bridge {symbol}")
            continue
        owner = definitions.get(c_name)
        if owner is not None and owner != group:
            errors.append(
                f"imports {symbol}, whose implementation is in {owner}; "
                "an overlay must not call another overlay"
            )
        renamed = re.fullmatch(r"s64_overlay_([a-z]+)_(s64_\w+)", c_name)
        if renamed and renamed.group(1).upper() != group:
            errors.append(f"imports cross-overlay symbol {symbol}")
    return errors


def reject_cross_group_imports(
    group: str, objects: Iterable[Path], definitions: dict[str, str], od65: str, *,
    bridges: dict[str, tuple[str, ...]] | None = None
) -> None:
    bridges = BRIDGES if bridges is None else bridges
    observed_bridges: set[str] = set()
    for obj in objects:
        output = run([od65, "--dump-imports", str(obj)], cwd=CLIENT).stdout
        if not re.search(r"^\s*Imports:\s*$", output, re.MULTILINE):
            raise FitError(f"Could not parse import table from od65 output for {obj}")
        symbols = imported_symbols(output)
        errors = cross_group_import_errors(group, symbols, definitions, bridges=bridges)
        if errors:
            raise FitError(f"{obj.name} ({group}) " + "; ".join(errors))
        observed_bridges.update(
            symbol[1:] if symbol.startswith("_") else symbol
            for symbol in symbols
            if (symbol[1:] if symbol.startswith("_") else symbol)
            in {bridge_symbol(group, name) for name in bridges.get(group, ())}
        )
    expected_bridges = set(bridges.get(group, ()))
    bridge_prefix = f"s64_overlay_bridge_{group.lower()}_"
    observed_names = {
        name[len(bridge_prefix):]
        for name in observed_bridges
        if name.startswith(bridge_prefix)
    }
    if observed_names != expected_bridges:
        missing = expected_bridges - observed_names
        unexpected = observed_names - expected_bridges
        detail = []
        if missing:
            detail.append("missing bridge imports: " + ", ".join(sorted(missing)))
        if unexpected:
            detail.append("unexpected bridge imports: " + ", ".join(sorted(unexpected)))
        if detail:
            raise FitError(f"{group} overlay " + "; ".join(detail))


def bss_end(map_path: Path) -> int:
    pattern = re.compile(r"^BSS\s+([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)\s+")
    for line in map_path.read_text(encoding="utf-8").splitlines():
        match = pattern.match(line)
        if match:
            return int(match.group(2), 16)
    raise FitError(f"Could not find BSS segment in {map_path}")


def compile_overlay_layout(
    client: Path, output_dir: Path, sources: list[str],
    groups: dict[str, tuple[str, ...]], group_ids: dict[str, int],
    bridges: dict[str, tuple[str, ...]], cfg: Path, flags: list[str], *,
    compiler: str, od65: str = "od65", image_name: str,
) -> Path:
    group_for_source = {
        path: group for group, paths in groups.items() for path in paths
    }
    grouped_names: dict[str, list[str]] = {group: [] for group in groups}
    for group, paths in groups.items():
        for path in paths:
            full = client / path
            if not full.is_file():
                raise FitError(f"Required overlay source is missing: {path}")
            grouped_names[group].extend(public_s64_definitions(full.read_text(encoding="utf-8")))
    definitions = {
        name: group for group, names in grouped_names.items() for name in names
    }
    if not definitions:
        raise FitError("No grouped public s64_* definitions were found")

    if not cfg.is_file():
        raise FitError(f"Required overlay linker configuration is missing: {cfg}")
    for path in sources:
        if not (client / path).is_file():
            raise FitError(f"Source listed by make is missing: {path}")

    compiler = shutil.which(compiler) or compiler
    output_dir.mkdir(parents=True, exist_ok=True)
    generated = output_dir / "generated"
    object_dir = output_dir / "objects"
    generated.mkdir(parents=True, exist_ok=True)
    object_dir.mkdir(parents=True, exist_ok=True)

    wrapper_path = generated / "code_overlay_wrappers.s"
    objects: list[Path] = []
    grouped_objects: dict[str, list[Path]] = {group: [] for group in groups}

    for index, source in enumerate(sources):
        group = group_for_source.get(source)
        input_path = client / source
        compile_input = input_path
        if group:
            generated_input = generated / f"{group.lower()}_{index}_{input_path.stem}.c"
            generated_input.write_text(
                generated_translation_unit(
                    group, source, grouped_names[group], generated, client, bridges=bridges
                ),
                encoding="ascii",
                newline="\n",
            )
            compile_input = generated_input
        obj = object_dir / f"{index:03d}_{input_path.stem}.o"
        run([compiler, *flags, "-c", "-o", str(obj), str(compile_input)], cwd=client)
        objects.append(obj)
        if group:
            grouped_objects[group].append(obj)

    imports: set[str] = set()
    for obj in objects:
        imports.update(imported_symbols(run([od65, "--dump-imports", str(obj)], cwd=client).stdout))
    wrapper_path.write_text(assembler_wrappers(required_veneer_names(grouped_names, imports, bridges=bridges),
        group_ids=group_ids, bridges=bridges),
                           encoding="ascii", newline="\n")
    wrapper_obj = object_dir / "code_overlay_wrappers.o"
    run([compiler, *flags, "-c", "-o", str(wrapper_obj), str(wrapper_path)], cwd=client)
    objects.append(wrapper_obj)
    # Entry must use this exact link's sizes, never stale manifest assumptions.
    layout_path = generated / "code_overlay_lengths.s"
    layout_lines = ['.segment "RODATA"', '.export _s64_code_overlay_linked_lengths',
                    '_s64_code_overlay_linked_lengths:']
    for group in groups:
        layout_lines.extend([
            f'.import __{group}_CODE_SIZE__, __{group}_RODATA_SIZE__',
            f'.word __{group}_CODE_SIZE__ + __{group}_RODATA_SIZE__',
        ])
    layout_path.write_text("\n".join(layout_lines) + "\n", encoding="ascii")
    layout_obj = object_dir / "code_overlay_lengths.o"
    run([compiler, *flags, "-c", "-o", str(layout_obj), str(layout_path)], cwd=client)
    objects.append(layout_obj)
    for group, group_objects in grouped_objects.items():
        reject_cross_group_imports(group, group_objects, definitions, od65, bridges=bridges)

    map_path = output_dir / (image_name + ".map")
    program = output_dir / (image_name + ".prg")
    run(
        [
            compiler,
            *flags,
            "-C",
            str(cfg),
            "-m",
            str(map_path),
            "-o",
            str(program),
            *(str(obj) for obj in objects),
        ],
        cwd=client,
    )

    map_text = map_path.read_text()
    for group in groups:
        binary = output_dir / f"{group.lower()}.bin"
        if not binary.is_file():
            raise FitError(f"Linker did not produce expected overlay: {binary}")
        size = binary.stat().st_size
        print(f"{group} overlay: {size} bytes / {OVERLAY_LIMIT}")
        if size > OVERLAY_LIMIT:
            raise FitError(f"{group} overlay exceeds the 15 KiB limit by {size - OVERLAY_LIMIT} bytes")
        for segment in (f"{group}_CODE", f"{group}_RODATA"):
            match = re.search(
                rf"^{segment}\s+([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)\s+",
                map_text, re.MULTILINE,
            )
            if not match:
                raise FitError(f"Missing overlay segment {segment}")
            start, finish = (int(value, 16) for value in match.groups())
            if start < RESIDENT_BSS_LIMIT or finish >= RESIDENT_BSS_LIMIT + OVERLAY_LIMIT:
                raise FitError(f"{segment} leaves the reserved execution slot")
            if segment.endswith("_CODE") and start != RESIDENT_BSS_LIMIT:
                raise FitError(f"{segment} starts at a different execution address")

    end = bss_end(map_path)
    print(f"Resident BSS ends at ${end:04X}; slot begins at ${RESIDENT_BSS_LIMIT:04X}")
    if end >= RESIDENT_BSS_LIMIT:
        raise FitError(
            f"Resident BSS reaches ${end:04X}; it must end below ${RESIDENT_BSS_LIMIT:04X}"
        )
    print("Diagnostic overlay fit passed. Loader activation and C64U hardware execution remain unverified.")
    return program


def compile_editor_overlay_fit(
    client: Path, output_dir: Path, *, make: str = "make", cl65: str | None = None,
    od65: str = "od65",
) -> None:
    expected_output = (client / "build" / "editor-overlay-fit").resolve()
    if output_dir.resolve() != expected_output:
        raise FitError(
            "The linker config writes to client/build/editor-overlay-fit; "
            f"requested output directory was {output_dir}"
        )
    db = run([make, "-pn", "managed-editor-fit"], cwd=client)
    values = make_variables(db.stdout)
    c64u = shlex.split(values["C64U_SOURCES"])
    runtime = shlex.split(values["MANAGED_EDITOR_RUNTIME_SOURCES"])
    sources = list(dict.fromkeys(c64u + runtime + shlex.split(values.get("CODE_ASSET_RUNTIME_SOURCES", ""))
                               + ["src/platform/code_overlay.c", "src/platform/managed_editor.c",
                                  "src/platform/managed_editor_exit.c", "src/platform/basic_return.c",
                                  "src/platform/basic_return_read.c",
                                  "src/platform/basic_return_stub.s", "src/platform/crc32.c"]))
    flags = shlex.split(values["CFLAGS"])
    if "-DS64_REU_BUFFER=1" not in flags:
        flags.append("-DS64_REU_BUFFER=1")
    flags.append("-DS64_MANAGED_OVERLAY=1")
    program = compile_overlay_layout(
        client, output_dir, sources, GROUPS, GROUP_IDS, BRIDGES,
        client / "cfg" / "editor-overlay-fit.cfg", flags,
        compiler=cl65 or os.environ.get("CL65") or values["CL65"] or "cl65",
        od65=od65, image_name="editor-overlay-fit",
    )
    generator = ROOT / "tools" / "generate_code_asset_manifest.py"
    manifest = output_dir / "S64OVL.MNF"
    run([sys.executable, str(generator), str(output_dir), "--root", str(program)], cwd=client)
    run([sys.executable, str(generator), str(output_dir), "--root", str(program),
         "--validate", str(manifest)], cwd=client)
    print("Exact editor/code-asset manifest generated and revalidated (diagnostic only).")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client", type=Path, default=CLIENT)
    parser.add_argument(
        "--output-dir", type=Path, default=CLIENT / "build" / "editor-overlay-fit"
    )
    parser.add_argument("--make", default="make")
    parser.add_argument("--cl65")
    parser.add_argument("--od65", default="od65")
    args = parser.parse_args()
    try:
        compile_editor_overlay_fit(
            args.client.resolve(), args.output_dir.resolve(),
            make=args.make, cl65=args.cl65, od65=args.od65,
        )
    except FitError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
