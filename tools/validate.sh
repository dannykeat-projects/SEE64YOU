#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"

echo "== PHP syntax =="
find site wordpress -name '*.php' -type f | while IFS= read -r file; do
    php -l "$file" >/dev/null
    echo "OK $file"
done

echo "== Standalone backend tests =="
php site/tests/run.php

echo "== Host gap-buffer tests =="
TMP_BIN=$(mktemp)
trap 'rm -f "$TMP_BIN"' EXIT
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/buffer_test.c client/src/buffer.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host editor tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/editor_test.c client/src/buffer.c client/src/clipboard.c client/src/editor.c -o "$TMP_BIN"
"$TMP_BIN"

if command -v cl65 >/dev/null 2>&1; then
    echo "== cc65 C64 build =="
    make -C client clean all
else
    echo "SKIP cc65 build (cl65 not found)"
fi

echo "Validation passed"
