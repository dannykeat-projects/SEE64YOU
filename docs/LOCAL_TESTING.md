# Local validation runbook

This file contains the **current** reproducible validation path for the C64 Ultimate-only direction. Historical conventional-C64/IEC investigations remain in Git history.

The authoritative outstanding-work queue is todo.md.

## 1. Product test baseline

Target machine configuration:

- supported C64 Ultimate;
- Command Interface enabled;
- REU enabled;
- **REU size 16 MiB**;
- package on SD;
- firmware 1.1 is the first real-hardware baseline.

Software IEC/device 8 is not a product requirement. It may still be present temporarily for regression-testing old migration code.

## 2. Full repository validation

Integration CI uses the private-projects `terra64-wsl` self-hosted runner with
labels `[self-hosted, linux, x64, terra64]`. The workflow verifies the preinstalled
toolchain and VICE version; it does not install packages or use sudo. Its triggers
remain feature-branch pushes and explicit dispatch, without fork PR execution.
CI builds and publishes the managed c64u-package directory, validates both
manifests/memory gates, and keeps reusable host/VICE diagnostic coverage. Retired
conventional-C64 and old standalone-editor fit gates are excluded.

The full integration run passed for implementation commit d5ed796:
https://github.com/dannykeat-projects/SEE64YOU/actions/runs/37176770706
The downloaded see64you-c64u-package artifact contains exactly nine files, each
matching the locally validated managed package by SHA256. This CI result covers
software/build/VICE gates; physical keyboard and broader hardware acceptance
remain separate.

Windows PowerShell:

~~~powershell
.\tools\validate.ps1
~~~

WSL/Linux:

~~~sh
./tools/validate.sh
~~~

During migration some scripts still build conventional-C64 artifacts/headroom checks. Those are temporary regression checks, not product acceptance criteria. Remove them as the C64U-only code paths replace the old implementation.

Do not suppress genuine warnings/errors.

Focused editor overlay gates (installed cc65, Python, VICE 3.9 and Xvfb):

~~~sh
python3 tools/tests/editor_overlay_fit_test.py
make -C client editor-overlay-fit
make -C client code-assets-fit
make -C client managed-package
python3 tools/check_memory_headroom.py client/build/code-assets-fit.map --label code-assets-fit --min-headroom 1024
python3 tools/test_code_overlay_vice.py --rom-dir /tmp/see64you-roms
~~~

Use the pinned Open ROMs and preparation procedure in `tools/test_reu_vice.sh`
for that ROM directory, or supply an existing directory containing `kernal.rom`,
`basic.rom` and `chargen.rom`. The full VICE script runs this ABI fixture after
preparing ROMs. Host cache-selector tests run in both validation scripts.
The fit output is `client/build/editor-overlay-fit/`, including the PRG, map
and three code binaries. The build also creates canonical code-asset copies
and generates/revalidates `S64OVL.MNF` against that exact root and those copies.
`code-assets-fit` enables the diagnostic launcher asset-preload/readiness path;
it stops before executable transfer. `managed-package` assembles and validates
the coherent preload package in `client/build/managed-package/`, including
SEE64YOU.PRG, S64EDIT/PUB/CFG.PRG, both manifests and the three code assets.
The ordinary launcher and public product targets remain unchanged.
Require all groups to fit the 15 KiB execution slot,
resident BSS below `$8C00`, and the reserved 1 KiB gap before the stack.
The VICE fixture tests copied code and cc65 ABI with an injected reader; it
does not validate production REU preload, DMA timing or C64U editor behavior.
Do not launch the managed editor directly or treat this diagnostic as a product
release. Its entry requires a trusted copier arrival from verified caches.

Both validation scripts include `code_assets_test.c` and
`tools/tests/code_asset_manifest_test.py`. They cover manifest/root boundaries,
atomic reservation rollback, exact owners/flags, native preload/size/close/DMA
failures, full-cache CRC mismatch, readback/publication failure, stale/CLEAN
journal rejection, unchanged failed outputs and trusted-arrival-only binding.
The module integrity regression suite also runs against the shared CRC updater.
None of these host tests establishes real UCI/REU timing or production entry.

