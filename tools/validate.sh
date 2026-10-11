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

    echo "== Module manifest generator tests =="
    python3 tools/tests/module_manifest_generator_test.py

    echo "== PRG extent guard tests =="
    python3 tools/tests/prg_extent_test.py

    echo "== Editor overlay fit helper tests =="
    python3 tools/tests/editor_overlay_fit_test.py
    python3 tools/tests/publisher_overlay_fit_test.py
    python3 tools/tests/code_asset_manifest_test.py
else
    echo "SKIP Python tests (python3 not found)"
fi

echo "== Host charset tests =="
TMP_BIN=$(mktemp)
TMP_MODULE_DIR=""
trap 'rm -f "$TMP_BIN"; if [ -n "$TMP_MODULE_DIR" ]; then rm -rf "$TMP_MODULE_DIR"; fi' EXIT
echo "== Host exit prompt tests =="
cc -std=c99 -Wall -Wextra -Werror -DCH_STOP=3 -Iclient/tests/launcher_stubs -Iclient/src \
    client/tests/ui_exit_test.c client/src/ui_exit.c -o "$TMP_BIN"
"$TMP_BIN"
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/charset_test.c client/src/charset.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host native text prompt tests =="
cc -std=c99 -Wall -Wextra -Werror -D__fastcall__= -ffunction-sections -fdata-sections \
    -Iclient/tests/ui_prompt_stubs -Iclient/src client/tests/ui_prompt_test.c \
    client/src/ui.c -Wl,--gc-sections -o "$TMP_BIN"
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

echo "== Host persistent open preservation tests =="
cc -std=c99 -Wall -Wextra -Werror -DS64_REU_BUFFER=1 -Iclient/src \
    client/tests/persistent_open_preservation_test.c client/src/buffer_reu.c \
    client/src/history_reu.c client/src/editor.c client/src/clipboard.c \
    client/src/charset.c client/src/publisher/workspace.c \
    client/diagnostics/persistent_document_stage.c \
    client/diagnostics/persistent_document_read.c \
    client/diagnostics/persistent_open_prepare.c \
    client/diagnostics/persistent_publisher_services.c \
    client/src/platform/reu_runtime.c client/src/platform/reu_runtime_build.c client/src/platform/reu_runtime_allocate.c \
    client/src/platform/reu_runtime_write.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host shared application-state tests =="
cc -std=c99 -Wall -Wextra -Werror -ffunction-sections -Wl,--gc-sections \
    -DS64_REU_BUFFER=1 -Iclient/src client/tests/application_state_test.c \
    client/src/platform/application_state.c client/src/platform/application_state_wire.c \
    client/src/platform/application_state_payload.c client/src/platform/application_state_editor_decode.c \
    client/src/platform/application_state_read.c client/src/platform/application_state_build.c \
    client/src/platform/application_state_reserve.c \
    client/src/platform/application_state_editor.c client/src/platform/reu_runtime.c \
    client/src/platform/reu_runtime_build.c client/src/platform/reu_runtime_allocate.c client/src/platform/reu_runtime_write.c client/src/platform/session_journal.c \
    client/src/platform/crc32.c client/src/buffer_reu.c client/src/history_reu.c \
    client/src/editor.c client/src/clipboard.c client/src/document.c client/src/charset.c \
    -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host managed Editor state tests =="
cc -std=c99 -Wall -Wextra -Werror -ffunction-sections -Wl,--gc-sections \
    -DS64_REU_BUFFER=1 -Iclient/src client/tests/managed_editor_state_test.c \
    client/src/platform/application_state.c client/src/platform/application_state_wire.c \
    client/src/platform/application_state_payload.c client/src/platform/application_state_editor_decode.c \
    client/src/platform/managed_editor_state.c client/src/platform/managed_editor_restore.c \
    client/src/platform/managed_editor_publish.c client/src/platform/managed_editor_prepare.c client/src/platform/system_state.c client/src/platform/managed_editor_revoke.c client/src/platform/application_state_build.c \
    client/src/platform/application_state_reserve.c \
    client/src/platform/application_state_editor.c client/src/platform/reu_runtime.c \
    client/src/platform/reu_runtime_build.c client/src/platform/reu_runtime_allocate.c client/src/platform/reu_runtime_write.c client/src/platform/session_journal.c \
    client/src/platform/crc32.c client/src/buffer_reu.c client/src/history_reu.c \
    client/src/editor.c client/src/storage/local.c client/src/clipboard.c client/src/document.c client/src/charset.c \
    -o "$TMP_BIN"
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

