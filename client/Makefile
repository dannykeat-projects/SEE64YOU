TARGET := build/see64you.prg
REU_PROBE_TARGET := build/reu-probe.prg
REU_PROBE_VICE_TARGET := build/reu-probe-vice.prg
REU_PROBE_VICE_SMOKE_TARGET := build/reu-probe-vice-smoke.prg
SOURCES := src/main.c src/charset.c src/buffer.c src/history.c src/clipboard.c src/document.c src/editor.c src/editor_format.c src/ui.c src/ui_status.s src/input/input.c src/platform/uci.c src/platform/dos.c src/storage/local.c
REU_PROBE_SOURCES := diagnostics/reu_probe.c src/platform/reu.c

CL65 ?= cl65
# Keep the active conventional-C64 editor inside base RAM. Basic formatting is
# linked in editor_format.c. The larger URL/link formatter remains host/module-tested
# but unlinked because it does not fit while retaining the 1 KiB software stack.
CFLAGS := -t c64 -Os -I src -DS64_NATIVE_PETSCII=1 -Wl -D,__STACKSIZE__=0x0400

.PHONY: all reu-probe reu-probe-vice reu-probe-vice-smoke clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/see64you.map -o $@ $(SOURCES)

reu-probe: $(REU_PROBE_TARGET)

$(REU_PROBE_TARGET): $(REU_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/reu-probe.map -o $@ $(REU_PROBE_SOURCES)

reu-probe-vice: $(REU_PROBE_VICE_TARGET)

$(REU_PROBE_VICE_TARGET): $(REU_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_VICE_DEBUG_EXIT=1 -m build/reu-probe-vice.map -o $@ $(REU_PROBE_SOURCES)

reu-probe-vice-smoke: $(REU_PROBE_VICE_SMOKE_TARGET)

$(REU_PROBE_VICE_SMOKE_TARGET): $(REU_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_VICE_DEBUG_EXIT=1 -DS64_VICE_DEBUG_SMOKE=1 -o $@ $(REU_PROBE_SOURCES)

clean:
	rm -rf build
