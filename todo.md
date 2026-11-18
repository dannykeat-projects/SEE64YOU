# SEE64YOU work queue

Scope: [Product](docs/PRODUCT.md). Procedures: [Testing](docs/LOCAL_TESTING.md). Completed investigations and receipts remain in Git history and private build artifacts.

## Current baseline

Alpha.7 uses the ten-file SYSTEM layout and stores ordinary documents in SEE64YOU/docs/; settings, keys, recovery and journals remain at the root. First Save handles the Ultimate missing-directory status and creates docs. Host regressions, SD/USB fit/manifests, matching VICE and bounded SD hardware acceptance pass; remaining gates are below.

Save replacement/rollback, batched loading, resident reserve recovery and numbered selectors are implemented. Earlier receipts do not certify newer combined bytes or establish comparable loading gains. Alpha.6 exact-package network checks covered Settings and read-only Publisher; full uploads/failures remain unverified.

## Approved Editor format expansion (planned; no implementation yet)

The current shipped Editor accepts `.8tml` only. Product behaviour and boundaries are in [Product scope](docs/PRODUCT.md). Implement this as one bounded change series; do not duplicate the editor or alter release artefacts before validation.

- [ ] **P1 — Pin 8SS source and publishing rules.** Obtain authoritative current 8SS syntax/examples, 8TML style-reference rules and independently supported publishing behaviour. Do not infer 8SS grammar from CSS, invent required headers, or claim 8SS upload support from filename acceptance alone. If verification remains blocked, implement literal local `.8ss` editing with character/size safety and defer any format-specific validator/template/Publisher support.
- [ ] **P1 — Add document kind in existing editing/storage path.** Inspect `client/src/document.c`, `client/src/storage/local.c`, Editor File/New/Open/Save As menus and call sites before edits. Use one lightweight kind (8TML, 8SS, general text) across reader, writer, New, title/status and Save As. Preserve 8TML header rules, literal-source/unknown-construct retention, REU/editor buffers, no autosave and fail-safe Open/Save semantics. Make zero-byte text files valid.
- [ ] **P1 — Define safe filename and extension behaviour.** New menu `1 8TML / 2 8SS / 3 Plain Text / 0 Cancel`; defaults `.8tml`, `.8ss`, `.txt`. Explicit `README.md`, `notes.ini`, `data.csv` remain as typed in general-text mode. Case-insensitive `.8tml`/`.8ss` are always special; treat `.tmp`/`.bak` recovery as copies of the original source type, not general text. Reject unsafe paths, reserved package files, incompatible format renames and collisions without silently changing or overwriting user data. Avoid speculative extension fallbacks on Open.
- [ ] **P1 — Preserve current text limits and UI.** Reuse the ASCII/LF codec (CR/CRLF input normalized), 61,440-byte limit, existing 40x25 menus, 16-step history, manual Save, temporary/backup and journal protections. Reject binary, unsupported character/encoding and oversized sources cleanly. Keep 8TML-only formatting from altering 8SS/plain text; no highlighting, preview, new allocators or new editor program.
- [ ] **P1 — Add targeted regression coverage.** Test new/open/save/reopen for all three kinds, empty `.txt`, explicit `.md`/extensionless text, uppercase suffixes, invalid 8TML header, arbitrary valid 8SS source, unsupported byte/control input, CRLF normalization, max size, unrecognised but safe extension, Save As suffix changes, `.tmp`/`.bak` recovery and existing unsaved/collision/rename/interruption behaviour. Compare all existing 8TML host fixtures and byte output (after documented normalization).
- [ ] **P1 — Shipping fit and hardware acceptance.** Run narrow host tests then affected regressions, whole SD/USB independent-package build, manifests/fit/headroom checks, VICE and bounded C64U keyboard/storage/recovery/BASIC-return tests. Preserve >=1,024-byte Publisher/launcher headroom and Editor resident reserve. Mark each gate passed/failed/blocked/unverified with exact commit/package, not inherited results.
- [ ] **P2 — Publisher 8SS integration, conditional.** After confirming published API/server support, include `.8ss` in local selection and verify byte-exact server readback and external-reader styling, while preserving 8TML-only publication if unverified. General text remains local-only; no new remote protocol or automatic download-to-edit.
- [ ] **Documentation and delivery.** Once implementation passes its gates, update README, Editor/Install/Architecture/Runtime docs and examples to reflect *actual* behaviour (not this roadmap), clarify limits, and reconcile this section. Do not commit unrelated code, deploy or release without separate authorization.

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
