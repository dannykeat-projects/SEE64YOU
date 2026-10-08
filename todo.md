# See64You TODO

This is the authoritative **live outstanding-work queue**.

See64You is now committed to a **C64 Ultimate-only, 16 MiB REU architecture**. Ordinary-C64 compatibility, 128 KiB minimum-REU support and software-IEC application chaining are retired product requirements.

Completed troubleshooting/evidence belongs in canonical docs and Git history, not as checked-off tasks here.

## P0 — Establish the C64U-native runtime

- [ ] Finish platform-contract enforcement and hardware validation: the shared platform preflight now owns UCI, Ultimate-DOS $01, exact 16 MiB REU and non-destructive /sd/ write/read/cleanup verification, and both the diagnostic and real SEE64YOU.PRG launcher use it. The launcher shows exact setup guidance with R = RECHECK / X = EXIT before recovery continues. Positive 16 MiB launcher preflight has passed on real firmware 1.1; finish missing/wrong-size guidance coverage and fold network/optional capability inventory into the launcher diagnostics/setup path.
- [ ] Finish the central C64U system-state manager described in docs/C64U-RUNTIME.md. The host-tested manager/turbo backend and lifecycle coordinator are bound into SEE64YOU.PRG. After preflight/recovery the launcher captures state, durably writes ACTIVE, applies INTERACTIVE, then now exercises an explicit ACTIVE-preserving TRANSITION boundary for editor handoff. The current c64u-package now executes the verified cached Editor handoff and restores/verifies before durable CLEAN on exit; standalone preload diagnostics still cancel back to INTERACTIVE/CLEAN. No legacy software-IEC handoff is invoked. Next connect this transition boundary to the cached-module loader and explicit Exit See64You flow:
  - capture every temporary setting See64You changes;
  - restore the user's prior state on clean exit;
  - verify restoration where possible;
  - never rewrite unrelated persistent Ultimate preferences.
- [ ] Finish crash/interrupted-session journal integration. SEE64YOU.PRG now runs platform preflight, recovery, durable ACTIVE session start and an explicit editor-transition preparation while the session remains ACTIVE. The current package hands off the verified ACTIVE session to the managed editor, then restores/verifies and writes CLEAN on exit. Recovery failures remain retry-safe. The launcher persists an allocator-owned, checksummed shared record binding all verified descriptors and captured restore state to the exact durable ACTIVE journal; hardware consumer decoding and CLEAN-session rejection pass. A fresh destination can adopt the verified ACTIVE session from REU without DOS/capture/turbo writes or another ACTIVE save, preserving the original restore state; host and hardware ownership-transfer gates pass. A separate verified allocator-owned transfer capsule now binds the ACTIVE journal and exact READY destination descriptor, with execution bounds/cache ownership checks and cleanup revocation. The cached handoff and original SYS return now form the current C64U package; finish physical keyboard/file/menu acceptance and the remaining module cycles.
- [ ] Implement a 16 MiB REU allocator and versioned shared control block. The foundation now includes REU-backed store/load/readback verification at the reserved bank-2 control region. SEE64YOU.PRG creates and verifies the control block after durable ACTIVE session start, inheriting the journal package version/session ID before transition preparation. Host tests cover exact reserved-region DMA behavior and failure propagation. Module cache regions are now allocated from this control block. Next migrate document/history/clipboard/settings/handoff ownership behind allocator handles.
- [ ] Define and validate the application/module manifest. The schema and package identities now exist for Editor (S64EDIT.PRG), Publisher (S64PUB.PRG) and Settings (S64CFG.PRG), carrying module ID, ABI/application version, PRG load/start addresses, byte length, integrity value, cache address and capacity. A module cannot be marked loadable until measured artifact metadata is nonzero and within its cache allocation. SEE64YOU.PRG now reserves three 64 KiB aligned cache regions through the shared allocator and persists the updated control block. CI now generates S64MODULE.MNF from the exact built PRGs after Editor/Publisher/Settings exist, deriving PRG load address, BASIC SYS startup address, full file byte length and CRC32, then immediately revalidates the manifest against those same bytes before packaging it. The C64-side parser now strictly reads S64MODULE.MNF, verifies its magic, exact Editor/Publisher/Settings identities and numeric fields, and fills the already allocator-backed descriptors. SEE64YOU.PRG opens the manifest through Ultimate-DOS after cache planning and will not prepare a transition until all three descriptors are loadable. Direct Ultimate-DOS $21 preload now fills those reserved cache addresses; remaining hardware acceptance is tracked below.
- [ ] Complete module-cache integrity and hardware acceptance. The launcher now preloads all three full PRGs (including load headers) through Ultimate-DOS $21 into their allocated caches in IO_SAFE mode, requiring exact open-file size, successful status, reported transfer count/address and close. A bounded, host-tested full-file REU CRC32 pass now marks the complete package READY only after all three hashes match; descriptor/DMA/corruption failures revoke readiness and cannot proceed. Firmware-1.1 real C64U has passed matching-package direct preload, full cached CRC verification and restore/verify -> durable CLEAN from captured turbo byte 0. Same-length PRG-header corruption is rejected on hardware; X restores/verifies and writes the newer durable CLEAN, and corrected WRITE | CREATE_ALWAYS opens permit journal-slot reuse. Managed CPU_INTENSIVE hashing between IO_SAFE DMA chunks now reduces complete firmware-1.1 startup from over 200 seconds to 12.4 seconds, with turbo byte 0 restored and a newer durable CLEAN. The reproducible module-cache gate now passes from captured turbo byte 2, restoring/verifying it before durable CLEAN, and preserves/restores 17 sentinel blocks outside the runtime/cache range through bank 255. The allocator-owned shared readiness/session record is now published/readback/decoded after verification and revoked before every preload retry and before restore/CLEAN; failed revocation blocks CLEAN, and failed CLEAN saves remain retryable after restore. CLEAN/stale/mismatched journal records reject it. The destination adoption API and independent-owner hardware gate now pass without SD reads or original-state recapture. Verified cached Editor entry and clean original SYS return now pass on hardware and are packaged; finish physical input and remaining module cycles. Post-ACTIVE launcher exits now share restore/verify -> CLEAN cleanup, including failed session start that still owns ACTIVE; failed cleanup retains recovery state.
- [ ] Complete cached module cycles beyond the verified Launcher -> Editor -> BASIC path. The current c64u-package now contains the first managed Editor ↔ Publisher round trip: Editor publishes authenticated application state, lazily prepares the exact Publisher asset namespace and ACTIVE-bound settings, enters Publisher through the verified cached transfer path, and Publisher publishes return state before the generic verified transfer back to Editor. Exact-head package/overlay/full validation passes; real C64U execution is still required. Continue in bounded stages:
  - Editor → Publisher → Editor: software/CI complete; hardware acceptance pending;
  - Publisher → Settings → Publisher: software/CI complete; hardware acceptance pending;
  - repeated full cycles with exact state preservation. Host coverage now includes repeated Publisher return-state generations and eight consecutive ACTIVE-bound Settings republish/load cycles; real-C64U repeated-cycle acceptance remains pending.
