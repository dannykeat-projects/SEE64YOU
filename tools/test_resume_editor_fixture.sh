#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
BIN=$(mktemp)
trap 'rm -f "$BIN"' EXIT
cc -std=c99 -Wall -Wextra -Werror -ffunction-sections -Wl,--gc-sections \
    -DS64_REU_BUFFER=1 -Iclient/src -Iclient/diagnostics -Iclient/build/resume-editor-fixture \
    client/tests/resume_editor_fixture_test.c client/diagnostics/resume_editor_fixture.c \
    client/src/platform/application_state.c client/src/platform/application_state_wire.c \
    client/src/platform/application_state_payload.c client/src/platform/application_state_editor_decode.c \
    client/src/platform/application_state_reserve.c client/src/platform/managed_editor_state.c \
    client/src/platform/managed_editor_restore.c client/src/platform/reu_runtime.c \
    client/src/platform/reu_runtime_build.c client/src/platform/reu_runtime_allocate.c client/src/platform/reu_runtime_write.c \
    client/src/platform/session_journal.c client/src/platform/crc32.c -o "$BIN"
"$BIN" client/build/resume-editor-fixture
