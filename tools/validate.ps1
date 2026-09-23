$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $Root

Write-Host "== PHP syntax =="
Get-ChildItem site, wordpress -Recurse -Filter *.php | ForEach-Object {
    & php -l $_.FullName | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "PHP lint failed: $($_.FullName)" }
    Write-Host "OK $($_.FullName)"
}

Write-Host "== Standalone backend tests =="
& php site/tests/run.php
if ($LASTEXITCODE -ne 0) { throw "Standalone backend tests failed" }

$Cl65 = Get-Command cl65 -ErrorAction SilentlyContinue
if ($Cl65) {
    Write-Host "== cc65 C64 build =="
    New-Item -ItemType Directory -Force client/build | Out-Null
    & cl65 -t c64 -Oirs -I client/src -o client/build/see64you.prg client/src/main.c client/src/buffer.c client/src/clipboard.c client/src/editor.c client/src/ui.c client/src/input/input.c client/src/platform/uci.c
    if ($LASTEXITCODE -ne 0) { throw "C64 build failed" }
} else {
    Write-Host "SKIP cc65 build (cl65 not found)"
}

Write-Host "Validation passed"
