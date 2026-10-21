# SEE64YOU — current MVP validation runbook

## 8TML Editor and Publisher migration checks

The PHP source-service tests run independently:

```sh
php site/tests/eighttml.php
python3 site/tests/eighttml_http.py
```

The PHP tests cover exact source, strict input/capacity checks, atomic replacement/failure cleanup, lock failures and deterministic competing writers, including deletion before an omitted-metadata update. The HTTP test creates an isolated temporary site with ephemeral credentials, launches a local PHP server, and verifies HMAC authentication, tamper/replay refusal, exact source/metadata, source-only lists, publication-state headers, guarded PHP records (including PHP text in source), HTML escaping, draft privacy and deletion. It does not modify a configured site, deploy or certify HTTPS hosting. Existing site/browser/theme/HTTP/package tests must also pass. The web renderer remains the existing limited escaped subset; source preservation does not certify full rendering coverage.

Run native Publisher production code against that real disposable PHP service:

```sh
python3 tools/test_publisher_8tml.py --reference-tools client/build/8tml-reference/specs/tools --rom-dir client/build/private-vice-roms
```

Both arguments are optional. The host test uses the real REU buffer with simulated DMA and a POSIX-socket adapter for UCI Network `$03`; the adapter maps `example.com:80` to an ephemeral loopback PHP port. It is not evidence of native explicit-port support. Create/list/open/update/delete preserve exact source, status and unedited date/order. The maximum 61,440-byte source generates a 123,219-byte request. Invalid source and missing/malformed/duplicate status headers fail closed; injected connection failure is reported. The external validator checks actual exported files. The optional cc65/VICE probe checks PETSCII JSON escaping and BASIC return. No real C64U, installed files or configuration are touched. Real Publisher DOS/network/keyboard acceptance remains pending.

Run from the repository root under Linux/WSL with cc, cc65 and Python. Keep the supplied reference archive outside tracked source until licensing is resolved:

```sh
python3 tools/test_8tml.py --reference-tools client/build/8tml-reference/specs/tools --rom-dir client/build/private-vice-roms
make -C client independent-package
```

Both arguments are optional: without them, the script runs strict host codec/local-disk tests and the same codec tests with the actual REU buffer and a simulated DMA backend (61,440-byte capacity). `--reference-tools` invokes the supplied `8tml check -v` against generated empty and source-preservation fixtures in `client/build/`. The pinned reference is the internal 8TML 1 draft reviewed 2026-10-20, not bundled code. Unknown markup generates informational diagnostics and the exhaustive printable-character fixture deliberately produces an unclosed-emphasis warning; require zero errors. `--rom-dir` compiles the codec for the C64 with real PETSCII constants, tests under VICE and checks responsive BASIC after exit. Host tests check every chunk split; native tests sample splits to bound 6502 execution time.

The migration package builds all eleven files and both manifests. Independent Editor/Publisher use 8TML; Browser still implements the historical protocol. Source-host integration passes, but Publisher hardware acceptance and full rendering/Browser conversion remain incomplete. Do not present the package as an accepted end-to-end 8TML release.

The host firmware-1.1 direct Publisher test in `tools/validate.sh` also exercises the private request streamer with synthetic bodies at 0, 65,534, 65,535, 65,536, 122,880 and 122,881 bytes. It checks complete payloads, HMAC and Content-Length across the old 16-bit boundary, maximum decimal formatting and overflow refusal, alongside existing legacy-format/authentication/error regressions. This verifies transport counting, not native 8TML JSON serialization or real-network acceptance.

For storage-root regressions, run `python3 tools/test_storage_paths.py`. It exercises preflight, temporary-file cleanup, document origin, independent DOS contexts and journals with SD-only and USB-only mock namespaces, each in ordinary and Launcher mode, then configuration persistence and journal migration on both roots. Build an actual USB-root package with `make -B -C client STORAGE_ROOT=/Usb0/ independent-package`; select the actual mount and include its trailing slash. Always force the whole build when changing roots so cached binaries cannot retain another device. All existing memory/manifest guards apply. The hardware harness below currently targets the default `/sd/SEE64YOU/` installation; it does not certify a USB-only machine.

