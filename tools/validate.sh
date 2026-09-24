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

echo "== WordPress protocol tests =="
php wordpress/tests/run.php

if command -v curl >/dev/null 2>&1; then
    echo "== Standalone HTTP API tests =="
    sh site/tests/http.sh
else
    echo "SKIP HTTP API tests (curl not found)"
fi

if command -v python3 >/dev/null 2>&1; then
    echo "== Secure relay tests =="
    python3 -m unittest discover -s relay/tests -p 'test_*.py'
else
    echo "SKIP secure relay tests (python3 not found)"
fi

echo "== Host gap-buffer tests =="
TMP_BIN=$(mktemp)
trap 'rm -f "$TMP_BIN"' EXIT
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/buffer_test.c client/src/buffer.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host editor tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/editor_test.c client/src/buffer.c client/src/clipboard.c client/src/editor.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host document tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/document_test.c client/src/document.c client/src/buffer.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host UCI HTTP transport tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/http_test.c client/src/platform/http.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host Ultimate DOS tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/dos_test.c client/src/platform/dos.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host relay client tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/relay_test.c client/src/platform/http.c client/src/publisher/relay.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host standalone publisher tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/standalone_test.c client/src/platform/http.c client/src/publisher/relay.c client/src/publisher/standalone.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host standalone document publisher tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/standalone_documents_test.c client/src/buffer.c client/src/document.c client/src/platform/http.c client/src/publisher/relay.c client/src/publisher/standalone.c client/src/publisher/standalone_documents.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host standalone list/open tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/standalone_list_open_test.c client/src/buffer.c client/src/document.c client/src/platform/http.c client/src/publisher/relay.c client/src/publisher/standalone.c client/src/publisher/standalone_documents.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host WordPress publisher auth tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/wordpress_test.c client/src/platform/http.c client/src/publisher/relay.c client/src/publisher/wordpress.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host WordPress document publisher tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/wordpress_documents_test.c client/src/buffer.c client/src/document.c client/src/platform/http.c client/src/publisher/relay.c client/src/publisher/wordpress.c client/src/publisher/wordpress_documents.c -o "$TMP_BIN"
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
