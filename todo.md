# See64You TODO

This is the authoritative **live outstanding-work queue**.

See64You is now committed to a **C64 Ultimate-only, 16 MiB REU architecture**. Ordinary-C64 compatibility, 128 KiB minimum-REU support and software-IEC application chaining are retired product requirements.

Completed troubleshooting/evidence belongs in canonical docs and Git history, not as checked-off tasks here.

## P0 — Establish the C64U-native runtime

- [ ] Finish platform-contract enforcement and hardware validation: the shared platform preflight now owns UCI, Ultimate-DOS $01, exact 16 MiB REU and non-destructive /sd/ write/read/cleanup verification, and both the diagnostic and real SEE64YOU.PRG launcher use it. The launcher shows exact setup guidance with R = RECHECK / X = EXIT before recovery continues. Positive 16 MiB launcher preflight has passed on real firmware 1.1; finish missing/wrong-size guidance coverage and fold network/optional capability inventory into the launcher diagnostics/setup path.
- [ ] Finish the central C64U system-state manager described in docs/C64U-RUNTIME.md. The host-tested manager/turbo backend and lifecycle coordinator are bound into SEE64YOU.PRG. After preflight/recovery the launcher captures state, durably writes ACTIVE, applies INTERACTIVE, then now exercises an explicit ACTIVE-preserving TRANSITION boundary for editor handoff. Because cached-module transfer is not implemented yet, the stub cancels back to INTERACTIVE and only then performs restore/verify -> durable CLEAN. No legacy software-IEC handoff is invoked. Next connect this transition boundary to the cached-module loader and explicit Exit See64You flow:
  - capture every temporary setting See64You changes;
  - restore the user's prior state on clean exit;
  - verify restoration where possible;
  - never rewrite unrelated persistent Ultimate preferences.
- [ ] Finish crash/interrupted-session journal integration. SEE64YOU.PRG now runs platform preflight, recovery, durable ACTIVE session start and an explicit editor-transition preparation while the session remains ACTIVE. The pending handoff currently cancels safely back to INTERACTIVE, then restores/verifies and writes CLEAN. Recovery failures remain retry-safe. The launcher now persists an allocator-owned, checksummed shared record binding all verified descriptors and captured restore state to the exact durable ACTIVE journal; hardware consumer decoding and CLEAN-session rejection pass. A fresh destination can now adopt the verified ACTIVE session from REU without DOS/capture/turbo writes or another ACTIVE save, preserving the original restore state; host and hardware ownership-transfer gates pass. Next bind this to a verified transfer capsule/editor entry and complete executable handoff.
- [ ] Implement a 16 MiB REU allocator and versioned shared control block. The foundation now includes REU-backed store/load/readback verification at the reserved bank-2 control region. SEE64YOU.PRG creates and verifies the control block after durable ACTIVE session start, inheriting the journal package version/session ID before transition preparation. Host tests cover exact reserved-region DMA behavior and failure propagation. Module cache regions are now allocated from this control block. Next migrate document/history/clipboard/settings/handoff ownership behind allocator handles.
- [ ] Define and validate the application/module manifest. The schema and package identities now exist for Editor (S64EDIT.PRG), Publisher (S64PUB.PRG) and Settings (S64CFG.PRG), carrying module ID, ABI/application version, PRG load/start addresses, byte length, integrity value, cache address and capacity. A module cannot be marked loadable until measured artifact metadata is nonzero and within its cache allocation. SEE64YOU.PRG now reserves three 64 KiB aligned cache regions through the shared allocator and persists the updated control block. CI now generates S64MODULE.MNF from the exact built PRGs after Editor/Publisher/Settings exist, deriving PRG load address, BASIC SYS startup address, full file byte length and CRC32, then immediately revalidates the manifest against those same bytes before packaging it. The C64-side parser now strictly reads S64MODULE.MNF, verifies its magic, exact Editor/Publisher/Settings identities and numeric fields, and fills the already allocator-backed descriptors. SEE64YOU.PRG opens the manifest through Ultimate-DOS after cache planning and will not prepare a transition until all three descriptors are loadable. Direct Ultimate-DOS $21 preload now fills those reserved cache addresses; remaining hardware acceptance is tracked below.
- [ ] Complete module-cache integrity and hardware acceptance. The launcher now preloads all three full PRGs (including load headers) through Ultimate-DOS $21 into their allocated caches in IO_SAFE mode, requiring exact open-file size, successful status, reported transfer count/address and close. A bounded, host-tested full-file REU CRC32 pass now marks the complete package READY only after all three hashes match; descriptor/DMA/corruption failures revoke readiness and cannot proceed. Firmware-1.1 real C64U has passed matching-package direct preload, full cached CRC verification and restore/verify -> durable CLEAN from captured turbo byte 0. Same-length PRG-header corruption is rejected on hardware; X restores/verifies and writes the newer durable CLEAN, and corrected WRITE | CREATE_ALWAYS opens permit journal-slot reuse. Managed CPU_INTENSIVE hashing between IO_SAFE DMA chunks now reduces complete firmware-1.1 startup from over 200 seconds to 12.4 seconds, with turbo byte 0 restored and a newer durable CLEAN. The reproducible module-cache gate now passes from captured turbo byte 2, restoring/verifying it before durable CLEAN, and preserves/restores 17 sentinel blocks outside the runtime/cache range through bank 255. The allocator-owned shared readiness/session record is now published/readback/decoded after verification and revoked before every preload retry and before restore/CLEAN; failed revocation blocks CLEAN, and failed CLEAN saves remain retryable after restore. CLEAN/stale/mismatched journal records reject it. The destination adoption API and independent-owner hardware gate now pass without SD reads or original-state recapture. Next wire the editor entry through a verified transfer capsule and complete executable cached-module handoff. Post-ACTIVE launcher exits now share restore/verify -> CLEAN cleanup, including failed session start that still owns ACTIVE; failed cleanup retains recovery state.
- [ ] Replace the old software-IEC/$C800 production plan with a tiny overwrite-safe cached-module transition path. Prove in bounded stages:
  - Editor → Publisher;
  - Publisher → Editor;
  - Publisher → Settings;
  - Settings → Publisher;
  - repeated full cycles with exact state preservation.
- [ ] After initial preload, demonstrate that normal module transitions perform no software-IEC/KERNAL LOAD and do not require a fresh SD read.
- [ ] Give every transition deliberate UI (OPENING PUBLISHER..., OPENING SETTINGS..., RETURNING TO EDITOR...) even if REU-cached transitions become nearly instant.
- [ ] Define final /sd/SEE64YOU/ package/storage layout and transactional migration from the already-proven /sd/SEE64YOU.CFG location.

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

- [ ] Extend mouse editing beyond the now-implemented menu support: click-to-position in document text, drag selection and useful scrolling, without making keyboard use secondary.
- [ ] Transactional in-app update staging/verification using C64U storage/network facilities while preserving manual SD installation.
- [ ] Bounded rotating diagnostic log without keys/secrets.
- [ ] Optional server-assisted spell-check workflow if a sensible bounded design emerges.
- [ ] Evaluate future firmware capabilities individually and adopt them only when they simplify or improve the product.

## Documentation rule

Stable architecture belongs in README.md and docs/. docs/C64U-RUNTIME.md owns hardware/resource/system-state decisions. External evidence/licensing belongs in docs/ENGINEERING-REFERENCES.md. Current reproducible test procedures belong in docs/LOCAL_TESTING.md.

When a task materially changes, update its canonical documentation in the same PR and remove superseded active guidance rather than accumulating historical plans.