echo "== Host UCI network target tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/network_test.c client/src/platform/network.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host SHA-256/HMAC tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/sha256_test.c client/src/crypto/sha256.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host direct C64U auth signing tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/direct_auth_test.c client/src/publisher/direct_key.c client/src/publisher/direct_auth.c client/src/crypto/sha256.c client/src/charset.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host C64U platform requirement tests =="
cc -std=c99 -Wall -Wextra -Iclient/src \
    client/tests/platform_requirements_test.c client/src/platform/requirements.c \
    -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host 16 MiB REU runtime allocator tests =="
cc -std=c99 -Wall -Wextra -Iclient/src \
    client/tests/reu_runtime_test.c client/src/platform/reu_runtime.c client/src/platform/reu_runtime_build.c client/src/platform/reu_runtime_allocate.c client/src/platform/reu_runtime_write.c \
    -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host code overlay cache tests =="
cc -std=c99 -Wall -Wextra -Werror -Iclient/src \
    client/tests/code_overlay_test.c client/src/platform/code_overlay.c \
    client/src/platform/reu_runtime.c -o "$TMP_BIN"
"$TMP_BIN"

for ASSET_MODULE_FLAGS in "" "-DS64_CODE_ASSET_PUBLISHER=1"
do
    cc $ASSET_MODULE_FLAGS -std=c99 -Wall -Wextra -Werror -Iclient/src \
        client/tests/code_overlay_test.c client/src/platform/code_overlay.c \
        client/src/platform/reu_runtime.c -o "$TMP_BIN"
    "$TMP_BIN"
    echo "== Host code asset preload/session tests =="
    cc $ASSET_MODULE_FLAGS -std=c99 -Wall -Wextra -Werror -Iclient/src \
        client/tests/code_assets_test.c client/src/platform/code_overlay_bootstrap.c client/src/platform/code_assets.c \
        client/src/platform/code_assets_validate.c client/src/platform/code_assets_session.c client/src/platform/code_assets_session_build.c client/src/platform/code_assets_session_read.c \
        client/src/platform/code_overlay.c client/src/platform/crc32.c \
        client/src/platform/module_integrity.c client/src/platform/module_manifest.c \
        client/src/platform/reu_runtime.c client/src/platform/reu_runtime_build.c client/src/platform/reu_runtime_allocate.c client/src/platform/reu_runtime_write.c \
        client/src/platform/session_journal.c client/src/platform/session_journal_start.c \
        client/src/platform/system_state.c client/src/platform/system_state_start.c -o "$TMP_BIN"
    "$TMP_BIN"

done

echo "== Host C64U module manifest tests =="
cc -std=c99 -Wall -Wextra -Iclient/src \
    client/tests/module_manifest_test.c client/src/platform/module_manifest.c client/src/platform/module_manifest_build.c client/src/platform/module_manifest_parse.c \
    client/src/platform/reu_runtime.c client/src/platform/reu_runtime_build.c client/src/platform/reu_runtime_allocate.c client/src/platform/reu_runtime_write.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host C64U system-state manager tests =="
cc -std=c99 -Wall -Wextra -Iclient/src \
    client/tests/system_state_test.c client/src/platform/system_state.c client/src/platform/system_state_start.c \
    -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host C64U turbo backend tests =="
cc -std=c99 -Wall -Wextra -Iclient/src \
    client/tests/turbo_backend_test.c client/src/platform/system_state.c client/src/platform/system_state_start.c \
    client/src/platform/turbo.c client/src/platform/turbo_capture.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host C64U session journal tests =="
