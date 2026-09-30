# Local validation runbook

## Real C64U settings I/O probe

Build with `make -C client settings-io-probe` (also built by `c64u`).
`client/build/settings-io-probe.prg` is included in `see64you-c64u-package`
and separately published as `see64you-settings-io-probe`.

Enable Command Interface, load the PRG on C64U, RUN, then press Y.
The transaction writes only `/sd/S64IOTST.TMP` and `/sd/S64IOTST.CFG`.
The real `SEE64YOU.CFG` and `SEE64YOU.CFG.TMP` are inspected by FILE_STAT
only and are never opened, deleted, written or renamed.
Each stage waits for a key: photograph failures, raw UCI/DOS status and
reply bytes. Initial/final paths and the first failed stage are displayed.
Missing files are acceptable at deliberate inspection/absent-file checks and
delete, including explicit `FILE NOT FOUND` statuses outside the DOS mapping.
Missing required written/final files still fail. Unsupported FILE_STAT is displayed
as a failure. Failed CREATE_NEW/write/rename stops dependent work.
Delete/close failures remain visible but do not prevent the next command,
matching production's best-effort policy; final PASS requires all checks.

Success writes a valid settings record to the diagnostic final file and checks
exact read length, EOF, checksum and byte equality.
Repeat with `S64IOTST.TMP` absent and with a deliberately stale test temp present.
Write acknowledgement is separate from actual bytes verified by readback.

Production save at synchronized PR head `d231a1d` deletes stale temp,
uses CREATE_NEW `$06`, writes, closes, deletes old CFG and renames temp.
The diagnostic includes the production private open implementation so its
command bytes and status policy match exactly. Production source and chaining
are unchanged. Real settings-helper validation remains pending.

The user has now verified the file transaction on real C64U: temporary-file
write/close/rename/stat, exact 334-byte readback, zero trailing bytes and valid
content/checksum. This establishes the DOS transaction, not REU settings exchange.

## Real C64U settings exchange probe

Build `make -C client settings-exchange-probe` or `c64u`. Download
`settings-exchange-probe.prg` from `see64you-settings-exchange-probe`; it is
also included in `see64you-c64u-package`.

Enable REU (at least 128 KiB) and Command Interface, then load/RUN the PRG.
Press R to inspect the retained REU settings record before any exchange write.
This cannot recover unsaved edits that the settings helper kept only in C64 RAM.
An uninitialized/invalid retained record is reported, not silently replaced.
Press T for a known-good Standalone test URL with plain HTTP enabled; press K
at the next prompt to import `/sd/SEE64KEY.TXT` using the production importer,
or any other key to use empty keys. No network request is made.

Photograph the field classifications and store/readback screens. Keys and URL
values are never displayed. The probe reports original version/state/result,
values/record validity, URL presence/termination, HTTP flag and key states,
then applies production's READY/OK transition and runs exchange store, immediate
REU readback, checksum/validation and exact byte comparison. The old UI checksum
can legitimately be stale before store; store must rebuild it successfully.
Only after exchange success does it call the normal settings-store save path,
redirecting its filenames to `/sd/S64EXCH.TMP` and `/sd/S64EXCH.CFG`.
Real CFG/temp files remain untouched. Diagnostic files may contain imported keys;
delete those test files after recording results. The original REU settings record
is restored and verified before exit. Restart the normal app before using it.

This runbook is the handover for the current integrated See64You branch.

Use `integration-validation-2026-09-24` as the authoritative development/testing branch. The older stacked implementation PRs predate the integration branch and should not be merged into it again.

## 1. Restore the self-hosted runner

The repository transfer left current GitHub Actions jobs queued because the existing self-hosted runner is not picking up jobs from `dannykeat-projects/SEE64YOU`.

On the local runner machine:

1. Open the See64You repository on GitHub.
2. Go to **Settings → Actions → Runners**.
3. Add/re-register a self-hosted Linux x64 runner for this repository or organisation.
4. Ensure the runner has the custom label `terra64`.
5. Ensure PHP, a host C compiler, cc65 (`cl65`, `cc65`, `ca65`), Python 3, curl and a POSIX shell are available.
6. Start the runner and confirm it reports **Idle**.

The workflow requires:

```
self-hosted
Linux
X64
terra64
```

The integration workflow uses one concurrency group, so once the runner returns, the newest integration job should supersede stale queued/in-progress jobs.

## 2. Run the full repository validator locally

From the repository root:

Windows PowerShell:

```powershell
.\tools\validate.ps1
```

Unix-like shell / WSL:

```sh
./tools/validate.sh
```

Do not treat the run as complete if the cc65 stage is skipped.

The validator currently covers:

- PHP syntax;
- standalone backend, preview/archive and packaging regressions;
- standalone HTTP API lifecycle;
- WordPress protocol tests;
- secure relay tests;
- charset conversion;
- gap buffer and editor behavior;
- bounded undo/redo;
- document/wire format;
- UCI HTTP/DOS transport;
- local document storage/recovery;
- publishing destination/state;
- standalone and WordPress publisher cores;
- cc65 module checks and the full C64 build.

Fix real compiler/cc65 warnings rather than suppressing them.

## 3. Verify the current PRG

Build explicitly if needed:

```sh
make -C client clean all
```

Expected output:

```
client/build/see64you.prg
```

The GitHub integration workflow builds this PRG before validation and uploads it as the `see64you-prg` artifact. Keep that artifact path working so a current test build is always available after a successful integration run.

