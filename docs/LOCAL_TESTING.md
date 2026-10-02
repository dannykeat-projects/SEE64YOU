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

## 3. Current C64U package build

During migration:

~~~sh
make -C client launcher c64u publisher
~~~

Current package files:

- SEE64YOU.PRG — new C64U launcher shell; recovery loop is live, editor handoff intentionally pending;
- SEE64YOU-C64U.PRG
- S64EDIT.PRG
- S64PUB.PRG
- S64CFG.PRG

The future launcher/manifest/module-cache package may rename/reorganize these internal files. The user-facing goal is one SEE64YOU.PRG entry point and an application folder.

The launcher links to `build/see64you-launcher.prg`; `make launcher` always
refreshes `build/SEE64YOU.PRG` from that image. This avoids the legacy
`see64you.prg` validation build being mistaken for the launcher on Windows.
Run `make launcher` after legacy validation builds before any hardware upload.

A conventional client/build/see64you.prg may still be produced temporarily but is no longer a supported product artifact.

## 4. Hardware access from the development PC

Ultimate64 Manager is installed on the existing Windows development machine. Read the current host/profile from its local configuration rather than assuming an address remains unchanged, and never print/commit stored passwords.

REST/FTP can be used for authorized upload/run/readback testing. Preserve unsaved work before remotely replacing the running PRG.

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

Build `make -C client launcher c64u publisher module-manifest` and keep
`SEE64YOU.PRG`, `S64EDIT.PRG`, `S64PUB.PRG`, `S64CFG.PRG` and `S64MODULE.MNF`
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
and map. The sample immediately beyond runtime-owned memory is now 0x050180, after
the allocator-owned session record; update the probe when ownership changes.
This checks sampled preservation, not all unrelated REU bytes.

Using the required 16 MiB configuration:

- open a known module through Ultimate-DOS;
- load to an assigned REU cache region using DOS_CMD_LOAD_REU;
- verify exact byte count/status;
- verify integrity against the source;
- verify other REU allocations remain unchanged.

### 6.5 Cached module transition

After all normal modules have been validated/cached:

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
