$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $Root

function Invoke-Checked {
    param(
        [Parameter(Mandatory=$true)][string]$Description,
        [Parameter(Mandatory=$true)][scriptblock]$Command
    )
    Write-Host "== $Description =="
    & $Command
    if ($LASTEXITCODE -ne 0) {
        throw "$Description failed"
    }
}

Write-Host "== PHP syntax =="
Get-ChildItem site, wordpress -Recurse -Filter *.php | ForEach-Object {
    & php -l $_.FullName | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "PHP lint failed: $($_.FullName)" }
    Write-Host "OK $($_.FullName)"
}

Invoke-Checked "Standalone backend tests" { & php site/tests/run.php }
Invoke-Checked "Standalone package tests" { & php tools/tests/package_site_test.php }
Invoke-Checked "WordPress protocol tests" { & php wordpress/tests/run.php }

$Sh = Get-Command sh -ErrorAction SilentlyContinue
$Curl = Get-Command curl.exe -ErrorAction SilentlyContinue
if ($Sh -and $Curl) {
    Invoke-Checked "Standalone HTTP API tests" { & $Sh.Path site/tests/http.sh }
} else {
    Write-Host "SKIP HTTP API tests (sh/curl.exe not found)"
}

$Python = @("python", "python3") |
    ForEach-Object { Get-Command $_ -ErrorAction SilentlyContinue } |
    Select-Object -First 1
if ($Python) {
    Invoke-Checked "Secure relay tests" {
        & $Python.Path -m unittest discover -s relay/tests -p "test_*.py"
    }
    Invoke-Checked "Editor overlay fit helper tests" {
        & $Python.Path tools/tests/publisher_overlay_fit_test.py
        if ($LASTEXITCODE -ne 0) { throw "Publisher overlay helper tests failed" }
        & $Python.Path tools/tests/editor_overlay_fit_test.py
    }
    Invoke-Checked "Code asset manifest tests" {
        & $Python.Path tools/tests/code_asset_manifest_test.py
    }
} else {
    Write-Host "SKIP secure relay tests (python not found)"
}

$HostCc = @("cc", "gcc", "clang") |
    ForEach-Object { Get-Command $_ -ErrorAction SilentlyContinue } |
    Select-Object -First 1

