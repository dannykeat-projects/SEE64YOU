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

echo "== Standalone package tests =="
php tools/tests/package_site_test.php

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

    echo "== Memory map guard tests =="
    python3 tools/tests/memory_map_test.py
else
    echo "SKIP Python tests (python3 not found)"
fi

echo "== Host charset tests =="
TMP_BIN=$(mktemp)
TMP_MODULE_DIR=""
trap 'rm -f "$TMP_BIN"; if [ -n "$TMP_MODULE_DIR" ]; then rm -rf "$TMP_MODULE_DIR"; fi' EXIT
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/charset_test.c client/src/charset.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host gap-buffer tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/buffer_test.c client/src/buffer.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host REU gap-buffer tests =="
cc -std=c99 -Wall -Wextra -DS64_REU_BUFFER=1 -Iclient/src \
    client/tests/reu_buffer_test.c client/src/buffer_reu.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host REU editor clipboard tests =="
cc -std=c99 -Wall -Wextra -DS64_REU_BUFFER=1 -Iclient/src \
    client/tests/reu_editor_clipboard_test.c client/src/buffer_reu.c \
    client/src/clipboard.c client/src/history_reu.c client/src/editor.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host REU multi-level history tests =="
cc -std=c99 -Wall -Wextra -DS64_REU_BUFFER=1 -Iclient/src \
    client/tests/reu_history_test.c client/src/buffer_reu.c \
    client/src/clipboard.c client/src/history_reu.c client/src/editor.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host editor tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/editor_test.c client/src/buffer.c client/src/history.c client/src/clipboard.c client/src/editor.c client/src/editor_format.c client/src/editor_format_link.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host document tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/document_test.c client/src/document.c client/src/charset.c client/src/buffer.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host UCI HTTP transport tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/http_test.c client/src/charset.c client/src/platform/http.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host Ultimate DOS tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/dos_test.c client/src/platform/dos.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host local document tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/local_documents_test.c client/src/buffer.c client/src/document.c client/src/charset.c client/src/platform/dos.c client/src/storage/local.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host publisher destination tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/destination_test.c client/src/document.c client/src/charset.c client/src/buffer.c client/src/publisher/destination.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host publisher state tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/publisher_state_test.c client/src/document.c client/src/charset.c client/src/buffer.c client/src/publisher/destination.c client/src/publisher/state.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host publisher settings exchange tests =="
cc -std=c99 -Wall -Wextra -DS64_REU_BUFFER=1 -Iclient/src \
    client/tests/publisher_settings_exchange_test.c \
    client/src/publisher/settings_exchange.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host publisher settings persistence tests =="
cc -std=c99 -Wall -Wextra -DS64_REU_BUFFER=1 -Iclient/src \
    client/tests/publisher_settings_store_test.c \
    client/src/publisher/settings_exchange.c client/src/publisher/settings_store.c \
    -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host publisher REU handoff tests =="
cc -std=c99 -Wall -Wextra -DS64_REU_BUFFER=1 -Iclient/src \
    client/tests/publisher_handoff_test.c client/src/buffer_reu.c \
    client/src/clipboard.c client/src/history_reu.c client/src/editor.c \
    client/src/document.c client/src/charset.c client/src/publisher/workspace.c \
    client/src/publisher/handoff.c \
    -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host relay client tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/relay_test.c client/src/charset.c client/src/platform/http.c client/src/publisher/workspace.c client/src/publisher/relay.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host standalone publisher tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/standalone_test.c client/src/charset.c client/src/platform/http.c client/src/publisher/workspace.c client/src/publisher/relay.c client/src/publisher/standalone.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host standalone document publisher tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/standalone_documents_test.c client/src/buffer.c client/src/document.c client/src/charset.c client/src/platform/http.c client/src/publisher/workspace.c client/src/publisher/relay.c client/src/publisher/standalone.c client/src/publisher/standalone_documents.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host standalone list/open tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/standalone_list_open_test.c client/src/buffer.c client/src/document.c client/src/charset.c client/src/platform/http.c client/src/publisher/workspace.c client/src/publisher/relay.c client/src/publisher/standalone.c client/src/publisher/standalone_documents.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host WordPress publisher auth tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/wordpress_test.c client/src/charset.c client/src/platform/http.c client/src/publisher/workspace.c client/src/publisher/relay.c client/src/publisher/wordpress.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host WordPress document publisher tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/wordpress_documents_test.c client/src/buffer.c client/src/document.c client/src/charset.c client/src/platform/http.c client/src/publisher/workspace.c client/src/publisher/relay.c client/src/publisher/wordpress.c client/src/publisher/wordpress_documents.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host UCI compile check =="
cc -std=c99 -Wall -Wextra -Iclient/src -c client/src/platform/uci.c -o "$TMP_BIN.o"
rm -f "$TMP_BIN.o"

