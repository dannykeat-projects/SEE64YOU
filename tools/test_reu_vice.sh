#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
VICE_BIN=${VICE_BIN:-x64sc}
EDITOR_PRG="$ROOT/client/build/editor-probe.prg"
UI_PRG="$ROOT/client/build/ui-probe.prg"
INPUT_REPEAT_PRG="$ROOT/client/build/input-repeat-probe.prg"
PUBLISHER_CHARSET_PRG="$ROOT/client/build/publisher-charset-probe.prg"
UCI_ABSENCE_PRG="$ROOT/client/build/uci-absence-probe.prg"
PUBLISHER_PRG="$ROOT/client/build/c64u-package/SEE64YOU/S64PUB.PRG"
SETTINGS_PRG="$ROOT/client/build/c64u-package/SEE64YOU/S64CFG.PRG"
EDITOR_CHAIN_PRG="$ROOT/client/build/c64u-package/SEE64YOU/S64EDIT.PRG"
PRG="$ROOT/client/build/reu-probe.prg"
BUFFER_PRG="$ROOT/client/build/reu-buffer-probe.prg"
REU_EDITOR_PRG="$ROOT/client/build/reu-editor-probe.prg"
RECOVERY_PRG="$ROOT/client/build/recovery-probe.prg"
ROM_DIR=${S64_VICE_ROM_DIR:-/tmp/see64you-roms}
OPEN_ROMS_COMMIT=ad178dbe4d48cd6a317737a8e0e7e662f7e33d32

if ! command -v "$VICE_BIN" >/dev/null 2>&1; then
    echo "SKIP REU VICE tests ($VICE_BIN not found)"
    exit 0
fi

if [ ! -f "$PRG" ] || [ ! -f "$BUFFER_PRG" ] || [ ! -f "$EDITOR_PRG" ] || [ ! -f "$UI_PRG" ] || [ ! -f "$INPUT_REPEAT_PRG" ] || [ ! -f "$PUBLISHER_CHARSET_PRG" ] || [ ! -f "$UCI_ABSENCE_PRG" ] || [ ! -f "$PUBLISHER_PRG" ] || [ ! -f "$SETTINGS_PRG" ] || [ ! -f "$EDITOR_CHAIN_PRG" ] || [ ! -f "$REU_EDITOR_PRG" ] || [ ! -f "$RECOVERY_PRG" ]; then
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

python3 "$ROOT/tools/test_code_overlay_vice.py" --rom-dir "$ROM_DIR" --vice-bin "$VICE_BIN"

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

ui_log="/tmp/see64you-editor-ui.log"
xvfb-run -a "$VICE_BIN" \
    -kernal "$ROM_DIR/kernal.rom" \
    -basic "$ROM_DIR/basic.rom" \
    -chargen "$ROM_DIR/chargen.rom" \
    +sound \
    -autostartprgmode 1 \
    -binarymonitor \
    -binarymonitoraddress "ip4://127.0.0.1:6517" \
    -autostart "$UI_PRG" \
    >"$ui_log" 2>&1 &
ui_pid=$!

if ! python3 "$ROOT/tools/vice/test_editor_ui.py" --port 6517 --timeout 20; then
    kill "$ui_pid" 2>/dev/null || true
    cat "$ui_log" >&2
    wait "$ui_pid" 2>/dev/null || true
    exit 1
fi
wait "$ui_pid" 2>/dev/null || true
echo "OK editor UI screen regression"

repeat_log="/tmp/see64you-input-repeat.log"
xvfb-run -a "$VICE_BIN" \
    -kernal "$ROM_DIR/kernal.rom" \
    -basic "$ROM_DIR/basic.rom" \
    -chargen "$ROM_DIR/chargen.rom" \
    +sound \
    -autostartprgmode 1 \
    -binarymonitor \
    -binarymonitoraddress "ip4://127.0.0.1:6516" \
    -autostart "$INPUT_REPEAT_PRG" \
    >"$repeat_log" 2>&1 &
repeat_pid=$!

if ! python3 "$ROOT/tools/vice/test_input_repeat.py" --port 6516 --timeout 20; then
    kill "$repeat_pid" 2>/dev/null || true
    cat "$repeat_log" >&2
    wait "$repeat_pid" 2>/dev/null || true
    exit 1