- [ ] After initial preload, demonstrate that normal module transitions perform no software-IEC/KERNAL LOAD and do not require a fresh SD read. Host coverage now runs repeated Editor/Publisher/Settings cache-to-cache cycles while asserting the destination DOS context is untouched; real-C64U acceptance still needs to confirm no fresh SD transfer occurs on the wire.
- [x] Give every managed module transition deliberate UI. Editor → Publisher now clears stale Editor UI immediately before DMA and shows `OPENING PUBLISHER...`; Publisher → Settings shows `OPENING SETTINGS...`; Settings → Publisher shows `RETURNING TO PUBLISHER...`; Publisher → Editor shows `RETURNING TO SEE64YOU EDITOR...`.
- [ ] Define final /sd/SEE64YOU/ package/storage layout. Runtime package binaries/manifests/assets now live under /sd/SEE64YOU/. Publishing config is now authoritative at /sd/SEE64YOU/SEE64YOU.CFG: Settings first tries the app-folder copy, transactionally migrates a valid legacy /sd/SEE64YOU.CFG by write/reopen/exact validation, and retires the old root copy only after success. Editor PREP prefers the app-folder copy and retains a legacy-root read fallback until migration occurs. Profiles also use the app folder; SEE64KEY.TXT may be imported from the app folder or SD root. Exact-head CI/VICE and focused migration/fallback tests pass. Session journals deliberately remain at /sd/ pending a separately designed exact-record migration that preserves two-slot ACTIVE/CLEAN recovery authority.

## Current P0 handoff checkpoint

### Architecture decision gate — persistent supervisor/code-overlay spike

Pause further debugging of the production Editor -> Publisher application-state
adoption failure while this bounded spike is evaluated. Preserve the current
whole-PRG handoff implementation as the fallback; code commit `e5ffd0f` plus
checkpoint `6ccf818` is the last hardware-diagnosed baseline before this
reassessment. Do not delete or weaken its tests while the spike is undecided.

The question is deliberately narrow: can SEE64YOU keep **one cc65 runtime,
one ACTIVE lifecycle owner and one system-state snapshot alive for the whole
application**, while Editor, Publisher and Settings execute as REU-cached code
assets in the existing `$8C00-$C7FF` slot? If yes, normal mode changes no longer
need PRG-to-PRG crt0 replacement, destination lifecycle adoption, arrival
markers, transfer capsules or reconstruction of application state that is
already live in the owning runtime.

