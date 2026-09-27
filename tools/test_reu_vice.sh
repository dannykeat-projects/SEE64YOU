#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
VICE_BIN=${VICE_BIN:-x64sc}
EDITOR_PRG="$ROOT/client/build/editor-probe.prg"
PRG="$ROOT/client/build/reu-probe.prg"
BUFFER_PRG="$ROOT/client/build/reu-buffer-probe.prg"
REU_EDITOR_PRG="$ROOT/client/build/reu-editor-probe.prg"
ROM_DIR=${S64_VICE_ROM_DIR:-/tmp/see64you-roms}
OPEN_ROMS_COMMIT=ad178dbe4d48cd6a317737a8e0e7e662f7e33d32

if ! command -v "$VICE_BIN" >/dev/null 2>&1; then
    echo "SKIP REU VICE tests ($VICE_BIN not found)"
    exit 0
fi

if [ ! -f "$PRG" ] || [ ! -f "$BUFFER_PRG" ] || [ ! -f "$EDITOR_PRG" ] || [ ! -f "$ROOT/client/build/reu-editor-probe.prg" ]; then
    echo "VICE diagnostic PRG missing" >&2
    exit 1
fi

prepare_roms() {
    if [ -s "$ROM_DIR/kernal.rom" ] && [ -s "$ROM_DIR/basic.rom" ] && [ -s "$ROM_DIR/chargen.rom" ]; then
        return
    fi
    mkdir -p "$ROM_DIR"
    base="https://raw.githubusercontent.com/MEGA65/open-roms/$OPEN_ROMS_COMMIT/bin"
    curl --fail --location --silent --show-error "$base/kernal_generic.rom" -o "$ROM_DIR/kernal.rom"
    curl --fail --location --silent --show-error "$base/basic_generic.rom" -o "$ROM_DIR/basic.rom"
    curl --fail --location --silent --show-error "$base/chargen_openroms.rom" -o "$ROM_DIR/chargen.rom"
}

run_case() {
    size_kib=$1
    port=$2
    log="/tmp/see64you-reu-$size_kib.log"

    xvfb-run -a "$VICE_BIN" \
        -kernal "$ROM_DIR/kernal.rom" \
        -basic "$ROM_DIR/basic.rom" \
        -chargen "$ROM_DIR/chargen.rom" \
        +sound \
        -reu \
        -reusize "$size_kib" \
        -autostartprgmode 1 \
        -binarymonitor \
        -binarymonitoraddress "ip4://127.0.0.1:$port" \
        -autostart "$PRG" \
        >"$log" 2>&1 &
    vice_pid=$!

    if ! python3 "$ROOT/tools/vice/test_reu_probe.py" --port "$port" --size-kib "$size_kib"; then
        kill "$vice_pid" 2>/dev/null || true
        cat "$log" >&2
        wait "$vice_pid" 2>/dev/null || true
        exit 1
    fi

    wait "$vice_pid" 2>/dev/null || true
    echo "OK REU $size_kib KiB"
}

prepare_roms

base_editor_log="/tmp/see64you-editor.log"
xvfb-run -a "$VICE_BIN" \
    -kernal "$ROM_DIR/kernal.rom" \
    -basic "$ROM_DIR/basic.rom" \
    -chargen "$ROM_DIR/chargen.rom" \
    +sound \
    -autostartprgmode 1 \
    -binarymonitor \
    -binarymonitoraddress "ip4://127.0.0.1:6519" \
    -autostart "$EDITOR_PRG" \
    >"$base_editor_log" 2>&1 &
base_editor_pid=$!

if ! python3 "$ROOT/tools/vice/test_editor.py" --port 6519 --timeout 20; then
    kill "$base_editor_pid" 2>/dev/null || true
    cat "$base_editor_log" >&2
    wait "$base_editor_pid" 2>/dev/null || true
    exit 1
fi
wait "$base_editor_pid" 2>/dev/null || true
echo "OK base C64 editor workload"

run_case 128 6520
run_case 256 6521
run_case 512 6522
run_case 1024 6523
run_case 2048 6524
run_case 4096 6525
run_case 8192 6526
run_case 16384 6527

buffer_log="/tmp/see64you-reu-buffer.log"
xvfb-run -a "$VICE_BIN" \
    -kernal "$ROM_DIR/kernal.rom" \
    -basic "$ROM_DIR/basic.rom" \
    -chargen "$ROM_DIR/chargen.rom" \
    +sound \
    -reu \
    -reusize 128 \
    -autostartprgmode 1 \
    -binarymonitor \
    -binarymonitoraddress "ip4://127.0.0.1:6530" \
    -autostart "$BUFFER_PRG" \
    >"$buffer_log" 2>&1 &
buffer_pid=$!

if ! python3 "$ROOT/tools/vice/test_reu_buffer.py" --port 6530 --timeout 20; then
    kill "$buffer_pid" 2>/dev/null || true
    cat "$buffer_log" >&2
    wait "$buffer_pid" 2>/dev/null || true
    exit 1
fi
wait "$buffer_pid" 2>/dev/null || true
echo "OK REU 6000-byte buffer workload"

editor_log="/tmp/see64you-reu-editor.log"
xvfb-run -a "$VICE_BIN" \
    -kernal "$ROM_DIR/kernal.rom" \
    -basic "$ROM_DIR/basic.rom" \
    -chargen "$ROM_DIR/chargen.rom" \
    +sound \
    -reu \
    -reusize 128 \
    -autostartprgmode 1 \
    -binarymonitor \
    -binarymonitoraddress "ip4://127.0.0.1:6531" \
    -autostart "$REU_EDITOR_PRG" \
    >"$editor_log" 2>&1 &
editor_pid=$!

if ! python3 "$ROOT/tools/vice/test_reu_editor.py" --port 6531 --timeout 60; then
    kill "$editor_pid" 2>/dev/null || true
    cat "$editor_log" >&2
    wait "$editor_pid" 2>/dev/null || true
    exit 1
fi
wait "$editor_pid" 2>/dev/null || true
echo "OK REU 6000-byte editor workload"

echo "REU VICE matrix passed"