## 3. Current C64U package build

~~~sh
make -C client c64u-package
~~~

Current upload files are the nine files in client/build/c64u-package/:

- SEE64YOU.PRG: verified executable launcher with original SYS-context capture;
- S64EDIT.PRG: managed editor root requiring trusted arrival;
- S64PUB.PRG and S64CFG.PRG: cached companions, managed transitions still blocked;
- S64MODULE.MNF: exact PRG lengths/entry points/CRC32;
- S64OVL.MNF: exact editor-root and raw-asset binding;
- S64MODEL.BIN, S64FILE.BIN and S64RUN.BIN: REU-cached editor code assets.

The target assembles the hardware-proven managed-handoff package into a separate
product directory, then revalidates both manifests against the copied bytes.
Launcher, resident editor/overlay slot, Publisher and Settings retain their
existing memory gates. Do not mix these files with standalone editor builds.

Copy all nine files to /sd/ and run SEE64YOU.PRG. Current storage still uses /sd/;
application-folder migration is queued. The standalone make launcher target and
managed-package target remain gated preload diagnostics. The current product
target executes the verified cached handoff and clean original SYS return.

Physical keyboard acceptance remains required on both USB and native keyboards:
New, type punctuation and multiple lines, Save As a unique scratch filename,
New, reopen that file and compare exact text, navigate File/Edit/Search menus,
then Exit with dirty/discard handling. Confirm responsive BASIC and newest CLEAN.
Use disposable source files; publishing transitions remain blocked.


## 4. Hardware access from the development PC

Ultimate64 Manager is installed on the existing Windows development machine. Read the current host/profile from its local configuration rather than assuming an address remains unchanged, and never print/commit stored passwords.

REST/FTP can be used for authorized upload/run/readback testing. Preserve unsaved work before remotely replacing the running PRG.

Read `/v1/drives` before a hardware test and preserve the user's drive settings.
The launcher preflight/cache path uses native Ultimate-DOS/REU and does not
require Drive A/B or Software IEC to be disabled. Change a drive setting only
for a verified test requirement, capture it first and restore it afterward.

Downloaded SEE64YOU.CFG contains secrets. Keep test copies under ignored build output and never paste key material into logs or PR descriptions.

A REST reset/reboot does not prove physical power-cycle persistence.

## 5. Established firmware-1.1 evidence

Do not repeatedly rerun retired incident diagnostics unless a relevant regression appears.

Already passed on real hardware:

- UCI $DF1C-$DF1F;
- Ultimate-DOS $01;
- Network $03 basic DNS/TCP/raw HTTP;
- HTTP Target $06 absence on firmware 1.1;
- settings save/readback;
- full physical power-cycle settings reload;
- authenticated direct GET from persisted key;
- profile save/load;
- exact large direct document upload/reopen;
- companion-stream-probe for all matching Editor/Publisher/Settings PRGs;
- companion-launch-probe replacement load, cc65 startup, target execution and return.

The last result closes the old resident-loader proof gate. The $C800-$CBFF loader is not the new production architecture.

## 6. New C64U runtime gates

As the new architecture lands, validate these in order.

### 6.1 Platform/16 MiB requirement

From a cold launch:

- detect missing/undersized REU cleanly;
- show exact user setup guidance;
- recheck without restarting the entire wizard;
- positively verify access near the intended high-REU range without corrupting existing state;
- verify /sd/ can create, write, reopen and exactly read a temporary marker;
- verify the preflight temporary file is removed and no pre-existing S64CHK?.TMP candidate is overwritten.

### 6.2 System-state capture/restore

Record the machine state See64You intends to change.

For each controllable setting:

1. note starting value;
2. launch See64You;
3. trigger each relevant performance/runtime mode;
4. cleanly exit;
5. verify the exact original value returned.

Repeat from at least two different starting configurations.

### 6.3 Interrupted-session journal

The journal format/two-slot selection logic, interrupted-session restore/verify/CLEAN resolution and launcher startup policy have host coverage. NONE/CLEAN starts silently; valid INTERRUPTED state is automatically restored; retryable failures remain ACTIVE and return RETRY rather than pretending recovery completed. On real hardware, once launcher integration lands:

1. launch and confirm an ACTIVE journal is durably verified before the first temporary global change;
2. force/reset while that state is active;
3. relaunch and verify startup classification reports INTERRUPTED for the newest valid ACTIVE record, CLEAN for a clean record, NONE when neither slot exists, and a distinct error for invalid/unreadable storage;
4. verify only settings whose journal restore-valid bits are present are exposed in the recovery snapshot;
5. restore only those explicitly captured settings;
6. complete recovery/clean exit and verify a newer CLEAN record supersedes the old ACTIVE slot;
7. corrupt/interfere with the newest slot in a controlled test and verify the older valid slot is selected rather than silently inventing state.

Do not claim arbitrary machine-state restoration beyond values See64You explicitly captures.

### 6.4 DOS-to-REU module load

Build the preload-only diagnostic with make -C client managed-package and keep
all nine matching files from client/build/managed-package/
together in `/sd/`. Run the launcher with an exact 16 MiB REU. Expect
`VERIFYING MODULES...`, `MODULE CACHE VERIFIED`, `SHARED SESSION READY`,
`EDITOR HANDOFF PENDING`, then `SESSION CLEAN`.
This verifies cached-file CRC32 but does not launch the editor. Test missing/mismatched
module files and choose X: machine settings must restore before CLEAN. If restore
or journal cleanup fails, ACTIVE must remain recoverable. Use scratch package
copies and preserve the user's existing configuration/journals. Also change one
byte in a scratch module PRG while retaining its size and the original manifest:
expect MODULE INTEGRITY FAILED, never MODULE CACHE VERIFIED. Choosing X must
restore/clean the session; restore the matching package before retrying. Record
verification time and check that unrelated REU allocations are preserved. DMA
must remain IO_SAFE while CRC chunks use managed CPU_INTENSIVE. Record the
starting and final turbo byte, including corruption/exit paths. Firmware-1.1
startup through CLEAN measured 12.4 seconds from captured turbo byte 0 after
chunk acceleration. Captured turbo byte 2 and sampled unrelated-region
preservation also passed with the diagnostic below.

For the reproducible preservation/second-turbo gate, build
`make -C client module-cache-gate`. Upload `build/module-cache-gate.prg` alongside
the matching package to a disposable name in `/sd/` and run it. This wraps the
actual launcher, backs up and seeds 17 separate 16-byte REU sentinel blocks
outside the runtime/cache range, then selects speed nibble 2 through an outer
diagnostic system-state manager. Expect the normal verified launcher/CLEAN path,
`ADOPTION PASS`, `CACHE GATE PASS` and
`START <prior> INNER <prior-upper-bits|2> FINAL <prior>`.
The adoption stage constructs a second active session through the real platform
APIs, initializes an independent destination, adopts the verified source journal
without destination DOS calls, accelerates temporarily and exits from that owner.
It does not overwrite executable code or enter the editor.
The launcher clears/readbacks shared readiness before restore/CLEAN.
The diagnostic also reloads the shared allocator control and newest DOS journal after
launcher exit and requires the metadata load API to reject the CLEAN session
and a simulated leftover ACTIVE capsule, proving exit revoked the REU record.
It checks preservation, restores/readbacks all original sentinel bytes and
restores/verifies the outer turbo state. Independently read the two journal
slots: the newer CLEAN must contain the inner captured turbo byte. Record the
original turbo byte before the run so it can be restored if the diagnostic is
interrupted. Remove only the uploaded diagnostic afterward; keep its local PRG
and map. The sample immediately beyond runtime-owned memory is now 0x050240, after
the allocator-owned session record and transfer capsule; update the probe when ownership changes.
This checks sampled preservation, not all unrelated REU bytes.

Using the required 16 MiB configuration:

- open a known module through Ultimate-DOS;
- load to an assigned REU cache region using DOS_CMD_LOAD_REU;
- verify exact byte count/status;
- verify integrity against the source;
- verify other REU allocations remain unchanged.