Refined implementation boundary after source inventory: **retain the already
hardware-proven Launcher -> managed Editor bootstrap and original BASIC-return
path.** The persistent application runtime begins once Editor has adopted that
ACTIVE session. Editor remains the lifecycle/system-state owner until final exit;
Publisher and Settings become callable overlays beneath that owner. Do not try
to keep the current launcher image resident below `$8C00` as part of this spike.
This keeps startup/recovery unchanged and targets only the repeatedly failing
Editor/Publisher/Settings ownership transfers.

Build this first as a diagnostic target, tentatively
`persistent-runtime-fit`. It must not replace `c64u-package` or change the
production package while feasibility is unknown.

Reuse rather than redesign:
- the 16 MiB REU allocator/control block;
- durable ACTIVE/CLEAN journal and original BASIC return handling;
- central system-state/performance manager;
- Ultimate-DOS preload and CRC verification;
- the existing Editor code-overlay selector/REU DMA mechanism;
- current Editor, Publisher and Settings logic where it can be linked as code
  assets without changing behaviour.

The first spike should use tiny representative Editor/Publisher/Settings assets,
not port the complete applications. One persistent owner should preload the
assets, enter an Editor asset, mutate known resident/REU state, switch to
Publisher, switch to Settings, return through Publisher to Editor, verify the
same state, repeat the sequence at least 30 times, then perform the normal
restore/verify -> CLEAN -> original BASIC return. No transition inside this
diagnostic should start a new crt0/runtime or adopt another ACTIVE lifecycle.

Acceptance gates before any production pivot:
1. **Conventional RAM:** measure the exact resident map. The design should recover
   useful safety margin rather than merely link; target at least 1 KiB below the
   `$8C00` overlay slot after moving cold/shared data to REU where appropriate,
   while retaining the existing `$CC00-$CFFF` 1 KiB software stack and all
   current memory guards. If that target is impractical, record the measured
   blocker rather than squeezing by bytes.
2. **Overlay fit:** every representative asset must stay within the existing
   15,360-byte slot. Report headroom for each real Editor/Publisher/Settings
   grouping proposed for migration and split responsibilities instead of
   accepting single-digit permanent headroom.

   - BSS is part of this gate: cc65 code overlays do not make ordinary static
     data disappear. Publisher/Settings mode-local state must not simply be
     added to the persistent Editor root.
   - Use `$C800-$CBFF` as a disposable **mode workspace** while Publisher or
     Settings is active, with durable mode data in REU. The BASIC-return record
     remains authoritative in REU during normal operation and claims its RAM
     workspace only during final exit, after Publisher/Settings have returned.
   - Existing Publisher evidence supports this direction: its standalone map
     has 2,248 bytes of BSS, including a 768-byte workspace plus UCI/crypto/
     plaintext/preview/direct-request scratch. The managed Publisher already
     removes the 768-byte workspace from resident BSS. Do not carry the
     remaining scratch as permanent Editor globals.
   - Existing Publisher code assets already fit at CONTENT 15,130, NETWORK
     14,066 and RUNTIME 13,971 bytes. The tight CONTENT asset currently includes
     application-state decoding, module-transition and BASIC-return machinery
     that the persistent Publisher will no longer own, so the migrated grouping
     should gain material headroom rather than merely preserve the current
     230-byte margin.

   - Do not duplicate the live Editor document/body in Publisher. The persistent
     owner already has authoritative `S64Editor`/`S64Document` state; expose
     it through a narrow resident mode/context API and keep overlay pointers
     call-scoped. Publisher's small remote/publish state may stay resident if
     the measured Editor headroom comfortably permits it; request/response
     scratch belongs in the mode workspace.
   - Current Settings keeps three `S64SettingsExchange` records (about 334
     bytes each) plus destination/key/diagnostic scratch. That layout must not
     be copied into resident Editor BSS. The persistent Settings mode should use
     one working record in the mode workspace; canonical/original/profile
     records should be loaded/saved through the existing REU/settings storage
     contracts as needed. Returning from Settings commits or discards that
     working record before Publisher resumes.
3. **State continuity:** resident and REU test state must survive 30 consecutive
   Editor -> Publisher -> Settings -> Publisher -> Editor cycles exactly.
4. **Lifecycle simplicity:** one capture, one durable ACTIVE session and one
   final restore/verify/CLEAN. No per-mode session adoption or application-state
   rehydration is allowed in the spike.
5. **No transition I/O:** after initial preload, mode switches perform no fresh
   SD/module load and no software-IEC/KERNAL LOAD.
6. **Allocator fit:** produce a projected production allocation table. Removing
   whole-PRG module caches/session/capsule ownership must free enough entries for
   the required code assets without exceeding the current allocator capacity;
   do not increase capacity merely to make the spike pass.

