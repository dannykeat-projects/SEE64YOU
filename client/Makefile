TARGET := build/see64you.prg
SOURCES := src/main.c src/charset.c src/buffer.c src/history.c src/clipboard.c src/document.c src/editor.c src/ui.c src/ui_status.s src/input/input.c src/platform/uci.c src/platform/dos.c src/storage/local.c

CL65 ?= cl65
# Keep the complete editor inside conventional C64 RAM. The current local-edit
# fast-path diagnostic retains the previously validated 512-byte software stack
# while its code-size/performance tradeoff is measured in VICE.
CFLAGS := -t c64 -Os -I src -DS64_NATIVE_PETSCII=1 -Wl -D,__STACKSIZE__=0x0200

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/see64you.map -o $@ $(SOURCES)

clean:
	rm -rf build