fi
wait "$repeat_pid" 2>/dev/null || true
echo "OK keyboard repeat regression"

charset_log="/tmp/see64you-publisher-charset.log"
xvfb-run -a "$VICE_BIN" \
    -kernal "$ROM_DIR/kernal.rom" \
    -basic "$ROM_DIR/basic.rom" \
    -chargen "$ROM_DIR/chargen.rom" \
    +sound \
    -autostartprgmode 1 \
    -binarymonitor \
    -binarymonitoraddress "ip4://127.0.0.1:6515" \
    -autostart "$PUBLISHER_CHARSET_PRG" \
    >"$charset_log" 2>&1 &
charset_pid=$!

if ! python3 "$ROOT/tools/vice/test_publisher_charset.py" --port 6515 --timeout 20; then
    kill "$charset_pid" 2>/dev/null || true
    cat "$charset_log" >&2
    wait "$charset_pid" 2>/dev/null || true
    exit 1
fi
wait "$charset_pid" 2>/dev/null || true
echo "OK PETSCII publisher wire-boundary regression"

uci_log="/tmp/see64you-uci-absence.log"
xvfb-run -a "$VICE_BIN" \
    -kernal "$ROM_DIR/kernal.rom" \
    -basic "$ROM_DIR/basic.rom" \
    -chargen "$ROM_DIR/chargen.rom" \
    +sound \
    -autostartprgmode 1 \
    -binarymonitor \
    -binarymonitoraddress "ip4://127.0.0.1:6514" \
    -autostart "$UCI_ABSENCE_PRG" \
    >"$uci_log" 2>&1 &
uci_pid=$!

if ! python3 "$ROOT/tools/vice/test_uci_absence.py" --port 6514 --timeout 20; then
    kill "$uci_pid" 2>/dev/null || true
    cat "$uci_log" >&2
    wait "$uci_pid" 2>/dev/null || true
    exit 1
fi
wait "$uci_pid" 2>/dev/null || true
echo "OK conventional-C64 UCI absence regression"

if command -v c1541 >/dev/null 2>&1; then
    package_disk="/tmp/see64you-package.d64"
    package_dir="/tmp/see64you-package-readback"
    rm -f "$package_disk"
    rm -rf "$package_dir"
    mkdir -p "$package_dir"

    c1541 -format "SEE64YOU,64" d64 "$package_disk" >/dev/null
    c1541 -attach "$package_disk" \
        -write "$EDITOR_CHAIN_PRG" s64edit \
        -write "$PUBLISHER_PRG" s64pub \
        -write "$SETTINGS_PRG" s64cfg \
        >/dev/null

    c1541 -attach "$package_disk" -read s64edit "$package_dir/S64EDIT.PRG" >/dev/null
    c1541 -attach "$package_disk" -read s64pub "$package_dir/S64PUB.PRG" >/dev/null
    c1541 -attach "$package_disk" -read s64cfg "$package_dir/S64CFG.PRG" >/dev/null

    cmp "$EDITOR_CHAIN_PRG" "$package_dir/S64EDIT.PRG"
    cmp "$PUBLISHER_PRG" "$package_dir/S64PUB.PRG"
    cmp "$SETTINGS_PRG" "$package_dir/S64CFG.PRG"
    echo "OK CBM-DOS package alias round trip"
else
    echo "SKIP CBM-DOS package alias test (c1541 not found)"
fi

recovery_log="/tmp/see64you-recovery.log"
xvfb-run -a "$VICE_BIN" \
    -kernal "$ROM_DIR/kernal.rom" \
    -basic "$ROM_DIR/basic.rom" \
    -chargen "$ROM_DIR/chargen.rom" \
    +sound \
    -autostartprgmode 1 \
    -binarymonitor \
    -binarymonitoraddress "ip4://127.0.0.1:6518" \
    -autostart "$RECOVERY_PRG" \
    >"$recovery_log" 2>&1 &
recovery_pid=$!

if ! python3 "$ROOT/tools/vice/test_recovery.py" --port 6518 --timeout 30; then
    kill "$recovery_pid" 2>/dev/null || true
    cat "$recovery_log" >&2
    wait "$recovery_pid" 2>/dev/null || true
    exit 1
fi
wait "$recovery_pid" 2>/dev/null || true
echo "OK base C64 recovery workload"

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
