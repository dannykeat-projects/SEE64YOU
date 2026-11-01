# SEE64YOU work queue

Scope: [Product](docs/PRODUCT.md). Procedures: [Testing](docs/LOCAL_TESTING.md). Completed investigations and receipts remain in Git history and private build artifacts.

## Current baseline

Alpha.7 uses the ten-file SYSTEM layout and stores ordinary documents in SEE64YOU/docs/; settings, keys, recovery and journals remain at the root. First Save handles the Ultimate missing-directory status and creates docs. Host regressions, SD/USB fit/manifests, matching VICE and bounded SD hardware acceptance pass; remaining gates are below.

Save replacement/rollback, batched loading, resident reserve recovery and numbered selectors are implemented. Earlier receipts do not certify newer combined bytes or establish comparable loading gains. Alpha.6 exact-package network checks covered Settings and read-only Publisher; full uploads/failures remain unverified.

## Release acceptance

- [x] Verify missing-docs first Save and subsequent saves/recovery, native Open/overwrite/rollback/collision failures, Publisher numbered/typed source and cancellation on backed-up C64U SD storage. Exact files, DOS contexts, settings, IRQ/turbo/colours and responsive BASIC are restored; only the test-created empty docs directory is removed. Matching SD/USB package VICE checks pass.
- [ ] Complete shipping Editor Open/overwrite-prompt and Publisher pending-message visibility acceptance. Native storage checks do not certify those UI paths.
- [ ] Complete exact-package Publisher status/list/create/replace/cancel, size/key/network failures and byte-exact independent-server readback. Verify source/target confirmation and full file/configuration/machine restoration; never retry uncertain writes automatically.
- [ ] Verify physical keyboard/Link controls, numbered Settings/key import and remote paging/selection/cancellation. REST injection is separate evidence.
- [ ] Test cold-power byte-exact reopen and permitted interruption/recovery on disposable data, preserving source, temporaries and machine state.
- [ ] Resolve USB transfer/readback reliability and actual USB-only operation on the selected mount; SD acceptance and USB builds alone are insufficient.
- [ ] Verify published source and relative-link navigation in an external compatible reader on an independent server; record versions and limitations.

## Editor reliability and performance

- [ ] **P1 — Finish shipping responsiveness acceptance after host/VICE profiling.** Host counters cover first midpoint edit with the gap at EOF, 64/256/1,024-byte paste, undo and Save's two codec calls; direct VICE jiffies cover the existing 48 Editor cases. The REU gap moves 64 bytes per load/store pair (`buffer_reu.c:move_gap`), paste inserts individual bytes (`editor.c:s64_editor_paste`), and history swap processes individual replacements. Measure end-to-end shipping UI/hardware before changing transfer sizes, bulk operations or acceleration; retain RAM/overlay guards.
- [ ] **P1 — Reduce redundant full-source passes on Save.** `write_document_file` calls `s64_document_write_wire` for validation then again for the actual file; each 8TML writer invocation validates and walks the entire document. Measure the cost and consider a single validated streaming pass without compromising preflight, recovery or write-failure behavior; also check duplicated file-existence probes.
- [x] **P1 — Surface reported REU read failures distinctly from absent text.** A sticky fault blocks Editor content mutation, source serialization and recovery deletion; the Editor offers normal BASIC exit without discarding disk copies. Host injection covers character/search/split reads, word deletion/cut, every serialization transfer and both Save writer invocations. Native DMA completion/corruption detection is not provided by the current REU driver; real-C64U acceptance remains required.
- [ ] **P2 — Target rendering and long-line navigation after measuring.** `ui.c` repaints 40 characters per changed row, with full redraws for selection/pointing and horizontal-line scans; `editor.c` has line-start/visibility rescans. Compare append, middle-line insertion, scrolling, large selections, search/replace and 40-column wrap against current fast paths. Avoid global redraw/line-index rewrites without evidence.
- [ ] **P2 — Strengthen Editor recovery/error-path coverage.** Test invalid/oversized file Open, interruption, pre-existing `.tmp`, partial REU paste/undo, full-history boundary, mid-save power loss, and source/header preservation. Keep failures visible and preserve dirty state and recoverability; do not claim a partially failed edit is rolled back.
- [ ] **P2 — Establish reproducible responsiveness gates.** Pin build/firmware/REU/turbo/root and exact package hashes, benchmark 1/16/60 KiB short-line and single-long-line fixtures, capture median/worst-case input-to-screen response, first edit after navigation, load/save times and REU transaction counts. Host counters and direct VICE jiffies omit shipping overlays/UI; confirm meaningful gains on C64U with physical keyboard, plus separate SD/USB acceptance. Preserve 1,024-byte Publisher/launcher headroom and all Editor fit guards.

- [ ] Benchmark shipping 1/16/60 KiB loading against a comparable baseline before claiming batched-loading gains; isolate codec/DOS/UI costs.

## Publisher and retained legacy paths

- [ ] Bound local-directory transport/drain time and support cancellation during enumeration; scan limits alone do neither.
- [ ] Retire proven-unused legacy code after caller tracing and replacement acceptance; preserve shared configuration, crypto and recovery.
- [ ] Reconcile the failed historical managed Editor fit target (BSS +612, MODEL_CODE +2489, FILE_RODATA +10, PUBLICATION_CODE +747 bytes). Independent-package acceptance does not mean the full historical matrix passed; preserve guards.
- [ ] Confirm publishing-draft evolution, independent-server support and validator redistribution licensing without private API extensions.
- [ ] Optional download-to-edit requires independently documented raw-source access and supported transport. Check existing compatible tools first; API 1 has no source GET or delete.

## Validation constraints

The shared self-hosted runner is restricted to private repositories. SEE64YOU was last checked as public; verify visibility before enabling it. Preserve memory guards, manifests, exact hashes and restoration evidence. Mark gates passed / failed / blocked / unverified for their tested commit and inputs.

- [x] Consolidate duplicated documentation and distinguish retained legacy components.
- [x] Audit and retire obsolete branches: existing REU, Editor/link and responsiveness work is already incorporated or superseded; merge the documentation cleanup and retain recoverable historical commits without merging obsolete diagnostics or product paths.
