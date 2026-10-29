# SEE64YOU work queue

**Target:** offline C64U `.8tml` Editor, standard Publisher with numbered character GUI and local/server file selectors, and separate Settings. [Product scope](docs/PRODUCT.md) pins the 26 October 2026 publishing API 1 test draft; [Publisher design](docs/PUBLISHER.md) defines the new GUI and conditional download-to-edit workflow. Implementation is on `feature/8tml-standard-core-20261027` / draft PR #66; migration and release acceptance remain incomplete.

## Active Editor priorities

1. **P0 — Save integrity:** preserve existing `.tmp`/`.bak` recovery copies and retain the original through replacement. Host regressions cover collision, backup/install/rollback/cleanup failures. Complete matching SD/USB fit/manifests, VICE and bounded real-C64U disk acceptance before promotion.
2. **P1 — Large-source loading:** measure and batch the per-byte REU writes in the existing source reader; retain byte-exact source, line-ending normalization and bounded memory.
3. **P1 — Editing and saving cost:** benchmark long-distance gap movement and redundant save scans before making a bounded change.
4. **P1 — REU error reporting:** distinguish failed reads from missing characters/search misses and preserve recoverable dirty text.
5. **P2 — Rendering, paste and undo:** use repeatable measurements and fault injection to target redraws and bulk edits.

- [x] Save integrity implementation and focused host fault regressions.
- [x] Save integrity SD and `/USB1/` shipping builds/fit/manifests, both packaged VICE startup/BASIC-return checks, and bounded real-C64U SD native-DOS replacement rollback and recovery-collision acceptance. Exact source/package hashes: `client/build/save-safety-validation.json`; hardware receipt: `client/build/save-safety-hardware-20261029-r2/`. All 44 original files, configuration, DOS contexts, IRQ/turbo/colours, scratch byte and responsive BASIC restored and verified. Physical input, cold power, uncontrolled interruption and real USB-only acceptance remain separate release gates.

## Current validation blockers

- [x] Exact `82dfda1` full host suite passed, including directory-stream and picker fixtures.
- [ ] Publisher at `82dfda1` fails the cc65 SD link: BSS ends at `$D19A`, overflowing by 1,435 bytes, before the required 1,024-byte reserve. Reclaim at least 2,459 bytes or review the picker scope; do not weaken guards or execute this failed package. USB/package VICE/C64U confirmation acceptance is blocked by fit.
- [x] Correct the CI standalone-direct host command to avoid linking the implementation twice; the test already includes it. Original failed run: `40962868301`; corrected-command run: `40963057669`.
- [ ] The shared runner is restricted to private repositories; SEE64YOU currently reports PUBLIC, so its self-hosted job must remain excluded. Use local tools and public-runner CI unless that policy or repository visibility is explicitly changed.
- [ ] Directory hardware framing remains unverified; the current picker scan limit does not bound total transport/drain time or allow RUN/STOP during enumeration. Keep these limits visible before expanding the picker.

## P0 — Remaining release acceptance

**Previously validated (older build):** Standard API 1 signing, independent Publisher/Settings, local storage fault guards, SD/USB matching package/manifest checks, VICE startup/return and bounded real-C64U SD create/replace and rollback tests. The earlier `45d2069` package had Publisher headroom 2,047/2,041 bytes and launcher 1,070/1,056 bytes. Receipts and exact hashes are in Git history and `client/build/`; **none certify the later numbered-GUI/directory-picker commits or replace their new build/fit gates**.

- [ ] Finish physical keyboard/Link, cold-power exact reopen, uncontrolled interruption/recovery and external compatible-reader navigation with the exact package; real USB-only acceptance also remains unverified. REST-assisted Editor/Settings/Publisher SD results below do not certify these gates.
- [ ] **Failed historical native matrix:** the next non-shipping managed Editor build exceeds BSS by612 bytes, MODEL_CODE by2489, FILE_RODATA by10 and PUBLICATION_CODE by747. Later historical targets remain unrun. The independent Editor/package is separately validated; do not weaken guards, add another runtime or claim the full matrix passed. Retire or reconcile this legacy target after replacement acceptance.

## Editor reliability and responsiveness audit targets (28 October 2026)

**Investigation targets; not completion claims.** PR #67 separately proposes batched REU source loading and must retain its own validation. Keep autosave disabled; benchmark shipping paths before wider rewrites. Prioritize measured user impact and data safety over broad rewrites.