## 4. VICE smoke test

Run the current `see64you.prg` and check:

- continuous typing has no full-screen flicker or stale cells;
- insertion, newline, Backspace, Delete and insert/overwrite behavior;
- Home, End, Page Up/Down, word movement and preferred-column vertical movement;
- word-wrap on/off and horizontal scrolling past 40 columns;
- MARK selection, Select All, Cut/Copy/Paste and reverse-video selection;
- Ctrl+Z undoes the current bounded transaction;
- Ctrl+Shift+Z redoes it;
- consecutive ordinary typing coalesces into one undo step;
- cursor movement breaks typing coalescing;
- Find, Find Next/Previous, Replace, Replace All and Go To Line;
- formatting transformations preserve cursor/selection state;
- RUN/STOP exits normally and cancels bounded prompts;
- Shift+letter capitals, shifted punctuation and Shift Lock/Caps Lock behavior;
- no status/body drawing escapes the 40×25 layout.

Record keyboard behavior that differs from host tests instead of changing mappings by guesswork.

## Planned reusable Ultimate test infrastructure

The current VICE scripts remain valid, but new C64U-specific regression work should converge on reusable external tooling instead of adding one-off monitor scripts.

Preferred evaluation order:

1. `JC-000/c64-test-harness` for one test API spanning VICE and real Ultimate hardware. Initial See64You scenarios should cover startup, typing/navigation, settings entry, settings persistence, publisher/settings chaining and direct-publishing smoke tests.
2. `barryw/sim6502` with the `u64sim` backend for deterministic CI coverage of UCI/Ultimate-DOS state. The first regression target is the exact transactional `SEE64YOU.CFG` save sequence.
3. Real C64U hardware remains mandatory for firmware behavior, modern USB keyboard behavior, timing and any claim involving stock C64 Ultimate 1.1.

Repeated REST uploads to C64 Ultimate firmware 1.1 must account for the firmware's known `/Temp` attachment leak. The hardware runner must perform safe cleanup equivalent to the protection documented by `c64-test-harness` rather than looping uploads until the firmware wedges.

For the current settings-save failure, use an instrumented target build that reports each Ultimate-DOS step and final read-back. Do not keep adding special cases to the settings store until the failing command/status is identified on hardware.

See `docs/ENGINEERING-REFERENCES.md` for source projects, licenses and adoption boundaries.

## 5. Ultimate-DOS and recovery test

On a C64 Ultimate, verify:

- UCI interface detection;
- separate Ultimate-DOS and HTTP capability detection;
- New/Open/Save/Save As;
- overwrite confirmation;
- exact source round-trip;
- file existence, rename, delete and 512-byte write chunking;
- no stale open file after errors;
- dirty editing creates `SEE64YOU.REC`;
- restart offers recovery/discard;
- recovered text matches exactly;
- successful Save/Save As removes recovery;
- confirmed discard removes recovery;
- undoing back to the saved state does not leave a stale recovery file.

## 6. UCI HTTP/publishing test

The current code converts native PETSCII text to wire ASCII at HTTP/DOS UCI boundaries while keeping binary parts unchanged.

On target hardware verify:

- URL/header/form text reaches the HTTP target as ordinary ASCII;
- UCI textual status/string replies return correctly to native C64 text;
- HTTP handle lifecycle survives error/reset/relaunch;
- standalone trusted-LAN login works;
- standalone list/open/create/update/delete works for pages and posts;
- publisher error messages are useful and bounded.

Do **not** relax the direct-HTTPS safety gate simply because an `https://` URL parses. Current direct TLS remains unverified; remote publishing should use the trusted-LAN relay until target firmware behavior proves otherwise.

## 7. Standalone-site checks

The automated suite now covers the core standalone site heavily. A manual browser pass should still confirm:

- one-time setup;
- login failure/success;
- draft creation and publishing;
- signed one-hour draft preview links;
- preview links remain unavailable without a valid matching token;
- preview responses are no-store/noindex/no-referrer;
- newest-first post list;
- 10-post pagination;
- year archive links;
- simple/C64 themes;
- generated upload package from:

```sh
php tools/package-site.php
```

The generated `dist/see64you-site/` should contain runtime files and starter content, exclude local `config.php` and tests, and include a fresh `ENABLE_SETUP` marker.

## 8. Relay and WordPress checks

These require external services and can remain deferred until a disposable WordPress test instance is available.

For the relay verify rejection of:

- an invalid relay key;
- disallowed/traversal paths;
- redirects;
- oversized requests/responses;
- unsafe forwarded headers.

For WordPress verify:

- plugin installation/activation;
- Application Password setup;
- connection/permission test;
- page/post list and open;
- create/update/publish/unpublish/delete;
- slug collision handling;
- insufficient permissions;
- exact See64You source retention.

## 9. What remains legitimately deferred

Until the runner/VICE/C64U/WordPress environment is available, keep these TODOs open rather than guessing:

- incremental renderer runtime validation;
- undo/redo target validation;
- recovery/autosave target validation;
- final USB/C64 keyboard mapping validation;
- UCI HTTP target behavior;
- end-to-end standalone/WordPress publishing UI;
- formatting UI/menu validation and Markup v1 freeze;
- C64 preview view;
- REU-backed document/clipboard/multi-level undo;
- mouse support and conventional-C64 degradation testing.

When validation passes, update `todo.md` with the actual date/result and only then mark the relevant validation-gated items complete.
