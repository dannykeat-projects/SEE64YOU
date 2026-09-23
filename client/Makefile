TARGET := build/see64you.prg
SOURCES := src/main.c src/buffer.c src/editor.c src/ui.c

CL65 ?= cl65
CFLAGS := -t c64 -Oirs -I src

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -o $@ $(SOURCES)

clean:
	rm -rf build