Current projection after source/package inspection:
- retain one full Editor-root PRG cache for the already-proven Launcher -> Editor
  bootstrap, plus its module-session, initial transfer capsule and BASIC-return
  records: 4 allocations;
- Editor code assets: 4 allocations;
- current Publisher groups: 3 allocations;
- Settings must be split: the current managed `S64CFG.PRG` artifact is 38,579
  bytes and cannot be treated as one 15,360-byte asset; budget 2-3 Settings
  assets until measured;
- prefer one unified ACTIVE-bound code-asset/session record for all in-process
  modes, because there is now only one lifecycle owner.

That projects to 14 allocations with two Settings assets or 15 with three,
without increasing the 16-entry allocator. Separate per-mode asset-session
records would consume the remaining margin unnecessarily and should not be the
default persistent-runtime design. This is a projection, not yet a validated
package layout.
7. **Validation:** add focused host tests, a repeatable VICE cycle/ABI test and
   the usual map/headroom checks. Real C64U testing is required only after the
   software spike passes these gates.

Software fit result (GitHub-hosted cc65 2.19, exact head before this note):
- synthetic persistent-owner diagnostic builds successfully;
- real persistent Editor root builds with MODEL 14,883 / 15,360,
  FILE 13,941 / 15,360, RUNTIME 14,972 / 15,360 and PUBLICATION 5 / 15,360;
- resident Editor headroom is **3,346 bytes below $8C00**, comfortably above
  the 1 KiB gate and far better than the production whole-PRG handoff build;
- the software fit gate therefore **passes**. Proceed with the persistent
  Editor-owned runtime architecture, while retaining the existing whole-PRG
  path as fallback until real Publisher/Settings overlays and hardware cycles
  are accepted.

GitHub-hosted validation also passed the synthetic VICE gate without the
self-hosted runner: cc65/VICE/Xvfb were installed on `ubuntu-latest`, the probe
preloaded three REU assets once, completed 30
Editor -> Publisher -> Settings -> Publisher -> Editor cycles, preserved exact
resident and REU state, retained one lifecycle owner and wrote one final CLEAN.
Use `.github/workflows/persistent-runtime-spike.yml` for software-only spike
validation while the self-hosted runner is unavailable. Real C64U hardware
remains the final authority.

The expanded real Publisher migration slice now passes on GitHub-hosted VICE:
- persistent Editor alone retains **3,351 bytes** below `$8C00`;
- persistent Editor + Publisher retains **1,433 bytes** below `$8C00`, still
  above the 1 KiB acceptance gate but close enough that resident BSS growth must
  remain deliberate;
- real Publisher slices measure PCONTENT **10,392 / 15,360**, PNETWORK
  **14,397 / 15,360**, PRUNTIME **1,408 / 15,360** and PUI **305 / 15,360**
  bytes. PUI is the fourth and final Publisher overlay slot and owns command
  decoding plus local status/type mutations. PCONTENT contains
  remote/publish state, Preview, direct-key setup, the staged-open finalizer and
  compact remote-list parsing. PNETWORK contains Network Target $03, SHA/HMAC,
  direct HTTP, the real document writer, staged-open receiver and bounded
  remote-list fetch, leaving **963 bytes** in that slot;
- the focused real-PUI VICE gate passes overlay entry, status toggling,
  type-change intent and action routing. Type changes return an explicit
  `S64_PUBLISH_UI_TYPE_CHANGED` intent so resident orchestration can switch to
  PCONTENT and clear remote binding without a forbidden nested overlay call;
- screen rendering remains resident. A focused experiment moved the main
  conio renderer into PUI: `clrscr()` and `revers()` returned, but
  `gotoxy()` hung under the real overlay veneer (trace 31). That experiment
  was reverted rather than adding special rendering machinery;
- direct-key parsing/setup is separated from signing/transport. The 32-byte live
  key remains resident while setup code lives in PCONTENT, keeping request-time
  signing in PNETWORK without duplicating key parsing there;
- authenticated remote open now uses a deliberate two-phase path because nested
  overlay calls are forbidden. PNETWORK authenticates and streams the canonical
  response into existing REU storage: headers temporarily borrow the history
  payload region and the raw body goes directly to the document region.
  PCONTENT then validates required metadata, converts the body ASCII -> native
  PETSCII in place and commits the live `S64Document`;
- staged-open header buffering uses a bounded **64-byte resident scratch**. A
  $CB80 mode-workspace scratch was rejected after focused VICE showed that its
  contents did not survive this path reliably. The 64-byte resident cost keeps
  total headroom at 1,401 bytes and removes that unexplained dependency;
- the focused staged list/open VICE gate now passes an authenticated compact
  list-summary request, PCONTENT slug/title/count parsing, then authenticated
  document open, exact 101-byte raw-header REU staging, PCONTENT metadata commit,
  signed Order/status/date/title/slug checks and exact PETSCII body conversion.
  It adds no allocator entry;
