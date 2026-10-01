# See64You TODO

This is the authoritative **live outstanding-work queue**. Completed milestones belong in the implementation docs and Git history, not as long checked-off entries here.

## P0 - Finish the stock C64 Ultimate 1.1 path

- [ ] Fix and revalidate cold-start publishing-settings persistence on stock C64 Ultimate 1.1. Current hardware result: URL/direct key work through warm editor/publisher/settings chaining and TEST DIRECT KEY succeeds, but after a full power cycle the Publisher settings are gone. Trace the actual save/rename/load sequence and inspect `/sd/SEE64YOU.CFG`; do not assume REU persistence or another path tweak is the fix. Keep deterministic `/sd/` selection and host path/failure regressions. Close only after Save + Return, on-disk readback, full power cycle, automatic reload and TEST DIRECT KEY all pass without prior browsing/key import.
- [ ] Replace the software-IEC-dependent internal PRG chain with an Ultimate-DOS/SD-aware C64U chain loader while preserving the separate editor/publisher/settings memory budgets and REU handoff. Repeatedly validate editor -> publisher -> settings -> publisher -> editor on real hardware.
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
