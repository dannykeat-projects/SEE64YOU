#!/usr/bin/env python3
"""Guard Publisher-specific overlay boundaries and resident callbacks."""
import sys
from pathlib import Path

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import build_publisher_overlay_fit as publisher

fit = publisher.fit
definitions = {"s64_preview": "CONTENT", "s64_connect": "NETWORK"}
for symbol in ("_s64_preview", "_s64_overlay_content_s64_preview"):
    assert fit.cross_group_import_errors("NETWORK", [symbol], definitions, bridges={})
assert not fit.cross_group_import_errors("NETWORK", ["_s64_connect"], definitions, bridges={})
assert not fit.cross_group_import_errors("CONTENT", ["_s64_document_read"], definitions, bridges={})
wrappers = fit.assembler_wrappers(
    {"CONTENT": ["s64_preview"], "NETWORK": ["s64_connect"]},
    group_ids={"CONTENT": 1, "NETWORK": 2}, bridges={},
)
assert "ldy #$01\n    jsr s64_overlay_gate" in wrappers
assert "ldy #$02\n    jsr s64_overlay_gate" in wrappers
assert "s64_overlay_bridge_" not in wrappers
# Streaming callbacks must return into the same still-loaded overlay.
for resident in ("src/document.c", "src/platform/dos.c", "src/platform/reu.c"):
    assert resident not in publisher.CONTENT + publisher.NETWORK
assert "diagnostics/managed_publisher_state_fit.c" not in publisher.RESIDENT_SUPPORT
assert "src/platform/code_overlay_bootstrap.c" in publisher.RESIDENT_SUPPORT
assert "src/platform/managed_publisher.c" in publisher.RUNTIME_SUPPORT
assert "src/platform/code_assets_session_read.c" in publisher.RUNTIME_SUPPORT
assert "src/platform/reu_runtime_write.c" in publisher.RUNTIME_SUPPORT
assert "src/publisher/settings_session.c" in publisher.NETWORK
assert "src/publisher/settings_validate.c" in publisher.NETWORK
assert "src/platform/application_state.c" in publisher.CONTENT
assert "src/platform/application_state_build.c" in publisher.CONTENT
assert "src/platform/application_state_payload.c" in publisher.CONTENT
assert "src/platform/managed_publisher_document.c" in publisher.CONTENT
for name in publisher.BRIDGES["CONTENT"]:
    assert not fit.cross_group_import_errors("CONTENT",
        ["_" + fit.bridge_symbol("CONTENT", name)], {name: "RUNTIME"},
        bridges=publisher.BRIDGES)
assert publisher.BRIDGES["NETWORK"] == ("s64_session_journal_encode",)
assert publisher.BRIDGES["RUNTIME"] == ("s64_application_state_load_wire",)
assert not fit.cross_group_import_errors("NETWORK",
    ["_" + fit.bridge_symbol("NETWORK", "s64_session_journal_encode")],
    {"s64_session_journal_encode": "RUNTIME"}, bridges=publisher.BRIDGES)
assert not fit.cross_group_import_errors("RUNTIME",
    ["_" + fit.bridge_symbol("RUNTIME", "s64_application_state_load_wire")],
    {"s64_application_state_load_wire": "CONTENT"}, bridges=publisher.BRIDGES)
assert fit.cross_group_import_errors("RUNTIME", ["_s64_application_state_load_wire"],
    {"s64_application_state_load_wire": "CONTENT"}, bridges=publisher.BRIDGES)
assert "src/platform/module_transition.c" in publisher.CONTENT
assert "src/platform/reu_runtime.c" in publisher.RESIDENT_SUPPORT
print("publisher overlay fit helper tests passed")
