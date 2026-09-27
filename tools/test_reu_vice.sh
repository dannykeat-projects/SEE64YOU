#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
VICE_BIN=${VICE_BIN:-x64sc}
PRG="$ROOT/client/build/reu-probe-vice.prg"

if ! command -v "$VICE_BIN" >/dev/null 2>&1; then
    echo "SKIP REU VICE tests ($VICE_BIN not found)"
    exit 0
fi

if [ ! -f "$PRG" ]; then
    echo "REU VICE test PRG not found: $PRG" >&2
    exit 1
fi

run_case() {
    size_kib=$1
    expected=$2
    log=$(mktemp)

    set +e
    timeout 20s "$VICE_BIN" \
        -default \
        -console \
        -silent \
        -debugcart \
        -reu \
        -reusize "$size_kib" \
        -autostartprgmode 1 \
        -autostart "$PRG" \
        >"$log" 2>&1
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

run_case 128 1
run_case 256 2
run_case 512 3
run_case 1024 4
run_case 2048 5
run_case 4096 6
run_case 8192 7
run_case 16384 8

echo "REU VICE tests passed"
