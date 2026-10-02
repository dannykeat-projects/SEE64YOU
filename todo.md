# See64You TODO

This is the authoritative **live outstanding-work queue**.

See64You is now committed to a **C64 Ultimate-only, 16 MiB REU architecture**. Ordinary-C64 compatibility, 128 KiB minimum-REU support and software-IEC application chaining are retired product requirements.

Completed troubleshooting/evidence belongs in canonical docs and Git history, not as checked-off tasks here.

## P0 — Establish the C64U-native runtime

- [ ] Finish platform-contract enforcement and hardware validation: the editor rejects anything other than the required 16 MiB REU; the dedicated preflight probe now covers UCI, Ultimate-DOS $01, exact 16 MiB REU and non-destructive /sd/ write/read/cleanup verification with RECHECK/EXIT. Integrate the preflight into the final launcher/supervisor and complete the real-C64U gate.
- [ ] Finish the central C64U system-state manager described in docs/C64U-RUNTIME.md. The host-tested manager/turbo backend plus launcher-owned lifecycle coordinator now enforce capture -> durable ACTIVE journal -> apply and restore/verify -> durable CLEAN journal ordering. The direct-auth diagnostic is migrated to the manager. Production publisher turbo acceleration remains deferred because linking manager/backend code into the current overlay broke its conventional-RAM safety budget. Next bind the lifecycle coordinator into the final launcher/supervisor and explicit Exit See64You flow:
  - capture every temporary setting See64You changes;
  - restore the user's prior state on clean exit;
  - verify restoration where possible;
  - never rewrite unrelated persistent Ultimate preferences.
- [ ] Finish crash/interrupted-session journal integration. The host-tested journal/lifecycle/recovery stack is now wrapped by a real C64U SEE64YOU.PRG launcher loop. Startup recovery runs automatically; blocked states expose R = RETRY, D = DIAGNOSTICS and X = EXIT while preserving retry-safe ACTIVE state. The current success path deliberately stops at EDITOR HANDOFF PENDING rather than invoking legacy chaining. Next integrate platform preflight and the new launcher-owned lifecycle/editor handoff, then complete the real-C64U persistence gate.
- [ ] Implement a 16 MiB REU allocator and versioned shared control block. Migrate document/history/clipboard/settings/handoff ownership behind allocator/control-block handles rather than adding more fixed ad-hoc banks.
- [ ] Define and validate the application/module manifest: module ID, filename, ABI/application version, PRG load address/extent, byte length, startup address and integrity value.
- [ ] Implement module preload/cache using Ultimate-DOS DOS_CMD_LOAD_REU into allocated high REU, with exact length/status/integrity checks.
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
- [ ] Finish modern USB/native-keyboard editor acceptance: punctuation, locks, forward Delete, navigation, selection/clipboard, undo/redo, search/replace, Go To Line and formatting.
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

- [ ] Mouse selection/menu support where it improves the editor without making keyboard use secondary.
- [ ] Transactional in-app update staging/verification using C64U storage/network facilities while preserving manual SD installation.
- [ ] Bounded rotating diagnostic log without keys/secrets.
- [ ] Optional server-assisted spell-check workflow if a sensible bounded design emerges.
- [ ] Evaluate future firmware capabilities individually and adopt them only when they simplify or improve the product.

## Documentation rule

Stable architecture belongs in README.md and docs/. docs/C64U-RUNTIME.md owns hardware/resource/system-state decisions. External evidence/licensing belongs in docs/ENGINEERING-REFERENCES.md. Current reproducible test procedures belong in docs/LOCAL_TESTING.md.

When a task materially changes, update its canonical documentation in the same PR and remove superseded active guidance rather than accumulating historical plans.