### Managed Publisher fit and ownership preparation

Run make -C client publisher-overlay-fit for the managed destination layout.
It compiles actual trusted Publisher entry, ACTIVE adoption, S64Q assets and shared
application-state consumption with transition/return support. Require CONTENT
13,697 / NETWORK 11,765 / RUNTIME 13,921 bytes within the 15,360-byte slot, resident
BSS $866D and 1,426-byte headroom (minimum 1,024). It generates/revalidates the exact
S64POVL.MNF and Publisher assets. Initial control load/validation must be resident;
only the explicit tested return-bridge allowlist may cross overlay groups.

Full tools/validate.sh or tools/validate.ps1 runs application_state_test.c and
managed_publisher_test.c plus both asset namespace suites and overlay-builder tests.
The state suite checks every-byte corruption, reserved/range rejection, exact
ACTIVE binding, CLEAN/stale/wrong-session/package rejection, owner/flag/alignment,
unchanged failed outputs, verified store/readback, allocator pressure, real history
flush/undo/redo, no-op round trips and replacement preserving clipboard. Publisher
checks adoption/binding/state failure ordering, unchanged failed document outputs
and one-shot decode authority. These are host contracts, not hardware execution.

This Publisher diagnostic must not be executed or deployed. The launcher reserves
the application-state allocation, but the publication caller, Publisher asset preload and
validated session settings remain unintegrated. Publisher/Settings transitions
stay gated. The current nine-file c64u-package retains Launcher -> Editor -> BASIC.

Both validation scripts also run `managed_editor_state_test.c`: clear validity
requires fresh entry without reading the state record; set validity requires exact
ACTIVE authentication and one-shot decode. Missing allocation/read failure,
every-byte corruption and journal/session/package/snapshot mismatches reject
resumption. The suite checks all decoded controls, filename/search/clipboard,
generation/source/replacement, usable undo/redo and unchanged REU payload bytes.
It also exercises fresh/replacement publication, verified authority revocation
before overwrite, complete state/control readback, final-bit publication and
authenticated resume round trips. Faults cover pending/failed history flush,
capture, initial clear write/readback, state encode/write/last-chunk readback or
corruption, and final control write/readback/corruption including failed cleanup.
Failures retain caller generation, leave the current Editor usable and disallow
transition. Runtime-control tests corrupt every one of the 192 readback bytes.
Both validators separately cc65/ca65-compile the gated producer service and
capture/encoder; they are not linked into the current Editor root. Exit tests
bind the real revocation service and exit implementation to one host return
workspace, check both clear/set flags and block CLEAN on failed revocation.
Decoded scratch lives in the second half of the asserted 1 KiB `$C800` gap.
`test_code_overlay_vice.py` covers the nested MODEL -> FILE -> MODEL return path
with 16/32-bit arguments/results and cache-read counts. Current Editor resident
BSS ends at `$8BE5` (26 bytes before `$8C00`); MODEL 15,216 / FILE 15,341 /
RUNTIME 15,225 bytes fit the 15,360-byte slot. The original workspace/stack and
slot limits remain mandatory. The preceding consumer package passed the bounded real-C64U
first-entry/edit/CLEAN/BASIC-return gate on 2026-10-05 (details below). Physical
keyboard/file/menu and actual resumed-entry hardware acceptance remain outstanding.

Full host validation includes module_transition_test.c for ordering, cancellation,
invalid ownership, failed mode/write/readback/load/plan and recovery-required
paths. module_runtime_test.c also repeats three complete four-way ownership
cycles using real capsule/shared-session REU record code. These prove session
continuity without recapture/ACTIVE rewrites; they do not execute modules or prove
hardware editor/publishing state preservation.

### 6.5 Cached module transition

Current software gate:

1. run the full validation suite and require session/capsule/transfer/return tests;
2. build make -C client c64u-package;
3. require the launcher, resident-editor, overlay-slot and companion memory gates;
4. require both manifests to validate the exact copied package bytes;
5. do not run a mixed or failed package.