cc -std=c99 -Wall -Wextra -Iclient/src \
    client/tests/session_journal_test.c client/src/platform/session_journal.c client/src/platform/session_journal_start.c \
    client/src/platform/dos.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host Settings journal migration staging tests =="
cc -std=c99 -Wall -Wextra -Werror -Iclient/src \
    client/tests/journal_migration_test.c client/src/platform/journal_migration.c \
    client/src/platform/session_journal.c client/src/platform/session_journal_start.c \
    client/src/platform/dos.c client/src/platform/dos_file.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host C64U session lifecycle tests =="
cc -std=c99 -Wall -Wextra -Iclient/src \
    client/tests/session_lifecycle_test.c client/src/platform/session_lifecycle.c client/src/platform/session_lifecycle_start.c \
    client/src/platform/session_journal.c client/src/platform/session_journal_start.c client/src/platform/system_state.c client/src/platform/system_state_start.c \
    client/src/platform/dos.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host C64U session adoption tests =="
cc -std=c99 -Wall -Wextra -Iclient/src \
    client/tests/session_adopt_test.c client/src/platform/system_state.c client/src/platform/system_state_start.c \
    client/src/platform/turbo.c client/src/platform/turbo_capture.c client/src/platform/session_lifecycle.c client/src/platform/session_lifecycle_start.c \
    client/src/platform/session_journal.c client/src/platform/session_journal_start.c client/src/platform/dos.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host C64U session recovery tests =="
cc -std=c99 -Wall -Wextra -Iclient/src \
    client/tests/session_recovery_test.c client/src/platform/session_recovery.c \
    client/src/platform/session_journal.c client/src/platform/session_journal_start.c client/src/platform/dos.c \
    -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host C64U launcher recovery flow tests =="
cc -std=c99 -Wall -Wextra -Iclient/src \
    client/tests/launcher_recovery_flow_test.c client/src/launcher/recovery_flow.c \
    client/src/platform/session_recovery.c client/src/platform/session_journal.c client/src/platform/session_journal_start.c \
    client/src/platform/dos.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host C64U launcher recovery runtime tests =="
cc -std=c99 -Wall -Wextra -Iclient/src \
    client/tests/launcher_recovery_runtime_test.c \
    client/src/platform/module_session.c client/src/platform/module_session_read.c client/src/platform/module_session_build.c client/src/platform/transfer_capsule.c client/src/platform/transfer_capsule_build.c client/src/platform/transfer_capsule_write.c client/src/platform/module_transfer.c client/src/launcher/recovery_runtime.c client/src/launcher/recovery_flow.c \
    client/src/platform/session_recovery.c client/src/platform/session_journal.c client/src/platform/session_journal_start.c \
    client/src/platform/session_lifecycle.c client/src/platform/session_lifecycle_start.c client/src/platform/system_state.c client/src/platform/system_state_start.c \
    client/src/platform/module_manifest.c client/src/platform/module_manifest_build.c client/src/platform/module_manifest_parse.c client/src/platform/reu_runtime.c client/src/platform/reu_runtime_build.c client/src/platform/reu_runtime_allocate.c client/src/platform/reu_runtime_write.c client/src/platform/dos.c client/src/platform/turbo.c client/src/platform/turbo_capture.c client/src/charset.c \
    -o "$TMP_BIN"
"$TMP_BIN"

for managed_records in 0 1; do
    set --
    if [ "$managed_records" -eq 1 ]; then
        set -- -DS64_MANAGED_OVERLAY=1 -include client/tests/managed_record_workspace.h client/tests/managed_record_workspace.c
    fi
echo "== Host shared module session tests =="
cc "$@" -std=c99 -Wall -Wextra -Iclient/src \
    client/tests/module_session_test.c client/src/platform/module_session.c client/src/platform/module_session_read.c client/src/platform/module_session_build.c \
    client/src/platform/module_manifest.c client/src/platform/module_manifest_build.c client/src/platform/module_manifest_parse.c client/src/platform/reu_runtime.c client/src/platform/reu_runtime_build.c client/src/platform/reu_runtime_allocate.c client/src/platform/reu_runtime_write.c \
    client/src/platform/session_journal.c client/src/platform/session_journal_start.c client/src/platform/dos.c -o "$TMP_BIN"
