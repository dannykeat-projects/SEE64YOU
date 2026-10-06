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
        "    ldy #$01\n    jsr s64_overlay_gate",
        "    pla\n    sta sreg+1\n    pla\n    sta sreg\n    pla\n    tax\n    pla",
        "    jmp _s64_overlay_model_s64_value",
        "_s64_overlay_bridge_file_s64_editor_has_selection:\n"
        "    jsr _s64_editor_has_selection\n"
        "    ldy #$02\n    jsr s64_overlay_gate\n    rts",
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
    assert "src/platform/application_state_wire.c" in FIT.GROUPS["RUNTIME"]
    assert "src/platform/application_state_payload.c" in FIT.GROUPS["FILE"]
    assert "src/platform/managed_editor_restore.c" in FIT.GROUPS["MODEL"]
    assert FIT.BRIDGES["MODEL"] == ("s64_application_state_decode_payload",)
    assert "s64_application_state_decode_editor" in FIT.BRIDGES["FILE"]
    assert FIT.GROUP_IDS["PUBLICATION"] == 4
    assert FIT.GROUPS["PUBLICATION"] == (
        "src/platform/managed_editor_prepare.c",
        "src/platform/managed_editor_publish.c",
        "src/platform/managed_editor_publisher_assets.c",
        "src/platform/dos_file.c",
        "src/platform/dos_reu.c",
        "src/platform/module_integrity.c",
        "src/platform/application_state_editor.c",
        "src/platform/application_state_build.c",
    )
    assert FIT.BRIDGES["PUBLICATION"] == (
        "s64_application_state_payload_valid",
        "s64_managed_editor_state_revoke",
        "s64_reu_runtime_store_verified",
        "s64_session_journal_encode",
        "s64_system_state_set_mode",
    )

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
    assert FIT.required_veneer_names({
        "MODEL": ["s64_editor_has_selection", "s64_unused"],
        "RUNTIME": ["s64_from_resident", "s64_internal"],
    }, {"_s64_from_resident", "_s64_overlay_runtime_s64_internal"}) == {
        "MODEL": ["s64_editor_has_selection"], "RUNTIME": ["s64_from_resident"],
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
