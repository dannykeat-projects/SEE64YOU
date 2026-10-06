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
  - repeated full cycles with exact state preservation.
- [ ] After initial preload, demonstrate that normal module transitions perform no software-IEC/KERNAL LOAD and do not require a fresh SD read.
- [ ] Give every transition deliberate UI (OPENING PUBLISHER..., OPENING SETTINGS..., RETURNING TO EDITOR...) even if REU-cached transitions become nearly instant.
- [ ] Define final /sd/SEE64YOU/ package/storage layout and transactional migration from the already-proven /sd/SEE64YOU.CFG location. Runtime package binaries/manifests/assets are now built and consumed from /sd/SEE64YOU/; persistent config and session journals deliberately remain at their hardware-proven root locations until the separate transactional migration. Exact-head CI/VICE validates the folder package.

## Current P0 handoff checkpoint

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
1. Finish physical USB/native keyboard and ordinary editing/file/menu acceptance of the current c64u-package. Preserve original-context/ACTIVE binding, exact manifests and stack/headroom gates; missing/corrupt context keeps a safe CLEAN park.
   - 2026-10-06 fresh-editor hardware observation: first typed characters can be invisible while the editor cursor advances, then appear on a later redraw. The append fast path now explicitly positions conio at the tracked editor cursor, and the UI probe deliberately perturbs conio's cursor before its first append. Real-C64U confirmation is still required.
   - 2026-10-06 responsiveness observation: intermittent multi-second typing stalls matched the synchronous durable recovery write path. The editor now polls input without changing blocking menu/help behavior and only runs `maybe_autosave()` after one full second with no keyboard/joystick/mouse event; continuous typing therefore cannot start SD recovery I/O between keystrokes. The existing 30-second durable cadence remains, with an already-due fresh recovery written on the first genuine pause. CI/VICE and real-C64U responsiveness/recovery acceptance remain required.
2. Hardware-accept the managed Editor → Publisher → Settings → Publisher → Editor cycle now present in the production package. Software/CI validation covers authenticated application-state publication/return, exact Publisher asset/session authority, canonical ACTIVE-bound settings, destination capsule validation, cached transfer execution and return to Editor without a fresh SD module load. Preserve original-context/ACTIVE binding, 16/16 allocator ownership, synchronous $C800 scratch and all stack/overlay guards. Then implement Publisher → Settings → Publisher and repeated cycles. Authenticated resumed Editor entry remains hardware-proven on firmware 1.1.0 (2026-10-06, final CLEAN 66/package 514).

Publication placement and the preparation-only caller use an Editor-only
fourth PUBLICATION overlay (`S64PREP.BIN`). Editor asset manifest/session v2
requires exactly four assets; Publisher v2 still requires three. Publisher asset
preparation is deliberately **not** launcher-resident: the executable launcher has
a protected original-SYS return workspace beginning at $C800 and remains on the
already-proven compact Editor-only producer. The current 14-file package now also
contains the exact managed Publisher root plus `S64POVL.MNF`, `S64PCNT.BIN`,
`S64PNET.BIN` and `S64PRUN.BIN`. A preparation-only Publisher producer reuses
the existing Publisher-specialized asset codec inside PUBLICATION/PREP; it is not
called by production Editor code yet. With its DOS/CRC helpers the measured Editor
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
- [ ] Implement simplified Basic Settings and move transport/hardware/cache/recovery diagnostics to Advanced.
- [ ] Add an integrated Diagnostics screen/export showing version, firmware/hardware, UCI targets, 16 MiB REU health/usage, module cache, performance mode, storage/recovery and connection state without exposing secrets.
- [ ] Add explicit Exit See64You flow that resolves dirty source, closes resources, restores machine state and marks the session clean.

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

- [ ] Add remote list browsing to the direct Network $03 publisher; direct open currently accepts a typed page/post slug.
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