"$TMP_BIN"
done

for managed_records in 0 1; do
    set --
    if [ "$managed_records" -eq 1 ]; then
        set -- -DS64_MANAGED_OVERLAY=1 -include client/tests/managed_record_workspace.h client/tests/managed_record_workspace.c
    fi
echo "== Host transfer capsule tests =="
cc "$@" -std=c99 -Wall -Wextra -Iclient/src \
    client/tests/transfer_capsule_test.c client/src/platform/transfer_capsule.c client/src/platform/transfer_capsule_build.c client/src/platform/transfer_capsule_write.c \
    client/src/platform/module_session.c client/src/platform/module_session_read.c client/src/platform/module_session_build.c client/src/platform/module_manifest.c client/src/platform/module_manifest_build.c client/src/platform/module_manifest_parse.c \
    client/src/platform/reu_runtime.c client/src/platform/reu_runtime_build.c client/src/platform/reu_runtime_allocate.c client/src/platform/reu_runtime_write.c client/src/platform/session_journal.c client/src/platform/session_journal_start.c \
    client/src/platform/dos.c -o "$TMP_BIN"
"$TMP_BIN"
done

echo "== Host one-shot module arrival tests =="
cc -std=c99 -Wall -Wextra -Iclient/src \
    client/tests/module_arrival_test.c client/src/platform/module_arrival.c \
    -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host module transfer plan tests =="
cc -std=c99 -Wall -Wextra -Iclient/src \
    client/tests/module_transfer_test.c client/src/platform/module_transfer.c \
    -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host launcher code asset coordination tests =="
cc -std=c99 -Wall -Wextra -Werror -Iclient/src \
    client/tests/launcher_code_assets_test.c client/src/launcher/code_assets_runtime.c \
    -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host managed editor adoption/exit tests =="
cc -std=c99 -Wall -Wextra -Werror -Iclient/src \
    client/tests/managed_editor_test.c client/src/platform/managed_editor.c \
    -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host managed editor settings path/fallback tests =="
cc -std=c99 -Wall -Wextra -Werror -DS64_REU_BUFFER=1 -Iclient/src \
    client/tests/managed_editor_settings_test.c \
    client/src/platform/managed_editor_settings.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host managed Publisher adoption/document tests =="
cc -std=c99 -Wall -Wextra -Werror -DS64_REU_BUFFER=1 -DS64_CODE_ASSET_PUBLISHER=1 -Iclient/src \
    client/tests/managed_publisher_test.c client/src/platform/managed_publisher.c \
    client/src/platform/managed_publisher_document.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host original SYS return record tests =="
cc -std=c99 -Wall -Wextra -Werror -Iclient/src client/tests/basic_return_test.c client/src/platform/basic_return.c client/src/platform/basic_return_read.c client/src/platform/basic_return_build.c client/src/platform/crc32.c client/src/platform/reu_runtime.c client/src/platform/reu_runtime_build.c client/src/platform/reu_runtime_allocate.c client/src/platform/reu_runtime_write.c client/src/platform/session_journal.c client/src/platform/session_journal_start.c client/src/platform/system_state.c client/src/platform/system_state_start.c client/src/platform/dos.c client/src/platform/dos_file.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host launcher original SYS return coordination tests =="
cc -std=c99 -Wall -Wextra -Werror -Iclient/src client/tests/launcher_basic_return_test.c client/src/launcher/basic_return_runtime.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host managed editor original SYS exit tests =="
cc -std=c99 -Wall -Wextra -Werror -DS64_REU_BUFFER=1 -include client/tests/managed_editor_exit_workspace.h -Iclient/src client/tests/managed_editor_exit_test.c client/src/platform/managed_editor_exit.c client/src/platform/managed_editor_revoke.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host managed module transition failure tests =="
cc -std=c99 -Wall -Wextra -Werror -Iclient/src \
    client/tests/module_transition_test.c client/src/platform/module_transition.c \
    client/src/platform/module_transfer.c client/src/platform/session_journal.c client/src/platform/dos.c -o "$TMP_BIN"