- the 30-cycle persistent Editor/Publisher/Settings lifecycle gate still passes;
- the real Publisher VICE gate switches namespaces, binds remote state, exercises
  Network Target $03 open/write/read, parses the direct HTTP challenge, computes
  SHA/HMAC authentication, accepts authenticated test/save/delete operations,
  round-trips REU settings state, visits Settings and returns to Editor under
  the same resident owner;
- the former direct-auth failure reported as step 124 was a VICE fixture bug:
  cc65 translated C string response literals into PETSCII, so the fake wire
  began with byte $C8 instead of ASCII $48 for `HTTP`. The fixture emits
  explicit raw ASCII byte arrays; production HTTP parsing was not weakened;
- the combined executable VICE harness uses bounded stubs/placeholders for
  responsibilities that have their own focused real-code gates, so embedded REU
  preload images do not masquerade as product resident RAM. It now exercises a
  four-entry Publisher namespace with a one-byte placeholder fourth asset, while
  separate focused gates execute the real document writer, staged list/open path
  and real PUI overlay;
- Preview's cache/metadata stay in the existing $C800-$CBFF mode workspace in
  persistent mode; the staged-open receiver is the exception, using its
  measured 64-byte resident scratch after the workspace source proved unreliable;
- VICE uses warp mode with bounded execution so crypto-heavy failures remain
  quick and identifiable.

This is sufficient to continue the Publisher migration incrementally. Keep the
whole-PRG production route intact until remaining Publisher UI/document
responsibilities and then Settings are migrated and accepted. The self-hosted
full integration lane for this exact checkpoint may remain queued independently;
GitHub-hosted persistent-runtime and Publisher gates are the current software
evidence.

Decision rule:
- If the diagnostic passes cleanly and the projected real asset layout has
  sensible headroom, pivot production incrementally to the persistent-runtime
  model. First replace only Editor <-> Publisher, then Settings, retaining the
  old handoff path until each replacement is accepted.
- If resident RAM, overlay fit or allocator ownership requires comparable or
  greater machinery than the current design, stop the spike, leave production
  unchanged and resume diagnosis at `s64_application_state_load_wire()`.

Implementation order:
1. inventory the current resident Editor root, overlay selector, Publisher
   overlay groups, Settings dependencies and allocator ownership;
2. add the isolated persistent-runtime fit target and synthetic three-mode
   cycle test;
3. measure maps/headroom and projected allocator entries;
4. only if those gates pass, link real Publisher responsibilities as assets,
   then real Settings responsibilities;
5. after software/VICE success, hardware-accept repeated cycles and clean BASIC
   return;
6. only then update the canonical architecture docs and retire obsolete
   cross-PRG transfer/adoption machinery.

Use PR #61 on `feature/c64u-publisher-chain` as the authoritative working branch. Before changing code in a new chat, read `AGENTS.md`, this file, `docs/C64U-RUNTIME.md`, `docs/ARCHITECTURE.md` and `docs/LOCAL_TESTING.md`, then inspect the current PR head rather than assuming a saved SHA is still current.

The active bounded task is the cached-module executable handoff. The launcher currently preflights, recovers, enters durable ACTIVE, creates/verifies the REU control/allocator, loads the generated module manifest, preloads and CRC-verifies Editor/Publisher/Settings, publishes the shared module-session record, enters TRANSITION, creates/readbacks a destination-bound transfer capsule and derives an exact transfer plan. The current c64u-package executes the verified copier into the managed Editor. Standalone launcher/preload targets remain gated diagnostics that cancel back to INTERACTIVE/CLEAN.

New handoff components are:
- `client/src/platform/transfer_capsule.[ch]`: allocator-owned 64-byte capsule binding ACTIVE journal + exact READY destination descriptor, with checksum, cache-ownership and execution-range checks;
- `client/src/platform/module_transfer.[ch]`: derives PRG-body REU source (cache + 2), RAM destination, exact payload length and startup address; the current package executes this verified plan;
- `client/src/platform/module_transfer_stub.s`: size-guarded transient REU DMA stub copied to screen RAM at $0700, parameters at $07D0, plus one-shot arrival marker at $02A7-$02A9 immediately before destination entry;
- `client/src/platform/module_runtime.[ch]`: lightweight destination owner that, only with trusted arrival, loads control/capsule/shared-session state, adopts the original system snapshot/lifecycle without a fresh ACTIVE save, and can resume INTERACTIVE / later restore-verify-CLEAN;
- tests: `transfer_capsule_test.c`, `module_transfer_test.c`, `module_runtime_test.c`, plus launcher-runtime coverage.