With explicit authorization for hardware/file changes, the automated hardware check requires idle BASIC with no program and reserved fixture names absent:

```sh
python3 tools/test_8tml_c64u.py --host C64U_IP --output client/build/NEW-8tml-hardware-receipt --reference-tools client/build/8tml-reference/specs/tools
```

It validates both manifests, builds a disposable native probe, backs up every app-folder file with hashes, captures firmware/configuration/drives/IRQ/live turbo/colours and both DOS paths, then stages/readbacks the exact package. The shipping Editor is driven by REST typing and its RUN/STOP → Save flow; reopening uses the existing recovery prompt. The separately compiled native probe uses the actual REU/codec/local-storage/UCI implementation for explicit file open/edit/save, unknown source preservation, CRLF normalization, empty source and 33-character final filenames. WRITE/RENAME faults are injected transport failures around actual SD operations, not physical disk faults. Full folder/configuration, DOS contexts and machine baseline are restored and verified afterward; failures retain receipts/backups and do not reset the machine.

This does not certify physical keyboard feel, the Editor's modifier-key Open shortcut, cold power restart, USB input or uncontrolled interruption. Keep those separate from REST-assisted real-hardware evidence. Receipt backups/configuration are private ignored build material; do not commit or upload them.

The remaining runbook describes the historical alpha validation pipeline.

**Purpose:** Reproduce and accept the **independent eleven-file C64U application package** and flat-file PHP website. This is a runbook, not a chronology. Historical managed-runtime test campaigns and one-off receipts remain in Git history. `todo.md` is the only live work queue; passing a past commit does not certify a rebuilt package.

**Codex hardware execution:** [CODEX-MVP-HARDWARE-ACCEPTANCE.md](CODEX-MVP-HARDWARE-ACCEPTANCE.md) contains the bounded autonomous sequence, fixture isolation, evidence and stop rules. Use this runbook for the detailed implementation checks; the handover does not authorise modification of user files or live sites.

## 1. Test environment and safety

- Read root `AGENTS.md`, current `todo.md` and [C64U-RUNTIME.md](C64U-RUNTIME.md) before changing runtime behaviour.
- Build with cc65; PHP site checks require PHP 8.1+; VICE checks need a working VICE installation, compatible local C64 ROMs, Python and (when headless) the project's documented display/Xvfb setup.
- Use a **disposable configured PHP site** accessible to the hardware, a **backed-up test SD installation**, and the same exact matching eleven-file package for any end-to-end acceptance. Avoid shared or production content.
- Back up existing `/sd/SEE64YOU/` files, legacy SD-root config/journals/recovery, the currently installed PRGs and the user's original C64U machine state; record file hashes, drives/IRQ, live turbo and clock where applicable. Never overwrite, remove or reset unrelated files or machine settings for a test.
- Hardware automation with REST/FTP may inject plain key presses and can reset the machine. **Do not call those physical-keyboard tests**; record the actual input mechanism and whether app-owned restoration or harness reset changed state.

## 2. Local build, PHP and host checks

From the repository root (use LF line endings for shell scripts under WSL):

```sh
make -C client independent-package
sh tools/validate.sh
php site/tests/run.php
php site/tests/browser.php
php site/tests/themes.php
sh site/tests/http.sh
php tools/package-site.php
```

The release candidate must meet existing launcher/Publisher/Settings/Browser memory guards, Editor overlay-slot protection, both manifests, native parser/source/config/integrity tests, site rendering and actual HTTP negotiation. A failure in an obsolete *managed* runtime target is not itself an independent-release failure; determine whether any shipping path still depends on it before discarding the failure.