"$TMP_BIN"

for managed_records in 0 1; do
    set --
    if [ "$managed_records" -eq 1 ]; then
        set -- -DS64_MANAGED_OVERLAY=1 -include client/tests/managed_record_workspace.h client/tests/managed_record_workspace.c
    fi
echo "== Host destination module runtime tests =="
cc "$@" -std=c99 -Wall -Wextra -Iclient/src \
    client/tests/module_runtime_test.c client/src/platform/module_runtime.c client/src/platform/module_runtime_init.c client/src/platform/module_transition.c client/src/platform/module_transfer.c \
    client/src/platform/transfer_capsule.c client/src/platform/transfer_capsule_build.c client/src/platform/transfer_capsule_write.c client/src/platform/module_session.c client/src/platform/module_session_read.c client/src/platform/module_session_build.c \
    client/src/platform/module_manifest.c client/src/platform/module_manifest_build.c client/src/platform/module_manifest_parse.c client/src/platform/reu_runtime.c client/src/platform/reu_runtime_build.c client/src/platform/reu_runtime_allocate.c client/src/platform/reu_runtime_write.c \
    client/src/platform/session_lifecycle.c client/src/platform/session_lifecycle_start.c client/src/platform/session_journal.c client/src/platform/session_journal_start.c \
    client/src/platform/system_state.c client/src/platform/system_state_start.c client/src/platform/turbo.c client/src/platform/turbo_capture.c \
    client/src/platform/dos.c -o "$TMP_BIN"
"$TMP_BIN"
done

echo "== Host Ultimate DOS tests =="
cc -std=c99 -Wall -Wextra -Iclient/src \
    client/tests/module_integrity_test.c client/src/platform/module_integrity.c client/src/platform/crc32.c \
    client/src/platform/module_manifest.c client/src/platform/module_manifest_build.c client/src/platform/module_manifest_parse.c client/src/platform/system_state.c client/src/platform/system_state_start.c -o "$TMP_BIN"
"$TMP_BIN"
cc -std=c99 -Wall -Wextra -Iclient/tests/launcher_stubs -Iclient/src \
    client/tests/launcher_exit_test.c -o "$TMP_BIN"
"$TMP_BIN"
cc -std=c99 -Wall -Wextra -DS64_CODE_ASSET_LAUNCHER=1 -Iclient/tests/launcher_stubs -Iclient/src \
    client/tests/launcher_exit_test.c -o "$TMP_BIN"
"$TMP_BIN"
cc -std=c99 -Wall -Wextra -DS64_CODE_ASSET_LAUNCHER=1 -DS64_EXECUTABLE_HANDOFF=1 -Iclient/tests/launcher_stubs -Iclient/src \
    client/tests/launcher_exit_test.c -o "$TMP_BIN"
"$TMP_BIN"
cc -std=c99 -Wall -Wextra -Iclient/src \
    client/tests/module_preload_test.c client/src/platform/module_preload.c \
    client/src/platform/module_manifest.c client/src/platform/module_manifest_build.c client/src/platform/module_manifest_parse.c client/src/platform/dos.c \
    client/src/platform/dos_file.c client/src/platform/dos_reu.c -o "$TMP_BIN"
"$TMP_BIN"
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/dos_test.c client/src/platform/dos.c client/src/platform/dos_file.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host companion loader preflight tests =="
cc -std=c99 -Wall -Wextra -Iclient/src \
    client/tests/companion_loader_test.c client/src/platform/companion_loader.c \
    client/src/platform/dos.c client/src/platform/dos_file.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host settings I/O probe tests =="