- [ ] **P0 — Protect the previous saved document and existing recovery temporaries.** The shared writer now blocks existing `.tmp`/`.bak`, retains the original during replacement and attempts rollback after failed installation. Host faults and bounded native-DOS checks verify distinct old/new recoverable bytes and retry refusal. Cold-power/interruption guarantees remain unverified.
- [ ] **P1 — Validate PR #67's batched REU source loading.** `document_wire_reader_write` converts each input byte and calls `s64_buffer_insert`, which issues a single-byte REU store. Benchmark C64U Editor/Publisher load of 1 KiB, 16 KiB and near-60 KiB before changing the shared path; prototype bounded, validated chunk append while preserving ASCII/LF conversion, gap invariants and failure handling. Real-C64U Publisher loaded 16,385 bytes in 91.87 seconds with Turbo Control Off; the relative cost of DOS and REU is not yet isolated.
- [ ] **P1 — Profile first edit after a distant cursor move, large paste and undo.** The REU gap moves 64 bytes per load/store pair (`buffer_reu.c:move_gap`), paste inserts individual bytes (`editor.c:s64_editor_paste`), and history swap processes individual replacements. Use existing host REU counters and VICE jiffy diagnostics, then measure end-to-end shipping UI/hardware. Change buffer size, bulk operations or selective compute acceleration only if timings and RAM/overlay guards justify them.
- [ ] **P1 — Reduce redundant full-source passes on Save.** `write_document_file` calls `s64_document_write_wire` for validation then again for the actual file; each 8TML writer invocation validates and walks the entire document. Measure the cost and consider a single validated streaming pass without compromising preflight, recovery or write-failure behavior; also check duplicated file-existence probes.
- [ ] **P1 — Surface REU read failures distinctly from absent text.** `s64_buffer_char_at` returns zero on failed transfer and `s64_buffer_find_byte` returns document length; consumers can treat errors as ordinary characters/end-of-search. Trace shipping callers and add targeted fault-injection tests before changing APIs or error paths.
- [ ] **P2 — Target rendering and long-line navigation after measuring.** `ui.c` repaints 40 characters per changed row, with full redraws for selection/pointing and horizontal-line scans; `editor.c` has line-start/visibility rescans. Compare append, middle-line insertion, scrolling, large selections, search/replace and 40-column wrap against current fast paths. Avoid global redraw/line-index rewrites without evidence.
- [ ] **P2 — Strengthen Editor recovery/error-path coverage.** Test invalid/oversized file Open, interruption, pre-existing `.tmp`, partial REU paste/undo, full-history boundary, mid-save power loss, and source/header preservation. Keep failures visible and preserve dirty state and recoverability; do not claim a partially failed edit is rolled back.
- [ ] **P2 — Establish reproducible responsiveness gates.** Pin build/firmware/REU/turbo/root and exact package hashes, benchmark 1/16/60 KiB short-line and single-long-line fixtures, capture median/worst-case input-to-screen response, first edit after navigation, load/save times and REU transaction counts. Host counters and direct VICE jiffies omit shipping overlays/UI; confirm meaningful gains on C64U with physical keyboard, plus separate SD/USB acceptance. Preserve 1,024-byte Publisher/launcher headroom and all Editor fit guards.

## P1 — Slim Publisher interface after existing acceptance

**Scope:** no GUI framework, new browser, proprietary server or remote manager. [Publisher UI](docs/PUBLISHER.md) is the concise target. First finish the Editor and standard API hardware gates above, including the separate PR #67 REU-load optimisation.

- [x] Implement a compact numbered keyboard home screen and numbered publish/replace confirmation without changing standard network/storage paths (PR #66). **Code committed; build, fit, VICE and C64U tests are unverified.** Legacy letter shortcuts remain available.
- [ ] Build/test the modified Publisher and its matching SD/USB packages, check >=1,024-byte Publisher/launcher headroom, and validate actual number keys, confirmation/cancellation and BASIC/DOS restoration before calling the UI accepted.
- [x] **Initial local picker coded, protocol corrected:** bounded Ultimate-DOS OPEN_DIR plus a **single READ_DIR streaming multiple entry packets**, six numbered `.8tml` rows, paging and manual-name fallback. The earlier passing host fixtures modeled READ_DIR incorrectly and are superseded; new multi-packet and split-packet host regressions are committed but **not yet executed in this environment**. Packet-aware UCI support is enabled only in independent Publisher builds. **Host retest, C64 compilation, memory fit, VICE and hardware remain unverified**; no release gate is claimed. Folder traversal remains deferred.
- [ ] Add a bounded server-name selector using the **existing** standard authenticated list parser. Choosing an entry sets only the remote target; retain manual entry and fresh-list explicit replacement confirmation.
- [ ] Optional: reuse a *small* portion of Editor mouse/joystick code for clickable numbered rows **only if measured fit and hardware reliability allow**. Keyboard-only operation is sufficient for release.
- [ ] Exercise focused host/VICE and exact real-C64U input, storage, server and restoration acceptance with the new file controls. No automatic retries of uncertain uploads.

## P2 — Optional download-to-edit, not part of initial release

- [ ] Check whether T8WN or another compatible reader already saves raw source locally. If not, first verify independently documented raw 8TML URLs and C64U-compatible HTTP GET. Implement a single staged, collision-safe download only when needed and technically proven; never add a proprietary endpoint, HTTPS downgrade or bulk file manager. Test the Editor/republication round trip separately.
## P1 — Retire obsolete product paths after acceptance

- [x] Trace active/legacy compile, validation and release-package callers; trace retained shared dependencies before each deletion. Current alpha packaging contains only the standard client SD/USB packages; shared configuration layout, crypto helpers and recovery remain required.
- [ ] Once replacements pass required hardware/interoperability gates, remove obsolete code/assets/tests and stale server instructions incrementally. CI package count and standard test dependencies are corrected; the historical native overlay failure remains explicit. Preserve user files, credentials/recovery data, shared safety code and existing release assets.
- [ ] Complete exact release/package checksum and rollback audit. No merge, deployment or stable promotion while required gates remain failed, blocked or unverified. User-authorized alpha previews must disclose these limits.

## Existing unresolved limits

- [ ] Investigate slow large-document operations and startup verification; measured16385-byte Publisher source load91.87 seconds with Turbo Control Off. Do not combine speculative performance rewrites with protocol migration.
- [ ] Resolve USB transfer/readback reliability and real USB-only acceptance on the actual firmware before recommending USB installation.
- [ ] Confirm draft evolution, independent-server support and validator redistribution licensing; do not invent private conditional-write extensions.