Outputs: `client/build/independent-package/SEE64YOU/` for matching client files, and `dist/see64you-site/` for the upload-ready PHP site. Preserve the actual exit status, tested commit, warnings and SHA-256 of output ZIP/individual files; a developer artifact is **not** yet a public release.

## 3. Packaged VICE checks

```sh
python3 tools/test_independent_programs_vice.py --rom-dir LOCAL_ROM_DIRECTORY
python3 tools/test_basic_return_vice.py --rom-dir LOCAL_ROM_DIRECTORY
```

The independent-program script also builds `browser-palette-native-test` in
`client/build/browser-native-vice/` and runs it with the same ROMs. It feeds raw
ASCII palettes through the real browser parser/REU callback: all 16 values in
each colour slot, both one-byte and whole-record chunks (96 cases), malformed
digits and a text row. The same probe enters/exits Browser with green text and
requires that original colour to survive. `PALTEST.PRG` is a test fixture and
is excluded from the eleven shipping files. These native checks are necessary
because host character constants do not reproduce cc65's PETSCII mapping.

Use supported local C64 ROMs; old pinned Open ROM test material is not a substitute when conio startup differs. Require all four entry/exit paths and numeric **and string** BASIC commands. Check absent UCI/DOS/network paths, URL refusal, Settings wizard required fields/consent/cancel, browser stale-link guards and returned memory state. VICE does **not** prove UCI, physical disk/network, real key input or cold persistence.

## 4. Integrated first-user real-C64U acceptance — required for public beta

Use the actual instructions in [FIRST-PAGE.md](FIRST-PAGE.md) and the PHP site's six-step `start.php` walkthrough, not developer shortcuts. Start with a disposable, unconfigured site and test installation. For the **automated C64U acceptance gate**, drive inputs through the existing bounded key-injection harness and record the method explicitly. A native physical-keyboard usability run is a separate manual observation, not simulated by remote keys:

1. Enable C64U prerequisites, verify the 16 MiB REU and app folder, and copy the complete matching eleven-file package.
2. Configure the PHP site with `setup.php`, pair using HTTPS `pair.php`, import or manually enter the dedicated key using `S64CFG.PRG`, explicitly consent to plain HTTP publishing, save, restart/relaunch and confirm Settings persistence.
3. Launch `SEE64YOU.PRG`, create/save `/sd/SEE64YOU/FIRST.S64` and cleanly return to BASIC.
4. Fresh-launch `S64PUB.PRG`, load `FIRST.S64`, authenticate and publish a **public** page/post (not a draft). Verify normal HTML and exact text from another modern browser.
5. Fetch its canonical remote source, save to a **new** `.s64`, reopen/edit/save in Editor, and update that same remote document with Publisher. Verify canonical title/slug/type/body and that old/temporary sources are not silently overwritten.
6. Optionally browse this site from `S64WEB.PRG`; browser package acceptance additionally requires **two different independent sites** with different C64 colour palettes, v1 fallback, links/paging and clean colour restoration. Confirm unencrypted/HTTPS limitations are displayed rather than hidden.
7. Exit all programs into responsive BASIC; execute both a numerical and string BASIC command. Verify original drives/IRQ/clock/live turbo, existing SD files/config and valid CLEAN recovery state are preserved to the supported extent.

Record firmware/core versions, commit, package hashes, physical-keyboard observations, exact URLs (without credentials), visual issues, actual source files, restoring steps and each *passed / failed / blocked / unverified* gate. A success screen in a setup wizard alone is not a successful publish.

## 5. Browser and theme acceptance

Automated tests in section 2 include the public HTML path, exact `Accept: application/x-see64you-browser` v1, `Accept: application/x-see64you-browser; version=2` v2 with the required colour record, and malformed negotiation/input rejection. All site colours are selected from the 16 C64 values; legacy saved site settings are preserved.

