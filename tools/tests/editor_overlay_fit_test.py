#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import tempfile
import sys
from pathlib import Path

sys.dont_write_bytecode = True

SCRIPT = Path(__file__).resolve().parents[1] / "build_editor_overlay_fit.py"
SPEC = importlib.util.spec_from_file_location("editor_overlay_fit", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
FIT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(FIT)


def main() -> int:
    source = """
static int s64_private(void) { return 0; }
int s64_plain(int value) { return value; }
const char *
s64_message(
    unsigned char value
) { return value ? "yes" : "no"; }
"""
    names = FIT.public_s64_definitions(source)
    assert names == ["s64_plain", "s64_message"], names

    generated = FIT.generated_translation_unit(
        "MODEL",
        "src/editor.c",
        ["s64_move", "s64_init"],
        Path("/work/client/build/editor-overlay-fit/generated"),
        Path("/work/client"),
    )
    assert '#pragma code-name ("MODEL_CODE")' in generated
    assert '#pragma rodata-name ("MODEL_RODATA")' in generated
    assert "#define s64_init s64_overlay_model_s64_init" in generated
    assert '#include "../../../src/editor.c"' in generated

    file_generated = FIT.generated_translation_unit(
        "FILE",
        "src/ui.c",
        ["s64_ui_render"],
        Path("/work/client/build/editor-overlay-fit/generated"),
        Path("/work/client"),
    )
    for name in FIT.BRIDGES["FILE"]:
        assert f"#define {name} {FIT.bridge_symbol('FILE', name)}" in file_generated

    wrappers = FIT.assembler_wrappers(
        {
            "MODEL": [
                "s64_value",
                "s64_editor_has_selection",
                "s64_editor_selection_start",
                "s64_editor_selection_end",
            ],
            "FILE": ["s64_ui_render"],
        }
    )
    for expected in (
        ".importzp sreg",
        '.assert __HIMEM__ - __STACKSIZE__ - $C800 >= $0400, lderror, "Overlay requires 1 KiB before stack"',
        "    pha\n    txa\n    pha\n    lda sreg\n    pha\n    lda sreg+1\n    pha",
        "    lda #$01\n    jsr _s64_code_overlay_enter",
        "    pla\n    sta sreg+1\n    pla\n    sta sreg\n    pla\n    tax\n    pla",
        "    jmp _s64_overlay_model_s64_value",
        "_s64_overlay_bridge_file_s64_editor_has_selection:\n"
        "    jsr _s64_editor_has_selection\n"
        "    pha\n    txa\n    pha\n    lda sreg\n    pha\n    lda sreg+1\n    pha\n"
        "    lda #$02\n    jsr _s64_code_overlay_enter\n"
        "    pla\n    sta sreg+1\n    pla\n    sta sreg\n    pla\n    tax\n    pla\n    rts",
    ):
        assert expected in wrappers, expected
    assert ".import _s64_editor_has_selection\n" not in wrappers

    definitions = {
        "s64_editor_insert": "MODEL",
        "s64_file_open": "FILE",
    }
    assert FIT.cross_group_import_errors(
        "FILE",
        ["_s64_overlay_bridge_file_s64_editor_has_selection"],
        definitions,
    ) == []
    assert FIT.cross_group_import_errors(
        "FILE", ["_s64_editor_insert"], definitions
    )
    assert FIT.cross_group_import_errors(
        "FILE", ["_s64_overlay_bridge_file_s64_editor_insert"], definitions
    )
    assert FIT.cross_group_import_errors(
        "FILE", ["_s64_overlay_runtime_s64_module_runtime_end"], definitions
    )
    assert FIT.OVERLAY_LIMIT == 0x3C00
    assert FIT.RESIDENT_BSS_LIMIT == 0x8C00

    variables = FIT.make_variables(
        "C64U_SOURCES := src/main.c src/editor.c\n"
        "MANAGED_EDITOR_RUNTIME_SOURCES := src/platform/module_runtime.c\n"
        "CFLAGS := -t c64 -Os -I src\n"
        "CL65 = cl65\n"
    )
    assert FIT.source_lists(
        "C64U_SOURCES := src/main.c src/editor.c\n"
        "MANAGED_EDITOR_RUNTIME_SOURCES := src/platform/module_runtime.c\n"
        "CFLAGS := -t c64 -Os -I src\n"
        "CL65 = cl65\n"
    ) == (
        ["src/main.c", "src/editor.c"],
        ["src/platform/module_runtime.c"],
    )
    assert variables["CL65"] == "cl65"
    assert FIT.imported_symbols('Name: "_s64_other"\nName: _s64_plain') == {
        "_s64_other",
        "_s64_plain",
    }

    with tempfile.TemporaryDirectory() as directory:
        map_path = Path(directory) / "test.map"
        map_path.write_text(
            "Segment list:\nBSS 008000 0087FF 000800 00001\n",
            encoding="utf-8",
        )
        assert FIT.bss_end(map_path) == 0x87FF

    print("editor overlay fit helper tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