The diagnostic executable Launcher -> managed Editor entry/short edit/CLEAN gate now preserves a real original SYS return context. Before launcher crt0 runs it captures stack, zero page, $01 and flags; a checksummed allocator-owned record binds this context to the exact ACTIVE journal. The editor validates it before readback revocation and restore/verify/CLEAN, runs cc65 destructors and returns through the original SYS frame. Firmware-1.1 hardware reached responsive BASIC, restored IRQ $EA31, executed PRINT 6*7 -> 42, and persisted CLEAN sequence 56/package 513. Host failure/integrity checks, both VICE probes and diagnostic package memory gates pass. Package 0x0201 carried those hardware-verified bytes; current package 0x0202 expands allocator capacity to 16 entries and now passes the same short hardware entry/edit/CLEAN/BASIC-return gate (CLEAN 58/package 514). Standalone preload targets remain diagnostic.

Next work:
The 2026-10-07 machine-dependent validation restored the runner by starting
Ubuntu/WSL and verified two package-build failures at 3586256: Editor resident
overflow (14 bytes) and managed Settings overflow (512 bytes). The scoped fixes
preserve all memory guards and dirty-exit behavior. Full local host/cc65/VICE
validation passes. Editor BSS ends at $8BFF; MODEL/FILE/RUNTIME/PUBLICATION have
89/5/76/50 bytes spare. Managed Settings has 9150 bytes before $CC00. Preserve
these measured limits when continuing the outstanding work below; real-C64U
acceptance remains required.
On 2026-10-07 the missing-REU guidance/exit gate passed with unchanged journal
slots, restored original files and preserved drive state/IRQ. After enabling
16 MiB, real hardware exposed managed Editor entry stage 4: pointer scratch
aliases made `sizeof` transfer only two capsule/session bytes. Array-extent
aliases now preserve full record operations; ordinary and managed-workspace
capsule/session/adoption host suites and full local validation pass; exact-head
CI for cc563f4 passed. Real C64U recovered ACTIVE 69 to CLEAN 70, began ACTIVE
71, entered the managed Editor and displayed the first TEST input immediately.
Integrated diagnostics isolated OPEN_FILE's target error. Native firmware 1.1
reports a missing app-folder config as `FILE DOESN'T EXIST`; exact classification
now maps it to NOT_FOUND so the existing legacy fallback can run. Regression and
full local validation pass without changing overlay sizes or resident limits.
The fixed native probe confirms NOT_FOUND/legacy-root OK. The normal package
passes settings preparation using the original root config alone.
The uninstrumented package passed settings preparation but reproduced Publisher
entry failure 0. Live Editor IRQ points to $0840, which Publisher overwrites with
its failure handler. Both transfer paths now run cc65 destructors before DMA;
the VICE regression fails on the old code and passes with original IRQ restored
on generic and capsule-DMA departure. Full local host/cc65/VICE validation passes;
resident footprint and all overlay sizes are unchanged (Editor $8BFF, Publisher
1474-byte headroom, Settings 9213 bytes before $CC00).
The normal package now reaches Publisher's genuine stage-4 failure with original
IRQ $EA31 restored and a valid software stack. An isolated hardware probe
confirms session adoption, IO_SAFE mode and Publisher asset binding succeed;
`s64_application_state_load_wire()` fails on loading or validating the
ACTIVE-bound application-state record. Next isolate its allocation/DMA/header/
journal/CRC checks. Full module-cycle acceptance remains pending.
At wrap-up the diagnostic session recovered to CLEAN 86; original package,
config and recovery files were restored with exact readback, drive state and
IRQ unchanged, and the machine returned to BASIC. This recovery is not managed
clean-exit acceptance.
The interrupted diagnostic recovered to CLEAN 78, restored original files
exactly and verified unchanged drives/IRQ. Full module cycles and managed exit
remain unverified. Managed Settings depends on both record headers, preventing
stale incremental PRGs; 3bed2ac CI passed.

1. Finish physical USB/native keyboard and ordinary editing/file/menu acceptance of the current c64u-package. Preserve original-context/ACTIVE binding, exact manifests and stack/headroom gates; missing/corrupt context keeps a safe CLEAN park.
   - 2026-10-06 fresh-editor hardware observation: first typed characters can be invisible while the editor cursor advances, then appear on a later redraw. The append fast path now explicitly positions conio at the tracked editor cursor, and the UI probe deliberately perturbs conio's cursor before its first append. Real-C64U confirmation is still required.
   - 2026-10-06 responsiveness observation: intermittent multi-second typing stalls matched the synchronous durable recovery write path. The editor now polls input without changing blocking menu/help behavior and only runs `maybe_autosave()` after one full second with no keyboard/joystick/mouse event; continuous typing therefore cannot start SD recovery I/O between keystrokes. The existing 30-second durable cadence remains, with an already-due fresh recovery written on the first genuine pause. CI/VICE and real-C64U responsiveness/recovery acceptance remain required.
