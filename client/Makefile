TARGET := build/see64you.prg
C64U_TARGET := build/see64you-c64u.prg
C64U_CHAIN_TARGET := build/SEE64YOU-C64U.PRG
EDITOR_PROBE_TARGET := build/editor-probe.prg
UI_PROBE_TARGET := build/ui-probe.prg
REU_PROBE_TARGET := build/reu-probe.prg
REU_BUFFER_PROBE_TARGET := build/reu-buffer-probe.prg
REU_EDITOR_PROBE_TARGET := build/reu-editor-probe.prg
RECOVERY_PROBE_TARGET := build/recovery-probe.prg
PUBLISHER_COMPANION_FIT_TARGET := build/publisher-companion-fit.prg
PUBLISHER_TARGET := build/see64you-publisher.prg
PUBLISHER_CHAIN_TARGET := build/S64PUB.PRG
SOURCES := src/main.c src/charset.c src/buffer.c src/history.c src/clipboard.c src/document.c src/editor.c src/editor_format.c src/ui.c src/ui_status.s src/input/input.c src/input/repeat.s src/platform/uci.c src/platform/dos.c src/storage/local.c
C64U_SOURCES := $(filter-out src/buffer.c src/history.c,$(SOURCES)) src/buffer_reu.c src/history_reu.c src/platform/reu.c src/platform/reu_dma.s src/platform/chain.s src/publisher/editor_handoff.c src/publisher/editor_handoff_record.s
EDITOR_PROBE_SOURCES := diagnostics/editor_probe.c src/buffer.c src/history.c src/clipboard.c src/editor.c src/editor_format.c
UI_PROBE_SOURCES := diagnostics/ui_probe.c diagnostics/ui_conio_shim.c src/buffer.c src/history.c src/clipboard.c src/editor.c src/editor_format.c src/ui.c src/ui_status.s
REU_PROBE_SOURCES := diagnostics/reu_probe.c src/platform/reu.c src/platform/reu_dma.s
REU_BUFFER_PROBE_SOURCES := diagnostics/reu_buffer_probe.c src/buffer_reu.c src/platform/reu.c src/platform/reu_dma.s
REU_EDITOR_PROBE_SOURCES := diagnostics/reu_editor_probe.c src/buffer_reu.c src/history_reu.c src/clipboard.c src/editor.c src/platform/reu.c src/platform/reu_dma.s
RECOVERY_PROBE_SOURCES := diagnostics/recovery_probe.c src/buffer.c src/document.c src/charset.c src/platform/dos.c src/storage/local.c
PUBLISHER_CORE_SOURCES := src/charset.c src/buffer_reu.c src/document.c src/platform/uci.c src/platform/http.c src/platform/reu.c src/platform/reu_dma.s src/publisher/destination.c src/publisher/state.c src/publisher/handoff.c src/publisher/relay.c src/publisher/standalone.c src/publisher/wordpress.c
PUBLISHER_DOCUMENT_SOURCES := src/publisher/standalone_documents.c src/publisher/wordpress_documents.c
PUBLISHER_COMPANION_FIT_SOURCES := diagnostics/publisher_companion_fit.c $(PUBLISHER_CORE_SOURCES) $(PUBLISHER_DOCUMENT_SOURCES)
PUBLISHER_SOURCES := src/publisher/main.c $(PUBLISHER_CORE_SOURCES) $(PUBLISHER_DOCUMENT_SOURCES)

CL65 ?= cl65
# Keep the active conventional-C64 editor inside base RAM. Basic formatting is
# linked in editor_format.c. The larger URL/link formatter remains host/module-tested
# but unlinked because it does not fit while retaining the 1 KiB software stack.
CFLAGS := -t c64 -Os -I src -DS64_NATIVE_PETSCII=1 -Wl -D,__STACKSIZE__=0x0400

.PHONY: all c64u publisher editor-probe ui-probe reu-probe reu-buffer-probe reu-editor-probe recovery-probe publisher-companion-fit clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/see64you.map -o $@ $(SOURCES)

c64u: $(C64U_TARGET) $(C64U_CHAIN_TARGET)

$(C64U_TARGET): $(C64U_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -m build/see64you-c64u.map -o $@ $(C64U_SOURCES)

$(C64U_CHAIN_TARGET): $(C64U_TARGET)
	cp $< $@

editor-probe: $(EDITOR_PROBE_TARGET)

$(EDITOR_PROBE_TARGET): $(EDITOR_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/editor-probe.map -o $@ $(EDITOR_PROBE_SOURCES)

ui-probe: $(UI_PROBE_TARGET)

$(UI_PROBE_TARGET): $(UI_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/ui-probe.map -o $@ $(UI_PROBE_SOURCES)

reu-probe: $(REU_PROBE_TARGET)

$(REU_PROBE_TARGET): $(REU_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/reu-probe.map -o $@ $(REU_PROBE_SOURCES)

reu-buffer-probe: $(REU_BUFFER_PROBE_TARGET)

$(REU_BUFFER_PROBE_TARGET): $(REU_BUFFER_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -m build/reu-buffer-probe.map -o $@ $(REU_BUFFER_PROBE_SOURCES)

publisher: $(PUBLISHER_TARGET) $(PUBLISHER_CHAIN_TARGET)

$(PUBLISHER_TARGET): $(PUBLISHER_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -m build/see64you-publisher.map -o $@ $(PUBLISHER_SOURCES)

$(PUBLISHER_CHAIN_TARGET): $(PUBLISHER_TARGET)
	cp $< $@

publisher-companion-fit: $(PUBLISHER_COMPANION_FIT_TARGET)

$(PUBLISHER_COMPANION_FIT_TARGET): $(PUBLISHER_COMPANION_FIT_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -m build/publisher-companion-fit.map -o $@ $(PUBLISHER_COMPANION_FIT_SOURCES)

recovery-probe: $(RECOVERY_PROBE_TARGET)

$(RECOVERY_PROBE_TARGET): $(RECOVERY_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/recovery-probe.map -o $@ $(RECOVERY_PROBE_SOURCES)

reu-editor-probe: $(REU_EDITOR_PROBE_TARGET)

$(REU_EDITOR_PROBE_TARGET): $(REU_EDITOR_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -m build/reu-editor-probe.map -o $@ $(REU_EDITOR_PROBE_SOURCES)

clean:
	rm -rf build