For disposable configured theme UI, verify: 16 named swatches and arbitrary colour rejection; Plain/Simple and custom theme lifecycle; text/link contrast and visited/focus readability; layout/font/settings persistence; C64 built-in preset is no longer offered; previews don't alter published themes; no-JavaScript forms; readable fallback without webfont/CSS; 390px responsive controls and keyboard focus.

For a reproducible controlled browser hardware gate, prepare a **new disposable fixture** rather than pointing the runner at real user content:

```sh
php tools/create_browser_test_site.php client/build/browser-test-site
python3 tools/test_browser_c64u.py --host C64U_IP \
  --site-url http://WEB_HOST/SEE64YOU/client/build/browser-test-site/ \
  --output client/build/NEW-browser-hardware-receipt
```

The runner checks public home/page/archive/post navigation, paging, delay/cancellation, 404/stale links, HTTPS refusal, reopen, BASIC return and restoration. Review its `restored` result and exact backups. This existing runner does not by itself prove **two cross-domain palette configurations**; that remains a separate physical test.

## 6. File integrity and legacy migration (when affected)

- All new source/downloads, config, recovery, journal and preflight temp files live under `/sd/SEE64YOU/`; bare filenames must not resolve to same-named old SD-root files. Explicit `/sd/NAME.S64` retains its root origin. A failed open must not silently switch sources.
- Test failed `.s64` reads, existing download collisions, short HTTP response, config truncation/trailing bytes and denied rename/verification. Successful source/config must remain exact; a completed `.tmp` can be recoverable after failure. Delete-then-rename can leave the destination absent: never describe it as a guaranteed atomic replacement.
- Old root journals block Editor and require **explicit** Settings `M` then `Y` migration. Migration must verify ACTIVE machine restoration, durable CLEAN, exact retained `S64OLD*.DAT` backups and file retirement. Conflicts or unreadable state must block it. `SEE64YOU.REC` from SD root is not silently migrated/deleted; copy and compare recovery snapshots before manual action.
- Maintain bounded host and packaged VICE checks for those paths. For a dedicated guarded real-DOS negative check, use `python3 tools/build_independent_storage_faults.py` **only on a disposable backed-up SD**, after inspecting its fixture-name prerequisites. It isn't part of the release package.

## 7. Existing evidence and remaining gaps

### 8TML Editor preview distribution (22 October 2026)

The user authorised an Editor-focused alpha release. A fresh current-SD hardware run at `client/build/8tml-hardware-20261022-release-a/state.json` records `restored=true`, `active=false`, `dos_context_restored=true` and no failure: all 38 originals, configuration, DOS contexts and machine baseline verified. Editor create/save, recovery reopen/edit/resave and native invalid-input/write/rename/collision/recovery checks passed; three actual outputs passed the supplied validator with zero errors/warnings. This remains REST keyboard-buffer injection, not physical-keyboard/cold-power evidence. No shipping Editor code blocker was found by the bounded audit.

From a clean committed source tree, create fresh output using:

```sh
python3 tools/package_8tml_release.py --version v0.2.0-alpha.1 --output client/build/release-v0.2.0-alpha.1 --hardware-receipt client/build/8tml-hardware-20261022-release-a/state.json
```

The packager force-builds and validates all eleven files/manifests for `/Usb0/` and `/sd/`, leaves the default SD build selected, and refuses SD payloads differing from the hardware receipt. ZIPs include the source commit/toolchain/storage root, per-file checksums and release guide. PHP packaging uses tracked sources only, excluding local configuration, runtime data and tests. Archives are read back for exact entries/bytes and integrity; release `SHA256SUMS` covers all three ZIPs. Publish a distinct prerelease/tag only after these checks, then download uploaded assets and independently verify hashes/integrity. Preserve every historical release/tag/asset. Physical-keyboard, cold-power, USB-only and new Publisher hardware acceptance remain open; an alpha is not a passed beta gate.