2. Hardware-accept the complete managed Editor → Publisher → Settings → Publisher → Editor cycle now present in the production package. Software/CI validation covers authenticated application-state publication/return, exact Publisher asset/session authority, canonical ACTIVE-bound settings, Settings round trips, destination capsule validation, cached transfer execution and return to Editor without a fresh SD module load. Preserve original-context/ACTIVE binding, 16/16 allocator ownership, synchronous $C800 scratch and all stack/overlay guards. Repeated Publisher return-state and Settings authority cycles now have host regression coverage; repeated real-C64U cycles remain pending. Authenticated resumed Editor entry remains hardware-proven on firmware 1.1.0 (2026-10-06, final CLEAN 66/package 514).

Publication placement and the preparation-only caller use an Editor-only
fourth PUBLICATION overlay (`S64PREP.BIN`). Editor asset manifest/session v2
requires exactly four assets; Publisher v2 still requires three. Publisher asset
preparation is deliberately **not** launcher-resident: the executable launcher has
a protected original-SYS return workspace beginning at $C800 and remains on the
already-proven compact Editor-only producer. The current 14-file package now also
contains the exact managed Publisher root plus `S64POVL.MNF`, `S64PCNT.BIN`,
`S64PNET.BIN` and `S64PRUN.BIN`. A preparation-only Publisher producer reuses
the existing Publisher-specialized asset codec inside PUBLICATION/PREP and is now
called by the production Editor before first Publisher entry. With its DOS/CRC helpers the measured Editor
layout is MODEL 15,216, FILE 15,355, RUNTIME 15,275, PUBLICATION 12,645 bytes,
with resident BSS ending $8A14 below the unchanged $8C00 slot. Invoking Publisher
preparation consumes the final four allocator entries and requires exactly
16/16. The production Editor now performs this preparation, publishes the
destination capsule and transfers through the existing transient DMA stub.
Publisher adopts the same ACTIVE session, consumes the canonical settings and
application state, and now publishes authenticated return state before the
generic cached transfer back to Editor. Exact-head CI/full validation passes.
The complete Editor ↔ Publisher cycle and recent typing/autosave fixes still
need real-C64U acceptance.
See docs/C64U-RUNTIME.md.

Do not merge/deploy merely because diagnostic validation passes.

## P0 — Make setup appliance-like

- [ ] Implement the first-run launcher/wizard from docs/SETUP-UX.md:
  - platform capability check;
  - exact one-time menu guidance only for prerequisites software cannot enable;
  - immediate RECHECK;
  - website URL;
  - SEE64KEY.TXT import with manual fallback;
  - direct connection test;
  - transactional save + final readback;
  - EDIT OFFLINE path when only networking/publishing is unavailable.
- [ ] Finish simplified Basic Settings / Advanced Settings separation. Basic now exposes website, publishing-key import/manual replacement/disable, readiness check, save/cancel and the Advanced entry only; plaintext-HTTP policy is Advanced-only. Existing key-import diagnostics are now explicitly reachable from Advanced. Finish moving broader hardware/cache/recovery diagnostics into the integrated Diagnostics screen rather than Basic.
- [ ] Finish the integrated Diagnostics screen/export. Advanced Settings now has a read-only Diagnostics screen showing package version, UCI, DOS, Network $03, optional HTTP $06, REU bank count/16 MiB status and the authoritative config path, plus a drill-down to key-import I/O results without exposing secrets. Still add firmware/control details, allocator/cache usage, performance mode, recovery state, connection/auth state and export.
- [ ] Finish explicit Exit See64You acceptance. The Editor now resolves dirty source with S SAVE / D DISCARD / STOP CANCEL; Save reuses the normal verified save path and exits only if the document becomes clean, while Discard removes the recovery snapshot. Managed exit already revokes shared authority, restores/verifies machine state, writes CLEAN and returns through the original SYS context. Real-C64U acceptance remains.

## P0 — Make hardware validation routine

- [ ] Integrate JC-000/c64-test-harness as the preferred reusable VICE/real-C64U smoke-test layer, including firmware-1.1-safe REST /Temp hygiene.
- [ ] Add a developer command that uploads/runs the current C64U package and can collect bounded screen/memory/diagnostic output.
- [ ] Evaluate barryw/sim6502 / u64sim as an external CI lane for UCI/Ultimate-DOS state-machine regressions.
- [ ] Add automated tests for 16 MiB allocator ownership, module-cache integrity, state-manager restoration and manifest/ABI mismatch rejection.
- [ ] Keep real C64U hardware as the final gate for firmware, turbo, DMA/REU, network and USB-keyboard claims.

## P1 — Exploit the larger runtime deliberately