The staged execution path uses a transient stub copied to screen RAM at $0700
with parameters at $07D0. It DMA-copies the cached PRG **body** (cache address +
2, excluding the two-byte PRG header) to the manifest load address, then writes
a one-shot arrival marker at $02A7-$02A9 and jumps to the validated startup
address. The destination must consume/clear that marker before adopting the
ACTIVE session. REU state alone must never authorize a cold/direct module launch.

For repeatable real firmware-1.1 acceptance, use a matching disposable package.
The bounded entry/edit/clean original SYS return has passed; physical keyboard
and broader file/menu acceptance remain outstanding:

1. start SEE64YOU.PRG and observe normal preload/verification;
2. require direct cached Editor entry rather than `EDITOR HANDOFF PENDING`;
3. verify no fresh SD read or software-IEC/KERNAL LOAD occurs during the handoff;
4. verify the Editor consumes the arrival proof, adopts the exact ACTIVE journal/shared descriptor and resumes INTERACTIVE without recapturing or rewriting the original turbo snapshot;
5. perform a simple edit/input smoke test;
6. exit through the managed owner and verify capsule + shared readiness are revoked, the original machine state is restored/verified, and a newer durable CLEAN journal is written;
7. reset/interfere at bounded points and confirm failures leave ACTIVE recoverable rather than silently claiming CLEAN.

The proven Launcher → Editor path can now be extended in bounded stages through:

- Editor → Publisher;
- Publisher → Editor;
- Publisher → Settings;
- Settings → Publisher;
- repeated full cycles.

After initial module preload, transitions must not depend on software IEC or a fresh SD load.

Verify document, cursor/view state, selection, history, clipboard, settings/profile state and dirty/recovery state.

## 7. Direct Standalone publishing

Current firmware-1.1 baseline uses Network Target $03.

Validate:

- explicit plaintext-HTTP consent;
- direct-key authentication;
- page/post list/open when implemented;
- create/update;
- draft/publish/unpublish;
- delete;
- exact canonical source reopen;
- network failure returning safely to local editing.

Direct mode rejects https:// unless an actual TLS-capable path is selected.

## 8. Local storage

Validate:

- New/Open/Save/Save As;
- overwrite confirmation;
- exact canonical reopen;
- temp-file cleanup;
- no stale DOS handles after failure/cancel;
- deterministic path setup from cold start.

For configuration:

1. save settings;
2. require immediate final-file readback/validation;
3. physically power cycle;
4. launch without browsing/import side effects;
5. verify settings auto-load;
6. test authentication.

When configuration eventually migrates into /sd/SEE64YOU/, test rollback from the proven old /sd/SEE64YOU.CFG path.

## 9. Recovery/autosave

Validate:

- dirty edit creates/refreshes durable recovery;
- restart offers Recover/Discard;
- recovered source remains dirty;
- Save/Save As clears stale recovery;
- deliberate discard clears stale recovery;
- REU fast recovery state and SD durable recovery do not conflict.

## 10. Keyboard/editor acceptance

At 40x25 validate both the C64U's native C64 keyboard behaviour and modern USB keyboard behaviour:

- continuous typing;
- insert/overwrite;
- Backspace/forward Delete;
- cursor visibility;
- Home/End/Page movement where distinct keys reach software;
- word movement;
- wrap/horizontal scrolling;
- MARK selection;
- Select All/Cut/Copy/Paste;
- undo/redo;
- search/replace;
- Go To Line;
- markup formatting;
- shifted punctuation/lock behaviour.

Record firmware and keyboard type for hardware-specific failures.

## 11. Large-document/REU acceptance

The new design is no longer bounded by the old 128 KiB/16-bit assumptions.

As allocator/paged-document work lands, test progressively larger documents and verify:

- responsive navigation/editing at start/middle/end;
- search/replace;
- large clipboard;
- deep history;
- save/reopen;
- recovery;
- publisher transitions;
- controlled memory-pressure reclamation.

Never accept silent loss of canonical source under memory pressure.

## 12. Setup wizard acceptance

On a fresh configuration:

