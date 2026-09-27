#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"

run_fit() {
    target=$1
    map=$2
    label=$3

    echo "== $label =="
    if make -C client "$target"; then
        python3 tools/check_memory_headroom.py "$map" \
            --label "$label" --min-headroom 0 || true
    else
        echo "$label: DOES NOT LINK within current C64 memory layout"
    fi
}

run_fit publisher-profile-fit client/build/publisher-profile-fit.map "C64U + destination/state"
run_fit publisher-http-fit client/build/publisher-http-fit.map "C64U + destination/state + HTTP"
run_fit publisher-standalone-fit client/build/publisher-standalone-fit.map "C64U + standalone publisher"
run_fit publisher-wordpress-fit client/build/publisher-wordpress-fit.map "C64U + WordPress publisher"
