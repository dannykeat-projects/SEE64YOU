#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
VICE_BIN=${VICE_BIN:-x64sc}
PRG="$ROOT/client/build/reu-probe-vice.prg"
ROM_DIR=${S64_VICE_ROM_DIR:-/tmp/see64you-roms}
OPEN_ROMS_COMMIT=ad178dbe4d48cd6a317737a8e0e7e662f7e33d32

if ! command -v "$VICE_BIN" >/dev/null 2>&1; then
    echo "SKIP REU VICE tests ($VICE_BIN not found)"
    exit 0
fi

if [ ! -f "$PRG" ]; then
    echo "REU VICE test PRG not found: $PRG" >&2
    exit 1
fi

prepare_roms() {
    if [ -s "$ROM_DIR/kernal.rom" ] \
        && [ -s "$ROM_DIR/basic.rom" ] \
        && [ -s "$ROM_DIR/chargen.rom" ]; then
        return
    fi

    if ! command -v curl >/dev/null 2>&1; then
        echo "VICE ROMs are missing and curl is unavailable" >&2
        exit 1
    fi

    mkdir -p "$ROM_DIR"
    base="https://raw.githubusercontent.com/MEGA65/open-roms/$OPEN_ROMS_COMMIT/bin"
    curl --fail --location --silent --show-error \
        "$base/kernal_generic.rom" -o "$ROM_DIR/kernal.rom"
    curl --fail --location --silent --show-error \
        "$base/basic_generic.rom" -o "$ROM_DIR/basic.rom"
    curl --fail --location --silent --show-error \
        "$base/chargen_openroms.rom" -o "$ROM_DIR/chargen.rom"

    test "$(stat -c %s "$ROM_DIR/kernal.rom")" = "8192"
    test "$(stat -c %s "$ROM_DIR/basic.rom")" = "8192"
    test "$(stat -c %s "$ROM_DIR/chargen.rom")" = "4096"
}

run_vice() {
    size_kib=$1
    log=$2

    if command -v xvfb-run >/dev/null 2>&1; then
        timeout 20s xvfb-run -a "$VICE_BIN" \
            -default \
            +sound \
            -silent \
            -kernal "$ROM_DIR/kernal.rom" \
            -basic "$ROM_DIR/basic.rom" \
            -chargen "$ROM_DIR/chargen.rom" \
            -debugcart \
            -reu \
            -reusize "$size_kib" \
            -autostartprgmode 1 \
            -autostart "$PRG" \
            >"$log" 2>&1
    else
        timeout 20s "$VICE_BIN" \
            -default \
            +sound \
            -silent \
            -kernal "$ROM_DIR/kernal.rom" \
            -basic "$ROM_DIR/basic.rom" \
            -chargen "$ROM_DIR/chargen.rom" \
            -debugcart \
            -reu \
            -reusize "$size_kib" \
            -autostartprgmode 1 \
            -autostart "$PRG" \
            >"$log" 2>&1
    fi
}

run_case() {
    size_kib=$1
    expected=$2
    log=$(mktemp)

    set +e
    run_vice "$size_kib" "$log"
    actual=$?
    set -e

    if [ "$actual" -ne "$expected" ]; then
        echo "FAIL REU $size_kib KiB: expected exit $expected, got $actual" >&2
        cat "$log" >&2
        rm -f "$log"
        exit 1
    fi

    rm -f "$log"
    echo "OK REU $size_kib KiB"
}

prepare_roms

run_case 128 1
run_case 256 2
run_case 512 3
run_case 1024 4
run_case 2048 5
run_case 4096 6
run_case 8192 7
run_case 16384 8

echo "REU VICE tests passed"