- missing prerequisites show exact instructions;
- RECHECK works;
- site URL/key setup does not expose UCI jargon;
- persistent save is reopened/verified;
- offline editing is available when only networking is missing;
- subsequent launch skips setup;
- Advanced/Diagnostics expose detailed hardware state without making it mandatory.

## 13. Standalone site browser acceptance

Run `php site/tests/themes.php` for custom-theme lifecycle, validation/injection rejection, preview non-persistence, secret preservation and built-in protection; this is also included by `site/tests/run.php`.

Theme UI checklist (use a disposable configured site):

1. Open Themes, test wrong/correct site password, then create a C64 copy. Change name, website title, title/menu/header-line visibility, boxed/open content layout, font, colours, spacing, border and menu layout; preview must not change the public site until saved/selected. For stacked navigation, links must appear one per line without stray pipe separators.
2. Save/reopen, rename and duplicate; select the copy and check Home/Page/Posts. Delete an inactive copy, then the active copy: only active deletion returns to Plain. Built-ins remain selectable.
3. Preview C64 text, punctuation, numbers, menu/input/button colours and keyboard focus. Block `assets/fonts/c64.ttf`: readable Courier/monospace must remain. Compare glyph appearance with a C64 screen; exact glyph/metrics acceptance remains manual.
4. Disable JavaScript before login and repeat create/preview/save/select/delete. Test rejected CSS-like values/low contrast; saved theme remains intact and entered form values remain available.
5. At 390px wide check labels, fields, menu, sample text and public content. Use keyboard-only navigation and verify readable focus.
6. On an available legacy browser, check GET/POST forms with CSS/font enhancements ignored. Alternatively document a fallback run with CSS and webfont loading disabled: Plain, login and all actions remain usable. This fallback does not certify a specific historic browser/TLS implementation.

When site-facing code changes, manually confirm:

- one-time setup;
- login failure/success;
- page/post draft/publish;
- signed draft preview;
- listings/archives;
- plain/simple/C64 themes;
- keyboard readability;
- generated upload package.

## 14. Diagnostic PRGs

Diagnostic PRGs are developer regression tools, not normal user workflow.

The old companion stream/launch probes are now **completed architecture evidence**. Keep them while useful for regression, but do not treat them as required steps for every build.

New diagnostics should prefer an integrated diagnostics screen/export plus reusable hardware automation over proliferating one-off PRGs.

## 15. Hardware automation

Converge on:

- JC-000/c64-test-harness for VICE/real-C64U smoke tests;
- barryw/sim6502 / u64sim for deterministic UCI/DOS state-machine tests;
- a developer upload/run command for the current C64U package;
- real C64U as final authority.

Firmware-1.1 REST quirks documented by the external harness still apply to automation.

The asset host suite also checks the resident bootstrap prerequisite: rejected cold arrival/version/length mismatch and DMA failure invalidate the selector; successful selection copies RUNTIME without SD access or turbo changes. Restore the pinned ROM fixture described by `tools/test_reu_vice.sh` when the temporary ROM directory is absent before rerunning the VICE command.

The managed editor host suite checks adoption failure gates, asset revocation before restore/CLEAN, and retry after failed CLEAN persistence without reopening an already-restored I/O scope. The managed overlay-fit root rejects a direct cold launch with `Managed entry failed. Restart launcher.`; only a verified launcher copier arrival may start this diagnostic. Managed publisher transitions remain blocked pending ownership integration.

For the executable diagnostic, build `make -C client managed-handoff-package`.
This replaces only the diagnostic package's launcher with an executable-transfer
variant; c64u-package uses the same verified executable bytes. Preserve existing SD companions,
manifest and `SEE64YOU.REC`, then upload matching package companions, manifests
and raw assets. Run its launcher from a disposable PRG path. Wait for managed
editor entry before injecting input. Decline recovery, type a short string and
verify it appears in the document, then RUN/STOP and confirm discard. Verify
the newest checksummed journal is CLEAN and BASIC returns without a reset. Compare
the IRQ vector with its pre-launch value, verify the KERNAL clock advances, and
inject a short BASIC command such as PRINT 6*7; require 42 followed by READY.
Screen-RAM staging may leave binary characters above BASIC's prompt. Restore
preserved files with exact readback parity, remove only newly created diagnostic
files and compare drive settings. Keep the new CLEAN journals. This integrated
capture/edit/return gate passed on firmware 1.1.0 on 2026-10-04, CLEAN sequence
56/package 513, original IRQ $EA31. REST memory DMA readback of I/O addresses is
RAM-shadow evidence, not a live turbo-register check. Missing/corrupt return
context intentionally retains the CLEAN resident reset message.