- [ ] Redesign large-document storage around a measured paged/piece-based REU model so logical document size is not constrained by old 16-bit/original-C64 assumptions.
- [ ] Move undo/redo to an allocator-backed variable history pool with sensible memory-pressure reclamation instead of a fixed 16-level product limit.
- [ ] Make clipboard capacity allocator-backed.
- [ ] Add memory-pressure policy: reclaim disposable caches, previews/remote lists and oldest history before critical source.
- [ ] Profile search/replace, preview/rendering, hashing, import/export and similar CPU-heavy work; use temporary turbo through the system manager where measurements justify it.
- [ ] Re-enable/develop useful editor conveniences that were previously excluded only for conventional-C64 BSS headroom, subject to UI/value testing.
- [ ] Add a simple persistent Editor appearance preference. With no saved preference (first start), use the normal/default C64 colour scheme rather than forcing the current black/green theme. Add a discoverable `Dark Mode` menu item and a collision-free keyboard shortcut (Ctrl+D is currently a suitable candidate if still unused when implemented). Dark Mode is specifically black background with white text. Save the choice and restore it on later launches. Reuse existing settings/persistence infrastructure where practical; do not create a separate theme subsystem for this single boolean preference. Keep menus/status/help consistent with the active mode, and preserve readable cursor/selection/reverse-video states in both schemes.

## P1 — Finish editor/storage/publishing acceptance

- [x] Add remote list browsing to the direct Network $03 publisher. It now uses bounded authenticated summary requests with constant RAM use; real-C64U CRUD acceptance remains below.
- [ ] Complete real-C64U Standalone CRUD acceptance for pages/posts: list/open, create/update, draft/publish/unpublish and delete with exact canonical round-trip.
- [ ] Complete local New/Open/Save/Save As hardware acceptance, including overwrite, exact reopen and failure/cancel cleanup.
- [ ] Close recovery/autosave hardware acceptance, including REU fast state plus durable SD recovery.
- [ ] Finish real-C64U input acceptance: modern USB/native keyboard punctuation, locks, forward Delete, navigation, selection/clipboard, undo/redo, search/replace, Go To Line and formatting; joystick port-2 cursor/menu navigation; and USB-mouse/1351 menu pointing.
- [ ] Stress large documents, deep history, large clipboard, recovery and repeated overlay transitions on real hardware.

## P1 — Retire migration baggage

- [ ] Once the C64U-native replacements pass hardware gates, remove the conventional-C64 release build as a CI/product requirement.
- [ ] Remove conventional buffer/history implementations and S64_REU_BUFFER-style forks that exist only for ordinary-C64 compatibility after their replacements are covered.
- [ ] Remove the software-IEC chain from production code.
- [ ] Remove obsolete $C800-$CBFF resident-loader production constraints while retaining only useful regression diagnostics.
- [ ] Change CI/artifact naming so the C64U package/launcher is the primary downloadable product; stop publishing see64you.prg as a product artifact.
- [ ] Audit docs/tests/comments for stale “base C64”, 128 KiB minimum or device-8 assumptions during each migration removal.

## P1 — Secondary transports/backends

- [ ] Audit safe compatibility for alternate UCI register windows without abandoning the hardware-proven $DF1C-$DF1F path without evidence.
- [ ] On hardware/firmware that actually exposes HTTP Target $06, validate it as an optional transport; do not infer TLS without proof.
- [ ] Validate the trusted-LAN relay on real C64U as an HTTPS compatibility path.
- [ ] Validate the WordPress plugin on a disposable instance: Application Passwords, permissions, list/open/create/update/publish/unpublish/delete, slug conflicts and exact source retention.

## P2 — C64U-native enhancements

- [ ] Extend mouse editing with useful edge/autoscroll behavior and real-C64U tuning; menu pointing, click-to-position and drag selection are implemented without making keyboard use secondary.
- [ ] Transactional in-app update staging/verification using C64U storage/network facilities while preserving manual SD installation.
- [ ] Bounded rotating diagnostic log without keys/secrets.
- [ ] Optional server-assisted spell-check workflow if a sensible bounded design emerges.
- [ ] Evaluate future firmware capabilities individually and adopt them only when they simplify or improve the product.

## Website theme acceptance

- [ ] Complete manual browser acceptance for the implemented website theme editor: C64 glyph appearance, no-JS lifecycle, 390px viewport and a legacy browser or documented CSS/font fallback path (docs/LOCAL_TESTING.md section 13). Automated validation is separate from historic-browser certification.

## Documentation rule

Stable architecture belongs in README.md and docs/. docs/C64U-RUNTIME.md owns hardware/resource/system-state decisions. External evidence/licensing belongs in docs/ENGINEERING-REFERENCES.md. Current reproducible test procedures belong in docs/LOCAL_TESTING.md.

When a task materially changes, update its canonical documentation in the same PR and remove superseded active guidance rather than accumulating historical plans.