if ($HostCc) {
    $TestDir = Join-Path ([System.IO.Path]::GetTempPath()) ("see64you-tests-" + [guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Force $TestDir | Out-Null

    function Invoke-CTest {
        param(
            [Parameter(Mandatory=$true)][string]$Name,
            [Parameter(Mandatory=$true)][string[]]$Sources
        )

        $Output = Join-Path $TestDir ($Name + ".exe")
        Write-Host "== Host $Name tests =="
        & $HostCc.Path -std=c99 -Wall -Wextra -Iclient/tests/launcher_stubs -Iclient/src @Sources -o $Output
        if ($LASTEXITCODE -ne 0) { throw "Host $Name test build failed" }
        & $Output
        if ($LASTEXITCODE -ne 0) { throw "Host $Name tests failed" }
    }

    try {
        Invoke-CTest "charset" @(
            "client/tests/charset_test.c",
            "client/src/charset.c"
        )
        Invoke-CTest "buffer" @(
            "client/tests/buffer_test.c",
            "client/src/buffer.c"
        )
        Invoke-CTest "editor" @(
            "client/tests/editor_test.c",
            "client/src/buffer.c",
            "client/src/history.c",
            "client/src/clipboard.c",
            "client/src/editor.c",
            "client/src/editor_format.c",
            "client/src/editor_format_link.c"
        )
        Invoke-CTest "document" @(
            "client/tests/document_test.c",
            "client/src/document.c",
            "client/src/charset.c",
            "client/src/buffer.c"
        )
        Invoke-CTest "http" @(
            "client/tests/http_test.c",
            "client/src/charset.c",
            "client/src/platform/http.c"
        )
        Invoke-CTest "destination" @(
            "client/tests/destination_test.c",
            "client/src/document.c",
            "client/src/charset.c",
            "client/src/buffer.c",
            "client/src/publisher/destination.c"
        )
        Invoke-CTest "publisher_state" @(
            "client/tests/publisher_state_test.c",
            "client/src/document.c",
            "client/src/charset.c",
            "client/src/buffer.c",
            "client/src/publisher/destination.c",
            "client/src/publisher/state.c"
        )
        Invoke-CTest "relay" @(
            "client/tests/relay_test.c",
            "client/src/charset.c",
            "client/src/platform/http.c",
            "client/src/publisher/relay.c"
        )
        Invoke-CTest "standalone" @(
            "client/tests/standalone_test.c",
            "client/src/charset.c",
            "client/src/platform/http.c",
            "client/src/publisher/relay.c",
            "client/src/publisher/standalone.c"
        )
        Invoke-CTest "standalone_documents" @(
            "client/tests/standalone_documents_test.c",
            "client/src/buffer.c",
            "client/src/document.c",
            "client/src/charset.c",
            "client/src/platform/http.c",
            "client/src/publisher/relay.c",
            "client/src/publisher/standalone.c",
            "client/src/publisher/standalone_documents.c"
        )
        Invoke-CTest "standalone_list_open" @(
            "client/tests/standalone_list_open_test.c",
            "client/src/buffer.c",
            "client/src/document.c",
            "client/src/charset.c",
            "client/src/platform/http.c",
            "client/src/publisher/relay.c",
            "client/src/publisher/standalone.c",
            "client/src/publisher/standalone_documents.c"
        )
        Invoke-CTest "wordpress" @(
            "client/tests/wordpress_test.c",
            "client/src/charset.c",
            "client/src/platform/http.c",
            "client/src/publisher/relay.c",
            "client/src/publisher/wordpress.c"
        )
        Invoke-CTest "wordpress_documents" @(
            "client/tests/wordpress_documents_test.c",
            "client/src/buffer.c",
            "client/src/document.c",
            "client/src/charset.c",
            "client/src/platform/http.c",
            "client/src/publisher/relay.c",
            "client/src/publisher/wordpress.c",
            "client/src/publisher/wordpress_documents.c"
        )
        Invoke-CTest "reu_runtime" @(
            "client/tests/reu_runtime_test.c",
            "client/src/platform/reu_runtime_build.c",
            "client/src/platform/reu_runtime.c"
        )
        Invoke-CTest "code_overlay" @(
            "client/tests/code_overlay_test.c",
            "client/src/platform/code_overlay.c",
            "client/src/platform/reu_runtime.c"
        )
        Invoke-CTest "module_manifest" @(
            "client/tests/module_manifest_test.c",
            "client/src/platform/module_manifest_build.c",
            "client/src/platform/module_manifest_parse.c",
            "client/src/platform/module_manifest.c",
            "client/src/platform/reu_runtime_build.c",
            "client/src/platform/reu_runtime.c"
        )
        $CodeAssetSources = @(
            "client/tests/code_assets_test.c",
            "client/src/platform/code_overlay_bootstrap.c",
            "client/src/platform/code_assets.c",
            "client/src/platform/code_assets_validate.c",
            "client/src/platform/code_assets_session.c", "client/src/platform/code_assets_session_build.c", "client/src/platform/code_assets_session_read.c",
            "client/src/platform/code_overlay.c",
            "client/src/platform/crc32.c",
            "client/src/platform/module_integrity.c",
            "client/src/platform/module_manifest.c",
            "client/src/platform/reu_runtime.c",
            "client/src/platform/reu_runtime_build.c",
            "client/src/platform/session_journal.c",
            "client/src/platform/session_journal_start.c",
            "client/src/platform/system_state.c",
            "client/src/platform/system_state_start.c"
        )
        Invoke-CTest "code_assets" $CodeAssetSources
        Invoke-CTest "publisher_code_assets" (@("-DS64_CODE_ASSET_PUBLISHER=1") + $CodeAssetSources)
        Invoke-CTest "ultimate_dos" @(
            "client/tests/dos_test.c",
            "client/src/platform/dos.c"
        )
        Invoke-CTest "session_adopt" @(
            "client/tests/session_adopt_test.c",
            "client/src/platform/system_state_start.c",
            "client/src/platform/system_state.c",
            "client/src/platform/turbo_capture.c",
            "client/src/platform/turbo.c",
            "client/src/platform/session_lifecycle_start.c",
            "client/src/platform/session_lifecycle.c",
            "client/src/platform/session_journal_start.c",
            "client/src/platform/session_journal.c",
            "client/src/platform/dos.c"
        )
        Invoke-CTest "launcher_recovery_flow" @(
            "client/tests/launcher_recovery_flow_test.c",
            "client/src/launcher/recovery_flow.c",
            "client/src/platform/session_recovery.c",
            "client/src/platform/session_journal_start.c",
            "client/src/platform/session_journal.c",
            "client/src/platform/dos.c"
        )
        Invoke-CTest "launcher_recovery_runtime" @(
            "client/src/charset.c",
            "client/tests/launcher_recovery_runtime_test.c",
            "client/src/launcher/recovery_runtime.c",
            "client/src/platform/module_session_build.c",
            "client/src/platform/module_session.c", "client/src/platform/module_session_read.c",
            "client/src/platform/transfer_capsule_build.c",
            "client/src/platform/transfer_capsule.c",
            "client/src/platform/module_transfer.c",
            "client/src/launcher/recovery_flow.c",
            "client/src/platform/session_recovery.c",
            "client/src/platform/session_journal_start.c",
            "client/src/platform/session_journal.c",
            "client/src/platform/session_lifecycle_start.c",
            "client/src/platform/session_lifecycle.c",
            "client/src/platform/system_state_start.c",
            "client/src/platform/system_state.c",
            "client/src/platform/module_manifest_build.c",
            "client/src/platform/module_manifest_parse.c",
            "client/src/platform/module_manifest.c",
            "client/src/platform/reu_runtime_build.c",
            "client/src/platform/reu_runtime.c",
            "client/src/platform/dos.c",
            "client/src/platform/turbo_capture.c",
            "client/src/platform/turbo.c"
        )
        Invoke-CTest "launcher_exit" @("client/tests/launcher_exit_test.c")
        Invoke-CTest "launcher_assets_exit" @("-DS64_CODE_ASSET_LAUNCHER=1", "client/tests/launcher_exit_test.c")
        Invoke-CTest "launcher_handoff_exit" @("-DS64_CODE_ASSET_LAUNCHER=1", "-DS64_EXECUTABLE_HANDOFF=1", "client/tests/launcher_exit_test.c")
        Invoke-CTest "module_session" @(
            "client/tests/module_session_test.c",
            "client/src/platform/module_session_build.c",
            "client/src/platform/module_session.c", "client/src/platform/module_session_read.c",
            "client/src/platform/module_manifest_build.c",
            "client/src/platform/module_manifest_parse.c",
            "client/src/platform/module_manifest.c",
            "client/src/platform/reu_runtime_build.c",
            "client/src/platform/reu_runtime.c",
            "client/src/platform/session_journal_start.c",
            "client/src/platform/session_journal.c",
            "client/src/platform/dos.c"
        )
        Invoke-CTest "transfer_capsule" @(
            "client/tests/transfer_capsule_test.c",
            "client/src/platform/transfer_capsule_build.c",
            "client/src/platform/transfer_capsule.c",
            "client/src/platform/module_session_build.c",
            "client/src/platform/module_session.c", "client/src/platform/module_session_read.c",
            "client/src/platform/module_manifest_build.c",
            "client/src/platform/module_manifest_parse.c",
            "client/src/platform/module_manifest.c",
            "client/src/platform/reu_runtime_build.c",
            "client/src/platform/reu_runtime.c",
            "client/src/platform/session_journal_start.c",
            "client/src/platform/session_journal.c",
            "client/src/platform/dos.c"
        )
        Invoke-CTest "module_arrival" @(
            "client/tests/module_arrival_test.c",
            "client/src/platform/module_arrival.c"
        )
        Invoke-CTest "module_transfer" @(
            "client/tests/module_transfer_test.c",
            "client/src/platform/module_transfer.c"
        )
        Invoke-CTest "launcher_code_assets" @(
            "client/tests/launcher_code_assets_test.c",
            "client/src/launcher/code_assets_runtime.c"
        )
        Invoke-CTest "application_state" @(
            "-Werror", "-ffunction-sections", "-Wl,--gc-sections", "-DS64_REU_BUFFER=1",
            "client/tests/application_state_test.c",
            "client/src/platform/application_state.c",
            "client/src/platform/application_state_payload.c",
            "client/src/platform/application_state_read.c",
            "client/src/platform/application_state_build.c",
            "client/src/platform/application_state_reserve.c",
            "client/src/platform/application_state_editor.c",
            "client/src/platform/reu_runtime.c",
            "client/src/platform/reu_runtime_build.c",
            "client/src/platform/session_journal.c",
            "client/src/platform/crc32.c",
            "client/src/buffer_reu.c", "client/src/history_reu.c", "client/src/editor.c",
            "client/src/clipboard.c", "client/src/document.c", "client/src/charset.c"
        )
        Invoke-CTest "managed_editor" @(
            "client/tests/managed_editor_test.c",
            "client/src/platform/managed_editor.c"
        )
        Invoke-CTest "managed_publisher" @(
            "-Werror", "-DS64_REU_BUFFER=1", "-DS64_CODE_ASSET_PUBLISHER=1",
            "client/tests/managed_publisher_test.c",
            "client/src/platform/managed_publisher.c",
            "client/src/platform/managed_publisher_document.c"
        )
        Invoke-CTest "basic_return_test" @(
            "-Werror",
            "client/tests/basic_return_test.c",
            "client/src/platform/basic_return.c",
            "client/src/platform/basic_return_read.c",
            "client/src/platform/basic_return_build.c",
            "client/src/platform/crc32.c",
            "client/src/platform/reu_runtime.c",
            "client/src/platform/reu_runtime_build.c",
            "client/src/platform/session_journal.c",
            "client/src/platform/session_journal_start.c",
            "client/src/platform/system_state.c",
            "client/src/platform/system_state_start.c",
            "client/src/platform/dos.c",
            "client/src/platform/dos_file.c"
        )
        Invoke-CTest "launcher_basic_return_test" @(
            "-Werror",
            "client/tests/launcher_basic_return_test.c",
            "client/src/launcher/basic_return_runtime.c"
        )
        Invoke-CTest "managed_editor_exit_test" @(
            "-Werror",
            "client/tests/managed_editor_exit_test.c",
            "client/src/platform/managed_editor_exit.c"
        )
        Invoke-CTest "module_transition" @(
            "-Werror",
            "client/tests/module_transition_test.c",
            "client/src/platform/module_transition.c",
            "client/src/platform/module_transfer.c",
            "client/src/platform/session_journal.c",
            "client/src/platform/dos.c"
        )
        Invoke-CTest "module_runtime" @(
            "client/tests/module_runtime_test.c",
            "client/src/platform/module_transition.c",
            "client/src/platform/module_transfer.c",
            "client/src/platform/module_runtime.c",
            "client/src/platform/transfer_capsule_build.c",
            "client/src/platform/transfer_capsule.c",
            "client/src/platform/module_session_build.c",
            "client/src/platform/module_session.c", "client/src/platform/module_session_read.c",
            "client/src/platform/module_manifest_build.c",
            "client/src/platform/module_manifest_parse.c",
            "client/src/platform/module_manifest.c",
            "client/src/platform/reu_runtime_build.c",
            "client/src/platform/reu_runtime.c",
            "client/src/platform/session_lifecycle_start.c",
            "client/src/platform/session_lifecycle.c",
            "client/src/platform/session_journal_start.c",
            "client/src/platform/session_journal.c",
            "client/src/platform/system_state_start.c",
            "client/src/platform/system_state.c",
            "client/src/platform/turbo_capture.c",
            "client/src/platform/turbo.c",
            "client/src/platform/dos.c"
        )
        Invoke-CTest "module_integrity" @(
            "client/tests/module_integrity_test.c",
            "client/src/platform/module_integrity.c",
            "client/src/platform/crc32.c",
            "client/src/platform/module_manifest_build.c",
            "client/src/platform/module_manifest_parse.c",
            "client/src/platform/module_manifest.c",
            "client/src/platform/system_state_start.c",
            "client/src/platform/system_state.c"
        )
        Invoke-CTest "module_preload" @(
            "client/tests/module_preload_test.c",
            "client/src/platform/module_preload.c",
            "client/src/platform/module_manifest_build.c",
            "client/src/platform/module_manifest_parse.c",
            "client/src/platform/module_manifest.c",
            "client/src/platform/dos.c",
            "client/src/platform/dos_file.c",
            "client/src/platform/dos_reu.c"
        )
        Invoke-CTest "local_documents" @(
            "client/tests/local_documents_test.c",
            "client/src/buffer.c",
            "client/src/document.c",
            "client/src/charset.c",
            "client/src/platform/dos.c",
            "client/src/storage/local.c"
        )

        Write-Host "== Host UCI compile check =="
        $UciObject = Join-Path $TestDir "uci.o"
        & $HostCc.Path -std=c99 -Wall -Wextra -Iclient/src -c client/src/platform/uci.c -o $UciObject
        if ($LASTEXITCODE -ne 0) { throw "Host UCI compile check failed" }
    } finally {
        Remove-Item -Recurse -Force $TestDir -ErrorAction SilentlyContinue
    }
} else {
    Write-Host "SKIP host C tests (cc/gcc/clang not found)"
}

$Cl65 = Get-Command cl65 -ErrorAction SilentlyContinue
if ($Cl65) {
    Write-Host "== cc65 module checks =="
    New-Item -ItemType Directory -Force client/build | Out-Null
    $ModuleDir = Join-Path ([System.IO.Path]::GetTempPath()) ("see64you-cc65-" + [guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Force $ModuleDir | Out-Null
    $Cc65Path = Join-Path (Split-Path -Parent $Cl65.Path) "cc65.exe"
    $Ca65Path = Join-Path (Split-Path -Parent $Cl65.Path) "ca65.exe"
    $C64ModuleSources = @(
        "client/diagnostics/module_cache_gate.c",
        "client/src/charset.c",
        "client/src/document.c",
        "client/src/editor_format.c",
        "client/src/editor_format_link.c",
        "client/src/platform/dos.c",
        "client/src/platform/http.c",
        "client/src/publisher/destination.c",
        "client/src/publisher/state.c",
        "client/src/publisher/relay.c",
        "client/src/publisher/standalone.c",
        "client/src/publisher/standalone_documents.c",
        "client/src/publisher/wordpress.c",
        "client/src/publisher/wordpress_documents.c"
    )
    try {
        foreach ($Source in $C64ModuleSources) {
            $BaseName = [System.IO.Path]::GetFileNameWithoutExtension($Source)
            $AssemblyPath = Join-Path $ModuleDir ($BaseName + ".s")
            $ObjectName = $BaseName + ".o"
            $ObjectPath = Join-Path $ModuleDir $ObjectName
            & $Cc65Path -t c64 -Oirs -DS64_NATIVE_PETSCII=1 -I client/src -o $AssemblyPath $Source
            if ($LASTEXITCODE -ne 0) { throw "cc65 module compile failed: $Source" }
            & $Ca65Path -t c64 -o $ObjectPath $AssemblyPath
            if ($LASTEXITCODE -ne 0) { throw "ca65 module assembly failed: $Source" }
        }
    } finally {
        Remove-Item -Recurse -Force $ModuleDir -ErrorAction SilentlyContinue
    }
    $Make = Get-Command make -ErrorAction SilentlyContinue
    if ($Make) {
        Invoke-Checked "Current C64U package build" {
            & $Make.Path -C client publisher-overlay-fit
            if ($LASTEXITCODE -ne 0) { throw "Publisher overlay fit failed" }
            & $Make.Path -C client c64u-package
        }
    } else {
        throw "Current C64U package validation requires make"
    }
} else {
    Write-Host "SKIP cc65 build (cl65 not found)"
}

Write-Host "Validation passed"