if command -v cl65 >/dev/null 2>&1 \
    && command -v cc65 >/dev/null 2>&1 \
    && command -v ca65 >/dev/null 2>&1; then
    echo "== cc65 C64 module checks =="
    TMP_MODULE_DIR=$(mktemp -d)
    for source in \
        client/src/charset.c \
        client/src/document.c \
        client/src/editor_format.c \
        client/src/editor_format_link.c \
        client/src/platform/dos.c \
        client/src/platform/http.c \
        client/src/platform/reu.c \
        client/src/publisher/destination.c \
        client/src/publisher/state.c \
        client/src/publisher/workspace.c \
        client/src/publisher/relay.c \
        client/src/publisher/standalone.c \
        client/src/publisher/standalone_documents.c \
        client/src/publisher/wordpress.c \
        client/src/publisher/wordpress_documents.c
    do
        base=$(basename "$source" .c)
        cc65 -t c64 -Oirs -DS64_NATIVE_PETSCII=1 -I client/src \
            -o "$TMP_MODULE_DIR/$base.s" "$source"
        ca65 -t c64 -o "$TMP_MODULE_DIR/$base.o" "$TMP_MODULE_DIR/$base.s"
    done

    cc65 -t c64 -Oirs -DS64_NATIVE_PETSCII=1 -DS64_REU_BUFFER=1 -I client/src \
        -o "$TMP_MODULE_DIR/buffer_reu.s" client/src/buffer_reu.c
    ca65 -t c64 -o "$TMP_MODULE_DIR/buffer_reu.o" "$TMP_MODULE_DIR/buffer_reu.s"

    cc65 -t c64 -Oirs -DS64_NATIVE_PETSCII=1 -DS64_REU_BUFFER=1 -I client/src \
        -o "$TMP_MODULE_DIR/history_reu.s" client/src/history_reu.c
    ca65 -t c64 -o "$TMP_MODULE_DIR/history_reu.o" "$TMP_MODULE_DIR/history_reu.s"

    for source in \
        client/src/publisher/settings_bridge.c \
        client/src/publisher/settings_exchange.c \
        client/src/publisher/settings_store.c
    do
        base=$(basename "$source" .c)
        cc65 -t c64 -Oirs -DS64_NATIVE_PETSCII=1 -DS64_REU_BUFFER=1 -I client/src \
            -o "$TMP_MODULE_DIR/$base.s" "$source"
        ca65 -t c64 -o "$TMP_MODULE_DIR/$base.o" "$TMP_MODULE_DIR/$base.s"
    done

    echo "== cc65 C64 build =="
    make -C client clean all

    echo "== cc65 C64U REU editor build =="
    make -C client c64u

    if command -v python3 >/dev/null 2>&1; then
        echo "== C64 memory headroom =="
        python3 tools/check_memory_headroom.py client/build/see64you.map \
            --label "Base C64" --min-headroom 256
        python3 tools/check_memory_headroom.py client/build/see64you-c64u.map \
            --label "C64U" --min-headroom 1024
    else
        echo "SKIP memory headroom checks (python3 not found)"
    fi

    echo "== Base editor diagnostic build =="
    make -C client editor-probe

    echo "== Editor UI diagnostic build =="
    make -C client ui-probe

    echo "== Keyboard repeat diagnostic build =="
    make -C client input-repeat-probe

    echo "== PETSCII publisher boundary diagnostic build =="
    make -C client publisher-charset-probe

    echo "== REU diagnostic build =="
    make -C client reu-probe

    echo "== REU buffer diagnostic build =="
    make -C client reu-buffer-probe

    echo "== REU editor diagnostic build =="
    make -C client reu-editor-probe

    echo "== Recovery diagnostic build =="
    make -C client recovery-probe

    echo "== C64U publisher + settings helper build =="
    make -C client publisher
    if command -v python3 >/dev/null 2>&1; then
        python3 tools/check_memory_headroom.py client/build/see64you-publisher.map \
            --label "C64U publisher" --min-headroom 1024
        python3 tools/check_memory_headroom.py client/build/see64you-settings.map \
            --label "C64U settings helper" --min-headroom 1024
    fi

    if command -v x64sc >/dev/null 2>&1 \
        && command -v xvfb-run >/dev/null 2>&1 \
        && command -v python3 >/dev/null 2>&1 \
        && command -v curl >/dev/null 2>&1; then
        echo "== VICE REU hardware validation =="
        sh ./tools/test_reu_vice.sh
    else
        echo "SKIP REU VICE validation (x64sc/xvfb-run/python3/curl required)"
    fi

    rm -rf "$TMP_MODULE_DIR"
    TMP_MODULE_DIR=""
else
    echo "SKIP cc65 build (cl65/cc65/ca65 not found)"
fi

echo "Validation passed"