Published on 22 October 2026: [v0.2.0-alpha.1](https://github.com/dannykeat-projects/SEE64YOU/releases/tag/v0.2.0-alpha.1), immutable source/tag `4394fa6bc3603b487adfdc00023a597d21b90d18`. Both complete client builds, memory guards and manifests passed; SD payloads matched the refreshed hardware receipt byte-for-byte. The three ZIPs and `SHA256SUMS` were downloaded from the final published release and matched the local originals; all ZIP integrity checks passed. The checksum-file SHA-256 is `33c74456e0ab9f666205a045132c561d03fe75721aa2fd47828384a04076e886`. Non-secret verification metadata is retained under ignored `client/build/release-v0.2.0-alpha.1/published-verification.json`. No historical release, live website or production branch was changed.

### Early-alpha distribution (2026-10-20)

The user explicitly authorised `v0.1.0-alpha.1` as an experimental GitHub
prerelease despite incomplete beta acceptance. Both ZIPs include early-version
warnings, installation instructions and the first-page checklist. The C64U ZIP
contains exactly eleven installed payload files, unchanged from `c67f19f` and
checked against `packages/c64u/BUILD.json` and both manifests. The separate PHP
installation ZIP comes from a clean Git archive through `tools/package-site.php`,
with its intentional fresh-install `ENABLE_SETUP`, starter content and hidden
access-control files; no local configuration, sessions or test suite. Archive
integrity, exact client hashes and PHP/package/HTTP checks are required before
publishing. Release notes and `SHA256SUMS` identify both asset hashes and the
tagged commit. This distribution authorisation does not mark beta gates passed.

Published release: [v0.1.0-alpha.1](https://github.com/dannykeat-projects/SEE64YOU/releases/tag/v0.1.0-alpha.1),
tagged commit `0d2d77b070f82c6e39c6ad9e40dc2edc8bf1c164`. All eight targeted
package/PHP/browser/theme/HTTP/manifest commands exited 0. Both uploaded ZIPs
were downloaded and verified against their local hashes and ZIP integrity.
Ignored build/validation/upload receipts: `client/build/release-v0.1.0-alpha.1/`.

- C64U ZIP SHA-256: `1496c42208ff137f176d80689d8a3fd365f77fe47bca2c04339a0552faf1bb3f`.
- PHP ZIP SHA-256: `968c1671a04c181129e5f7b8c8bc091c7b7a2afa1031eaa3396eeeb6b3cb1f57`.

### Current candidate checkpoint (2026-10-19)

The user authorised continuation after the `c723589e` filename implementation
failed Editor FILE fit by 318 bytes and left only 704 Publisher bytes against
the unchanged 1,024 minimum. The correction on top of `1d36ad7` combines bounded
filename validation/extension detection in one scan, uses byte-sized counters
for lengths bounded to 33, and expresses the same capacity checks as direct
threshold comparisons. Explicit extensions/paths, ordinary Save origin,
collision refusal and recoverable completed temporary files are preserved.
No transport, loader, memory guard or protocol changes are involved.

Ignored receipts: `client/build/mvp-extension-fit-20261019/`. `measurement.json`,
`validation.json` and `site-validation.json` record exact commands and exits;
`package-sha256.json` records all eleven output sizes/hashes and the verified
client ZIP. The website ZIP and its checksum are recorded separately. These are
local developer artifacts, not a public release. Environment: cc65 V2.18
(Ubuntu 2.19), PHP 8.5.4, Python 3.14.4 and VICE 3.9; shell scripts use LF in
the isolated source copy. Compatible ROMs are in `client/build/private-vice-roms/`.

- **PASS:** `make -C client independent-package`, both manifests, all unchanged
  guards. FILE 15,347/15,360 bytes; Publisher 1,040 bytes spare; Launcher 1,032;
  Editor resident BSS ends at `$8A95` before `$8C00`.
- **PASS:** focused source tests with `-Wall -Wextra -Werror`, including new
  empty-basename/line-break/299-byte overflow refusals and explicit absolute,
  colon and backslash paths with exact source reopen. Existing normal Save,
  filename-capacity, collision/second-stat and rename-recovery cases pass.
- **PASS:** both section 3 VICE commands, all four actual packaged program
  startup/refusal/exit paths, numerical/string BASIC, Settings wizard and
  native Browser palette/colour restoration. The separate BASIC-return probe
  also passes; VICE still does not prove physical UCI/network/storage.
- **PASS:** all section 2 PHP/backend/browser/theme/HTTP and site-package checks.
- **FAIL, historical target only:** `sh tools/validate.sh` exits 2 after all
  host gates and cc65 module checks pass, at managed Publisher `NETWORK_RODATA`
  overflow 133 bytes. Later broad diagnostics were not run. The independent
  shipping package above passes and has no dependency on that target.
- **PASS, bounded real C64U at `c67f19f`:** fresh disposable site setup and local
  HTTPS pairing; Settings wizard import/consent/verified save/independent
  relaunch; Editor enters basename `MV19A` through documented dirty-exit Save
  and creates `.s64` with exact canonical body; fresh Publisher loads it,
  authenticates through HMAC, publishes and produces matching ordinary HTML.
  Remote list/confirmed open, basename download, suffixed collision refusal,
  exact downloaded body/title/slug/type/published state and unchanged original
  source now pass. Numeric/string BASIC, IRQ/drives/clock/turbo pass after each
  exit. The tracked eleven-file candidate in `packages/c64u/SEE64YOU/` matches
  these binaries; Git blob hashes, checksums and both manifests validate.
- **Resolved driver failures, not product defects:** session A awaited
  `O OPEN` instead of `O=OPEN`; B consumed the fetch timeout while awaiting the
  documented replacement confirmation. Final session C waits for the selected
  title, answers Y immediately and times the fetch separately; it completes
  without failure. No product patch was needed. Native Editor reopen/edit/update
  remains **UNVERIFIED**; the user requested automated-only input for this run.
  The Browser PRG matches the earlier palette-tested binary byte-for-byte;
  Browser hardware checks were not rerun in this session.
- **PASS restoration:** all 38 original app-folder files and root inventory
  match; BASIC, original live turbo/colours, IRQ/drives and persistent machine
  configuration verify. Protected ignored receipt:
  `client/build/mvp-acceptance-20261014-4a714a4/hardware-extension-20261019c/state.json`
  records `restored=true`, `active=false`, completed automated checks and no
  failure. Sessions A/B remain beside it with their original driver failures
  and successful restoration records.
  Input was REST keyboard-buffer injection, not physical keyboard/cold power.
  USB/uncontrolled interruption remain deferred. **Beta remains NO-GO.**
- Repository is now public. The shared self-hosted job requires
  `github.event.repository.private == true`; public repositories cannot schedule
  it. No Actions were dispatched; all current validation was local.

Read-only publishing audit at `c723589e` found no redesign requirement. The
independent Makefile excludes generic HTTP `$06`, relay, old login and handoff
services; the managed-named Launcher remains an active bootstrap dependency.
`standalone_direct.c` owns bounded HTTP/1.0, streaming hashes, response parsing
and socket cleanup; `platform/network.c` owns UCI `$03` TCP. `direct_auth.c` /
`crypto/sha256.c` match PHP `DirectAuth.php`'s expiring single-use challenge/HMAC.
`api.php` authenticates writes; `Documents.php` validates source and uses
write-temporary/rename. HTTPS/password-gated `pair.php` provisions the dedicated
key; Settings validates 64 imported hex characters. No reusable site password
or bearer goes over direct HTTP. Direct transport/network/signing/SHA source
counts were 629/410/89/224 lines; they are not residency measurements. Only
harmless duplicate `$consume` assignments were proven redundant; cleanup waits.
The documented disposable challenge endpoint at
`http://192.168.1.111/SEE64YOU/client/build/mvp14c/a/api.php?action=challenge`
answered 200 without redirect. This does not certify production hosting or TLS.
Refreshed `main` retains README-only commit `271500e`; no open PR uses this
branch as head. No merge, deployment or release was attempted.

Current guided hardware receipt: `client/build/mvp-acceptance-20261014-4a714a4/hardware-guided-5/`.
`continuation-state.json` records native Editor create/save/reopen and downloaded
edit/save, automated fresh Publisher auth/publish/download/update, exact source
and metadata, existing-destination refusal and actual browser HTML changing from
`test` to `updated test`. Numerical/string BASIC, IRQ/drives/clock/turbo pass.
`restoration-final.json` verifies all 39 original app-folder files and machine
baseline after recovering an interrupted FTP readback check. Original installation
is restored; candidate is not staged. Physical wizard/cold power-cycle and full
native Publisher input are not inferred from this mixed-input run.

Browser resume fixes the captured `site-b-wire.txt` failure: cc65's native
`A`/`F` constants differ from wire ASCII, so palette validation/decoding uses
numeric ASCII values. The integrated native regression fails on original code
and passes on the fix. Capture original text before changing it to white; the
previous order restored white rather than the caller's text on normal exit.
The green-text VICE check and real CPU-executed colour checks cover restoration.
Host/PHP/HTTP tests, complete package/manifests/guards and packaged VICE pass.
Browser headroom is recorded in `receipts/palette-package.log`; only its PRG
changes among the eleven package files. Fixed candidate ZIP and exact hashes
are under the same ignored receipt root in `palette-package-verification.json`.
`hardware-browser-palette-7/state.json` records passing cross-site `01D`, paging,
legacy v1, error/cancel/reopen, BASIC/IRQ/drives/clock/turbo and live colour parity.
User confirmed both physical palettes on 2026-10-14. The final physical exit
check found Browser still open, so native RUN/STOP exit remains unverified;
the earlier automated exit passed. Cleanup restored original Browser/config and
machine baseline (`active=false`, `restored=true`); the receipt retains the final
exit-check failure. Session 6 also restored originals after the text-colour defect.

### Pinned software gate (2026-10-14)

Clean Git archive `4a714a4c5c0a931003ff57c57bac78942f313632` built with cc65
V2.18 (Ubuntu package 2.19), PHP 8.5.4 and VICE 3.9. Complete independent
package, both manifests and existing guards pass: Publisher 1,046 B, Launcher
1,032 B, Settings 4,009 B and Browser 17,235 B spare; Editor resident BSS ends
at `$8A95` before `$8C00`. All eleven output files match hosted CI run
[38372951631](https://github.com/dannykeat-projects/SEE64YOU/actions/runs/38372951631)
and its client ZIP byte-for-byte. Both ZIP integrity checks, exclusions and
published hashes verify:

- Client ZIP SHA-256: `632ab64140494459bc71898bad183f62304765d0d70bddb95e40a47803bdf444`.
- Website ZIP SHA-256: `0f2af35bcc02dd500fcc4374057e03e004411641eb72ad1d4a5443ec04f5fe3c`.

The section 2 commands ran in an isolated LF-shell-script copy. Initial shell
failures were CRLF invocation errors; corrected runs pass PHP/backend,
browser/theme, HTTP and site packaging checks. Broad host validation initially
found a missing `COLOR_WHITE` in `ui_prompt_stubs/conio.h`; the bounded stub fix
allows every host gate and cc65 module compile check to pass. The broad validator
then fails on the historical managed Publisher overlay's `NETWORK_RODATA`
overflow of 133 B. `independent-package` does not depend on that target; its
full shipping build passes. Subsequent broad diagnostic gates were not executed.
Both section 3 VICE commands pass using the local compatible ROMs, including
all four program entry/exit paths, Settings wizard/refusal cases, browser URL
guards and numerical/string BASIC commands. No shipping source changed.

Ignored receipts and downloaded CI archives:
`client/build/mvp-acceptance-20261014-4a714a4/`. Read-only C64U observation found
idle BASIC and firmware 1.1.0/FPGA 122/core 1.49, with existing user config/key,
recovery and journals. The user subsequently authorised changes to everything
inside `/sd/SEE64YOU/` for the backed-up disposable workflow. Hardware outcomes
are recorded separately; physical input and cold power-cycle gates are not
inferred from automated key input or reset/relaunch.

### Fresh setup and input-driver boundary (2026-10-14)

After explicit user authorisation to change the app folder, complete recursive
SD backups passed exact hashes. The candidate's eleven installed files passed
readback. Two new disposable site instances were configured using normal
`setup.php` forms; site A's dedicated key came from `pair.php` over an actual
local TLS connection (temporary self-signed certificate, test trust exception).
Fresh independent Settings imported it, required explicit HTTP consent, saved
with verification, returned to numerical/string BASIC and independently
reloaded/cancelled without changing the saved config. IRQ, drives, clock and
CPU-executed live turbo 0 passed. Neither reset/relaunch nor injected keys count
as physical cold-start or keyboard acceptance.

The Editor displayed the entered public test text, but the end-to-end driver
did not complete saving. Attempt one injected during launcher preflight;
attempt two correctly waited for the Editor menu but injected byte 19 without
the required Control modifier, so the input decoder correctly treated it as
Home. This is a driver failure, not evidence that physical Ctrl+S is broken.
The reviewed source requires the Control bit in `$028D` for those shortcuts;
this firmware returned 404 for `GET /v1/machine:input`, the documented native
[REST chord endpoint](https://github.com/GideonZ/1541u-documentation/blob/master/api/api_calls.rst).
No modifier-forcing or IRQ patch was attempted. Further automated Editor work
requires a proven supported chord mechanism, or a separately labelled human
Editor segment. Publication/download/update, two-site palette/navigation and
current-package broader hardware negatives remain unverified.

Both bounded sessions returned to BASIC and restored the original complete app
folder byte-for-byte, preserved root inventory, and verified IRQ/drives/live
turbo/screen colours with working number/string BASIC and no emergency reset.
Receipts: `hardware-first-user-2/state.json` and
`hardware-first-user-3/state.json` under the ignored software receipt directory;
both record `restored=true`, `active=false` and the precise driver failure.
An initial TLS setup attempt rejected the old local certificate as too weak
before any SD writes; a temporary RSA-2048 test certificate resolved it. A
separate proposed config copy for continuation was rejected by automatic
approval review and was not made or indirectly reused. Private backups remain
in the access-denied receipt directory; secrets are absent from these docs.

- Independent Editor/Publisher/Settings package: three bounded functional edit/publish/download/update real-C64U round trips on 2026-10-10/11; exact metadata/body, truncated-response rejection, storage/config fault handling, BASIC and turbo 0/15 restoration checked. These were not three fresh-install manual sessions.
- Public browser: separate bounded real-C64U result on 2026-10-11 (`client/build/hardware-browser-final-20261011/state.json`), nine checks passed, original SD restored and IRQ/drives/turbo 0 parity recorded.
- SD-folder package install/journal migration and settings first-run/import/consent/readback were hardware tested in scoped 2026-10-11 receipts. The physical wizard key path plus complete cold first-publication walkthrough remain **unverified**.
- Fixed v2 palette/browser candidate passes PHP/host/HTTP/cc65 and packaged VICE checks on 2026-10-14. Two-site real-C64U colours are user-confirmed; navigation, v1 fallback, automated exit and original colour restoration pass. Final native RUN/STOP exit remains **unverified**. Original Browser/config and machine baseline are restored.
- USB keyboard acceptance and uncontrolled SD removal/power interruption are user-deferred; do not claim them passed.

Do not ship solely from prior recorded outcomes. Gate public artifacts on section 4, relevant section 5/6 checks and the **exact** current release binary identities. Archive old detailed receipt narratives in Git history rather than duplicating them here.
