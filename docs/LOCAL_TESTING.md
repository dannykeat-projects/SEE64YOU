# SEE64YOU — current MVP validation runbook

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

### Current candidate checkpoint (2026-10-19)

Tested `codex/browser-palette-fix` at
`c723589e1fe539b44a0939db28d4b3e952851312` (includes source change
`fa89d5697febf757bb96d026dc964736817757d7`). The checkout was clean before
fast-forward from `0a42999`; no existing work was discarded. No PR currently
uses this branch as its head. Refreshed `origin/main` retains the main-only README
commit `271500e`; no reconciliation/merge was attempted.

An isolated Git archive under ignored
`client/build/mvp-acceptance-20261019-c723589/` uses cc65 V2.18 (Ubuntu 2.19),
PHP 8.5.4, Python 3.14.4 and VICE 3.9. Only shell-script line endings were
normalised in the disposable source copy. Logs, exact command/exit records,
source archive, website ZIP/checksum and discarded sizing experiments remain
there; these are local developer receipts, not approved release artifacts.

- **PASS:** focused `local_documents_test.c` compilation/run: basename suffix,
  explicit extensions and dotted parent path, ordinary extensionless Save
  origin, filename capacity, collision/second existence check, wire readback
  and completed temporary retention on rename failure.
- **PASS:** `php site/tests/run.php`, `php site/tests/browser.php`,
  `php site/tests/themes.php`, `sh site/tests/http.sh` and
  `php tools/package-site.php` (each exit 0; `results.json`).
- **PASS, limited fixture:** `python3 tools/test_basic_return_vice.py --rom-dir
  /mnt/c/site/SEE64YOU/client/build/private-vice-roms` (exit 0) verifies the
  synthetic original frame, SP/ZP/map/interrupt mask and cleared screen. It
  does not certify the unbuildable current package's entry/exit paths.
- **FAIL, historical target:** `sh tools/validate.sh` (exit 2) passes its host
  gates, including source/storage/config, UCI network, SHA/HMAC and direct
  Publisher tests, and cc65 module compile checks; it then stops at old managed
  Publisher `NETWORK_RODATA` overflow **133 bytes**. Later broad diagnostic
  gates were not run. This is distinct from the shipping failures below.
- **FAIL, shipping:** `make -C client independent-package` fails with FILE
  overlay overflow **318 bytes**. Separately,
  `make -C client independent-publisher` exits 2: BSS ends at `$C93F`,
  **704 bytes** free before `$CC00`, below the unchanged **1,024-byte** guard.
  No valid current eleven-file candidate ZIP or matching package hardware
  acceptance can be claimed.
- **Causal control:** substituting only `0a42999:client/src/storage/local.c`
  in the disposable archive makes the complete independent package and both
  manifests pass: FILE 15,341/15,360 bytes, Publisher 1,046 bytes spare.
  This is an experimental predecessor-storage package, not the current
  candidate. It was never installed on hardware.
- **Bounded attempts, not retained:** single-pass filename validation reduced
  FILE overflow to 124 bytes; additionally narrowing filename-derived counters
  (bounded to 33 bytes) reduced it to 39. Host source tests passed both, but
  neither passed package fit. Original candidate source is retained unchanged.
  Stop for go/no-go reassessment before another implementation attempt; do
  not expand loader architecture, weaken guards or start pre-acceptance cleanup.
- **BLOCKED:** exact current packaged VICE and all new C64U first-user,
  publish/download/update, Browser and restoration gates, because software
  Gate A fails. C64U was not requested on and no SD/machine writes were made.
  Physical keyboard, physical RUN/STOP and true cold restart remain unverified;
  USB/uncontrolled interruption remain deferred. Hosted Actions were not run.

Read-only publishing audit: independent Makefile source filters exclude generic
HTTP `$06`, relay, old standalone login and cross-program handoff services.
`publisher/standalone_direct.c` owns bounded HTTP/1.0 framing, streaming body
hashing, challenge requests, response parsing and socket closure;
`platform/network.c` owns UCI `$03` TCP/read/write. `publisher/direct_auth.c`
and `crypto/sha256.c` implement `see64you-direct/1` canonical HMAC.
`site/lib/DirectAuth.php` validates/consumes expiring challenges under a lock;
`site/api.php` authenticates writes; `Documents.php` validates canonical source
and writes temporary files before rename. `pair.php` requires HTTPS/password
verification for dedicated key provisioning; Settings imports exactly 64 hex
characters. No site password/bearer is sent over direct HTTP. Direct transport,
network, signing and SHA/HMAC occupy 629/410/89/224 source lines respectively;
these counts describe source complexity, not binary residency. The only proven
redundant statements were duplicate `$consume` assignments in DirectAuth; no
deletion is justified before acceptance. The managed-named launcher remains an
active shipping package dependency, so its name is not evidence of dead code.

The previously documented disposable endpoint
`http://192.168.1.111/SEE64YOU/client/build/mvp14c/a/api.php?action=challenge`
returned HTTP **200**, with no redirect (bounded curl request, no redirect
following). This checks that existing fixture host only; no production-host
HTTP/TLS claim or deployment is implied. Existing redirects remain errors in
the client; HTTPS is never silently downgraded. No new transport/authentication
architecture is justified by this audit. **First public beta: NO-GO** until a
valid matching package and the remaining automated hardware gates pass.

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