On 2026-10-04, the exact CI-passing cb4ebfe package (0x0202) ran on the C64U
at 192.168.1.112. Managed Editor entry, declined autosave recovery, visible TEST
edit and RUN/STOP/discard returned to BASIC. Both journal checksums passed;
newest was CLEAN sequence 58/package 514, older ACTIVE 57/package 514. The IRQ
vector matched the pre-launch bytes, the KERNAL clock advanced and PRINT 6*7
produced 42 followed by READY. Existing SD companions, module manifest and
SEE64YOU.REC were restored with exact SHA256 readback parity; only new raw
assets/asset manifest and /Temp/S64R514.PRG were removed. Drive settings were
unchanged. Evidence is in ignored client/build/hardware-package514-backup/.
This revalidates the expanded allocator package's short automated hardware gate;
physical keyboard/file/menu acceptance and Publisher cycles remain unverified.

Run python3 tools/test_basic_return_vice.py in the configured Linux/WSL cc65,
VICE/Xvfb environment with the pinned Open ROM fixture at /tmp/see64you-roms.
It uses a synthetic SYS frame to check full stack/zero-page restoration, SP,
memory map and flags; actual launcher capture is covered by the hardware gate.
The host validation scripts also run basic_return_test.c,
launcher_basic_return_test.c and managed_editor_exit_test.c for allocator,
session/integrity, publication/revocation and shutdown-order failure coverage.

For the managed preload hardware gate, back up existing SD S64EDIT/PUB/CFG.PRG and S64MODULE.MNF and both journals into ignored build output. Upload the package's modules, manifests and raw code assets to /sd/; run its launcher as a temporary PRG. Require PLATFORM READY, MODULE CACHE VERIFIED, SHARED SESSION READY, TRANSFER CAPSULE READY, TRANSFER PLAN READY, EDITOR HANDOFF PENDING and SESSION CLEAN. Verify turbo equals its captured prior value, both journal checksums and the newest CLEAN sequence, and unchanged drive settings. Restore the backed-up modules/manifest with exact readback parity and remove only files newly created by this test. Keep the resulting CLEAN journals. This gate passed on firmware 1.1.0 on 2026-10-04; it does not prove executable managed editor transfer.

On 2026-10-05, the locally validated, uncommitted Editor-resumption implementation
based on `bc2f935` ran on firmware 1.1.0 at 192.168.1.112. Exact module/code-asset
manifests passed before upload; every uploaded companion and temporary launcher
passed FTP readback parity. Managed fresh entry, declined backed-up recovery,
visible TEST edit and RUN/STOP/discard returned to BASIC. PRINT 6*7 produced 42
and READY. IRQ remained $EA31, the KERNAL clock advanced, drive settings were
unchanged, and both journal checksums passed: newest CLEAN 60/package 514,
older ACTIVE 59/package 514, captured turbo byte 0. CLEAN follows the managed
restore/verify path; REST RAM-shadow reads are not independent live turbo proof.
Original companions/manifest/SEE64YOU.REC were restored with exact readback
parity; the four newly created raw assets/asset manifest and temporary S64R500.PRG
were removed. The new CLEAN journal was retained. This exercises Launcher first
entry only; Publisher execution and actual resumed-entry hardware remain gated.
Editor SHA256: c17a1ab0958078bf38dcb00cb098e41d10c7b1c6d9656fd517681150541953a4.
Launcher SHA256: e71649f248c6112eafcc0499b939d9ef1b418adbf22308fcb4e3dae860e69a22.
Evidence/backups: ignored client/build/hardware-editor-resume-20261005/.
