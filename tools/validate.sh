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

if command -v curl >/dev/null 2>&1; then
    echo "== Standalone HTTP API tests =="
    sh site/tests/http.sh
else
    echo "SKIP HTTP API tests (curl not found)"
fi

echo "== Host gap-buffer tests =="
TMP_BIN=$(mktemp)
trap 'rm -f "$TMP_BIN"' EXIT
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/buffer_test.c client/src/buffer.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host editor tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/editor_test.c client/src/buffer.c client/src/clipboard.c client/src/editor.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host UCI HTTP transport tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/http_test.c client/src/platform/http.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host UCI compile check =="
cc -std=c99 -Wall -Wextra -Iclient/src -c client/src/platform/uci.c -o "$TMP_BIN.o"
rm -f "$TMP_BIN.o"

if command -v cl65 >/dev/null 2>&1; then
    echo "== cc65 C64 build =="
    make -C client clean all
else
    echo "SKIP cc65 build (cl65 not found)"
fi

echo "Validation passed"
