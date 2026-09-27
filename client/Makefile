TARGET := build/see64you.prg
C64U_TARGET := build/see64you-c64u.prg
REU_PROBE_TARGET := build/reu-probe.prg
REU_BUFFER_PROBE_TARGET := build/reu-buffer-probe.prg
REU_EDITOR_PROBE_TARGET := build/reu-editor-probe.prg
SOURCES := src/main.c src/charset.c src/buffer.c src/history.c src/clipboard.c src/document.c src/editor.c src/editor_format.c src/ui.c src/ui_status.s src/input/input.c src/platform/uci.c src/platform/dos.c src/storage/local.c
C64U_SOURCES := $(filter-out src/buffer.c src/history.c,$(SOURCES)) src/buffer_reu.c src/history_reu.c src/platform/reu.c src/platform/reu_dma.s
REU_PROBE_SOURCES := diagnostics/reu_probe.c src/platform/reu.c src/platform/reu_dma.s
REU_BUFFER_PROBE_SOURCES := diagnostics/reu_buffer_probe.c src/buffer_reu.c src/platform/reu.c src/platform/reu_dma.s
REU_EDITOR_PROBE_SOURCES := diagnostics/reu_editor_probe.c src/buffer_reu.c src/history_reu.c src/clipboard.c src/editor.c src/platform/reu.c src/platform/reu_dma.s

CL65 ?= cl65
# Keep the active conventional-C64 editor inside base RAM. Basic formatting is
# linked in editor_format.c. The larger URL/link formatter remains host/module-tested
# but unlinked because it does not fit while retaining the 1 KiB software stack.
CFLAGS := -t c64 -Os -I src -DS64_NATIVE_PETSCII=1 -Wl -D,__STACKSIZE__=0x0400

.PHONY: all c64u reu-probe reu-buffer-probe reu-editor-probe clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/see64you.map -o $@ $(SOURCES)

c64u: $(C64U_TARGET)

$(C64U_TARGET): $(C64U_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -m build/see64you-c64u.map -o $@ $(C64U_SOURCES)

reu-probe: $(REU_PROBE_TARGET)

$(REU_PROBE_TARGET): $(REU_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/reu-probe.map -o $@ $(REU_PROBE_SOURCES)

reu-buffer-probe: $(REU_BUFFER_PROBE_TARGET)

$(REU_BUFFER_PROBE_TARGET): $(REU_BUFFER_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -m build/reu-buffer-probe.map -o $@ $(REU_BUFFER_PROBE_SOURCES)

reu-editor-probe: $(REU_EDITOR_PROBE_TARGET)

$(REU_EDITOR_PROBE_TARGET): $(REU_EDITOR_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -m build/reu-editor-probe.map -o $@ $(REU_EDITOR_PROBE_SOURCES)

clean:
	rm -rf build
