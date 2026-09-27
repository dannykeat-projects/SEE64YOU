TARGET := build/see64you.prg
SOURCES := src/main.c src/charset.c src/buffer.c src/history.c src/clipboard.c src/document.c src/editor.c src/editor_format.c src/ui.c src/ui_status.s src/input/input.c src/platform/uci.c src/platform/dos.c src/storage/local.c

CL65 ?= cl65
# Keep the active conventional-C64 editor inside base RAM. Formatting lives in
# editor_format.c and remains host/target-module tested, but is not linked until
# the FORMAT UI is wired. The recovered headroom restores a 1 KiB software stack.
CFLAGS := -t c64 -Os -I src -DS64_NATIVE_PETSCII=1 -Wl -D,__STACKSIZE__=0x0400

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/see64you.map -o $@ $(SOURCES)

clean:
	rm -rf build
