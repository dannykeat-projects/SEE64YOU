# See64You TODO

This is the authoritative **live outstanding-work queue**. Completed milestones belong in the implementation docs and Git history, not as long checked-off entries here.

## P0 - Finish the stock C64 Ultimate 1.1 path

- [ ] Replace the software-IEC-dependent internal PRG chain with an Ultimate-DOS/SD-aware C64U chain loader while preserving the separate editor/publisher/settings memory budgets and REU handoff. Repeatedly validate editor -> publisher -> settings -> publisher -> editor on real hardware. Current uploaded package stalls on companion loading after physical Ctrl+P and helper returns; REST launches bypass the stall. SD settings persistence itself passed Save/readback, full physical power cycle, automatic cold `/` -> `/sd/` load with invalid REU cache, authenticated GET and profile save/load. Finish uninterrupted cold Publisher startup and TEST DIRECT KEY through the normal chain.
  - Diagnosis recorded on firmware 1.1.0: Software IEC enabled, bus ID 8, default `/sd/`; Drive A/B disabled; last device `$BA=8`; installed PRGs match the current package. Plain BASIC `LOAD"S64PUB",8` also remained at `LOADING`, so the failure reproduces without application exit/chaining. RAM matched exactly the first 2,212 payload bytes of S64PUB.PRG, ending at `$10A5`; the sampled load pointer stayed there across a further 40 seconds. REST/DMA launches work. This locates the observed failure in the IEC transfer lane but does not establish its firmware-level cause. Do not repair the retiring IEC chain unless needed for a loader control test.
  - [ ] Complete the bounded Ultimate-DOS loader shared by all companion transitions. The preflight foundation now selects `/sd/`, opens exact FAT `.PRG` names, obtains the file size, requires load address `$0801`, rejects payloads that reach the `$CC00` software-stack boundary, consumes the two-byte PRG header and leaves the file open at the payload. A read-only full-stream diagnostic covers all three companions. Remaining: the overwrite-safe resident transfer/launch path into the cc65 startup. All four application PRGs must come from the same current package.
  - [ ] Prove the loader's memory layout before implementation: the resident transfer/launch trampoline, command/reply buffers, stack and entry metadata must survive replacement of the caller's code/BSS. Reserve ownership explicitly; do not overwrite REU document/history/clipboard/settings/handoff regions. Complete and validate handoff before transfer.
  - [ ] Implement bounded reads/length checks, close/status handling, restored CPU/turbo state and explicit missing/truncated/I/O-error screens. Define recovery after a partial executable overwrite; do not assume the old application can still run. Preserve a usable route back to the editor.
  - [ ] Run `companion-stream-probe.prg` on real firmware 1.1 to prove full-size Ultimate-DOS reads of the exact current editor/publisher/settings companions. Host command/preflight/failure-path mocks and repository validation pass. Then validate resident-loader overlap/startup in VICE and every transition direction repeatedly on hardware, including exact document/history/clipboard retention, cold startup, settings reload and TEST DIRECT KEY. Measure transition times; demonstrate independence from Software IEC/device 8. Keep PR #61 draft until the hardware gates pass.
  - [ ] Clear the previous screen immediately during each companion transition and show an explicit loading message (for example `LOADING PUBLISHER...`, `LOADING SETTINGS...`, `RETURNING TO EDITOR...`) so slow loads never resemble screen corruption or a hang.
- [ ] Add remote list browsing to the direct Network `$03` publisher; direct open currently accepts a typed page/post slug.
- [ ] Complete real-C64U Standalone CRUD acceptance for both pages and posts: list/open, create/update, draft/publish/unpublish and delete, with exact canonical source round-trip.
- [ ] Complete local file acceptance on real C64U: New/Open/Save/Save As, dirty-document and overwrite prompts, exact reopen and failure/cancel cleanup.
- [ ] Close recovery/autosave on real C64U: `SEE64YOU.REC` creation/refresh, restart Recover/Discard, Save/discard cleanup and no stale recovery after returning to saved state.
- [ ] Finish real-C64U keyboard/editor acceptance: shifted punctuation, Shift/Caps Lock behaviour, modern USB keyboard mappings, forward delete, navigation, wrap/horizontal scrolling, selection/clipboard, undo/redo, search/replace, go-to-line and formatting.
- [ ] Validate large-document interactive responsiveness on real C64U, including REU-backed editing, search/replace, large clipboard, multi-level undo/redo, save/reopen and publisher/preview transitions.

## P0 - Make hardware validation routine

- [ ] Integrate `JC-000/c64-test-harness` as the preferred reusable VICE/real-C64U smoke-test layer, including firmware-1.1-safe REST `/Temp` hygiene.
- [ ] Add a developer command that uploads/runs the current C64U package so normal hardware cycles no longer require manual SD copying.
- [ ] Evaluate `barryw/sim6502` / `u64sim` as an external CI lane for Ultimate-DOS/UCI state-machine regressions; keep real hardware as the final gate.

## P1 - User setup and configuration

- [ ] Implement the first-run setup wizard and simplified Basic/Advanced settings model specified in `docs/SETUP-UX.md`. The wizard must save, read back and validate the persistent configuration before reporting setup complete, then test the configured connection. Preserve an offline-editor escape path and keep diagnostics/transport/profile controls out of the normal setup flow.

## P1 - Compatibility and secondary backends

- [ ] Audit UCI register-window compatibility for `$DF1C-$DF1F` versus `$DE1C-$DE1F`; retain the known-working path unless hardware evidence justifies a change.
- [ ] On hardware/firmware that actually provides HTTP Target `$06`, validate the existing compatibility transport including bearer login/session handling and page/post list/open/CRUD. Firmware 1.1 does not provide `$06`.
- [ ] Validate the secure trusted-LAN relay with a real C64U as a compatibility/fallback path, including relay authentication and remote HTTPS forwarding.
- [ ] Install the WordPress plugin on a disposable test instance and validate Application Password setup, permissions, list/open/create/update/publish/unpublish/delete, slug conflicts and exact source retention. WordPress remains deferred until the general C64U/Standalone path is complete.
- [ ] Profile remaining CPU-heavy C64U operations (large search/replace, preview/rendering, import/export and similar transforms) and extend temporary turbo only where measurements show a worthwhile real-hardware benefit.
- [ ] Keep the conventional-C64 build usable without C64U-only capabilities; validate graceful capability degradation while preserving the existing RAM-headroom guard.

## P2 - Optional polish

- [ ] Mouse selection/menu support where practical.
- [ ] Optional server-assisted spell-check workflow if a sensible bounded design emerges.

## Documentation rule

Stable architecture/behaviour belongs in README and `docs/`. External adoption/licensing belongs in `docs/ENGINEERING-REFERENCES.md`. Current test procedures belong in `docs/LOCAL_TESTING.md`. When a task above is completed or materially changes, update its canonical doc in the same PR and remove the task from this file rather than accumulating completed history.