cc -std=c99 -Wall -Wextra -DS64_REU_BUFFER=1 -Iclient/src \
    client/tests/settings_io_probe_test.c client/src/platform/dos.c \
    client/src/publisher/settings_exchange.c client/src/publisher/settings_validate.c client/src/charset.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host settings exchange probe tests =="
cc -std=c99 -Wall -Wextra -DS64_REU_BUFFER=1 -Iclient/src \
    client/tests/settings_exchange_probe_test.c client/src/platform/dos.c \
    client/src/publisher/settings_store.c client/src/charset.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host settings save trace tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/settings_save_trace_test.c -o "$TMP_BIN"
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
    client/src/publisher/settings_exchange.c client/src/publisher/settings_validate.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host managed settings session tests =="
cc -std=c99 -Wall -Wextra -Werror -DS64_REU_BUFFER=1 -Iclient/src \
    client/tests/settings_session_test.c client/src/publisher/settings_session.c \
    client/src/publisher/settings_exchange.c client/src/publisher/settings_validate.c \
    -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host publisher preview tests =="
cc -std=c99 -Wall -Wextra -Iclient/src \
    client/tests/publisher_preview_test.c client/src/buffer.c \
    client/src/publisher/preview.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host plain-text import/export tests =="
cc -std=c99 -Wall -Wextra -DS64_REU_BUFFER=1 -Iclient/src \
    client/tests/publisher_plaintext_test.c client/src/buffer_reu.c \
    client/src/charset.c client/src/publisher/plaintext.c -o "$TMP_BIN"
"$TMP_BIN"

echo "== Host publisher settings persistence tests =="
cc -std=c99 -Wall -Wextra -DS64_REU_BUFFER=1 -Iclient/src \
    client/tests/publisher_settings_store_test.c \
    client/src/charset.c client/src/publisher/settings_exchange.c client/src/publisher/settings_validate.c client/src/publisher/settings_store.c \
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

echo "== Host firmware-1.1 direct publisher tests =="
cc -std=c99 -Wall -Wextra -Iclient/src client/tests/standalone_direct_test.c client/src/buffer.c client/src/document.c client/src/charset.c client/src/platform/network.c client/src/crypto/sha256.c client/src/publisher/direct_key.c client/src/publisher/direct_key_store.c client/src/publisher/direct_auth.c client/src/publisher/workspace.c client/src/publisher/standalone_direct.c -o "$TMP_BIN"
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
        client/src/platform/dos_file.c \
        client/src/platform/dos_reu.c \
        client/src/platform/module_preload.c \
        client/src/platform/module_transition.c \
        client/src/platform/module_integrity.c client/src/platform/crc32.c \
        client/src/platform/companion_loader.c \
        client/src/platform/http.c \
        client/src/platform/network.c \
        client/src/platform/reu.c \
        client/src/platform/reu_runtime.c client/src/platform/reu_runtime_build.c client/src/platform/reu_runtime_allocate.c client/src/platform/reu_runtime_write.c \
        client/src/platform/module_manifest.c client/src/platform/module_manifest_build.c client/src/platform/module_manifest_parse.c \
        client/src/platform/requirements.c \
        client/src/platform/preflight.c \
        client/src/platform/session_journal.c client/src/platform/session_journal_start.c \
        client/src/platform/session_lifecycle.c client/src/platform/session_lifecycle_start.c \
        client/src/platform/session_recovery.c \
        client/src/launcher/recovery_flow.c \
        client/src/launcher/recovery_runtime.c \
        client/src/platform/system_state.c client/src/platform/system_state_start.c \
        client/src/platform/turbo.c client/src/platform/turbo_capture.c \
        client/src/publisher/destination.c \
        client/src/publisher/state.c \
        client/src/publisher/workspace.c \
        client/src/publisher/preview.c \
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
        client/src/publisher/plaintext.c \
        client/src/publisher/settings_bridge.c \
        client/src/publisher/settings_exchange.c client/src/publisher/settings_validate.c \
        client/src/publisher/settings_store.c \
        client/src/platform/managed_editor_publish.c \
        client/src/platform/application_state_build.c \
        client/src/platform/application_state_editor.c
    do
        base=$(basename "$source" .c)
        cc65 -t c64 -Oirs -DS64_NATIVE_PETSCII=1 -DS64_REU_BUFFER=1 -I client/src \
            -o "$TMP_MODULE_DIR/$base.s" "$source"
        ca65 -t c64 -o "$TMP_MODULE_DIR/$base.o" "$TMP_MODULE_DIR/$base.s"
    done

    echo "== Current C64U package build and manifest/memory checks =="
    make -C client publisher-overlay-fit
    make -C client c64u-package

    echo "== Authenticated resumed Editor diagnostic build and host gates =="
    make -C client resume-editor-gate
    sh ./tools/test_resume_editor_fixture.sh

    echo "== Base editor diagnostic build =="
    make -C client editor-probe

    echo "== Editor UI diagnostic build =="
    make -C client ui-probe

    echo "== Keyboard repeat diagnostic build =="
    make -C client input-repeat-probe

    echo "== PETSCII publisher boundary diagnostic build =="
    make -C client publisher-charset-probe

    echo "== Conventional-C64 UCI absence diagnostic build =="
    make -C client uci-absence-probe

    echo "== C64U platform preflight diagnostic build =="
    make -C client platform-preflight-probe

    echo "== C64U module-cache hardware gate build =="
    make -C client module-cache-gate

    echo "== C64U network target diagnostic build =="
    make -C client network-probe

    echo "== C64U companion DOS stream diagnostic build =="
    make -C client companion-stream-probe

    echo "== C64U resident replacement loader diagnostic build =="
    make -C client companion-launch-probe
    test "$(wc -c < client/build/companion-resident.bin)" -eq 1024
    python3 tools/check_memory_headroom.py client/build/companion-launch-probe.map \
        --label "C64U resident loader probe" --min-headroom 1024
    python3 tools/check_prg_extent.py client/build/S64LDTST.PRG \
        --label "C64U resident loader target" \
        --load-address 0x0801 --max-end 0xC7FF

    echo "== C64U HTTPS target diagnostic build =="
    make -C client https-probe

    echo "== Direct Standalone site diagnostic build =="
    make -C client direct-site-probe

    echo "== Direct authenticated Standalone diagnostic build =="
    make -C client direct-auth-probe

    echo "== C64U HMAC diagnostic build =="
    make -C client hmac-probe

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

    echo "== Persistent runtime overlay diagnostic fit =="
    python3 tools/test_persistent_runtime_vice.py --build-only
    echo "== Persistent document writer VICE diagnostic build =="
    python3 tools/test_persistent_document_write_vice.py --build-only
    echo "== Persistent staged open VICE diagnostic build =="
    python3 tools/test_persistent_open_vice.py --build-only
    echo "== Real persistent Editor root fit =="
    python3 tools/build_persistent_editor_fit.py

    if command -v x64sc >/dev/null 2>&1 \
        && command -v xvfb-run >/dev/null 2>&1 \
        && command -v python3 >/dev/null 2>&1 \
        && command -v curl >/dev/null 2>&1; then
        echo "== VICE REU and overlay ABI validation =="
        sh ./tools/test_reu_vice.sh
        echo "== VICE original SYS return validation =="
        python3 tools/test_basic_return_vice.py
        echo "== VICE module-transfer IRQ teardown =="
        python3 tools/test_module_transfer_vice.py
        echo "== VICE persistent runtime overlay validation =="
        python3 tools/test_persistent_runtime_vice.py
        echo "== VICE persistent document writer validation =="
        python3 tools/test_persistent_document_write_vice.py
        echo "== VICE staged persistent open validation =="
        python3 tools/test_persistent_open_vice.py
    else
        echo "SKIP REU VICE validation (x64sc/xvfb-run/python3/curl required)"
    fi

    rm -rf "$TMP_MODULE_DIR"
    TMP_MODULE_DIR=""
else
    echo "SKIP cc65 build (cl65/cc65/ca65 not found)"
fi

echo "Validation passed"
